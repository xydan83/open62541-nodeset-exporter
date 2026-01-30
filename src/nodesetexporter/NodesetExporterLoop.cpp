//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/NodesetExporterLoop.h"
#include "nodesetexporter/common/PerformanceTimer.h"
#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/common/v_1.3_Compatibility.h"

#include <open62541/types.h>

#include <algorithm>
#include <functional>

// NOLINTBEGIN
#define CONSTRUCT_MAP_ITEM(key) {key, #key}

#define CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(key) {UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, key), UA_TYPES_NODEID), #key}

#define CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(key) {UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, key), UA_TYPES_EXPANDEDNODEID)}
// NOLINTEND

namespace nodesetexporter
{

using PerformanceTimer = nodesetexporter::common::PerformanceTimer;
using nodesetexporter::common::UaIdIdentifierToStdString;

#pragma region Methods for obtaining and generating data

#pragma region Getting ID attribute

StatusResults NodesetExporterLoop::GetNodeAttributes(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res)
{
    m_logger.Trace("Method called: GetNodeAttributes()");

    // Подготовка запроса для получения атрибутов узлов с возможностью учета ограничения MaxNodesPerRead.
    size_t attr_counter = 0;
    for (size_t index = 0; index < node_ids.size(); ++index)
    {
        auto attr = GetCommonNodeAttributes();
        switch (node_classes_req_res.at(index).node_class)
        {
        case UA_NODECLASS_OBJECT:
            attr.merge(GetObjectNodeAttributes());
            break;
        case UA_NODECLASS_VARIABLE:
            attr.merge(GetVariableNodeAttributes());
            break;
        case UA_NODECLASS_OBJECTTYPE:
            attr.merge(GetObjectTypeNodeAttributes());
            break;
        case UA_NODECLASS_VARIABLETYPE:
            attr.merge(GetVariableTypeNodeAttributes());
            break;
        case UA_NODECLASS_REFERENCETYPE:
            attr.merge(GetReferenceTypeNodeAttributes());
            break;
        case UA_NODECLASS_DATATYPE:
            attr.merge(GetDataTypeNodeAttributes());
            break;
        default:
            m_logger.Warning(
                "Get attributes of node class {} not implemented. Node ID: {}",
                m_ignored_nodeclasses.at(node_classes_req_res.at(index).node_class),
                node_classes_req_res.at(index).exp_node_id.get().ToString());
            attr.clear();
        }
        attr_counter += attr.size();
        nodes_attr_req_res.emplace_back(IOpen62541::NodeAttributesRequestResponse{.exp_node_id = node_ids.at(index), .attrs = attr});
    }
    // The OPC UA standard for receiving attributes guarantees - The size and order of this list matches the size and order of the nodesToReadrequest
    // parameter. https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2 I extend this rule to the library as well.
    if (!nodes_attr_req_res.at(0).attrs.empty()) // There should always be at least one node with an unnecessary number of attributes to fulfill the request.
    {
        return m_open62541_lib.ReadNodesAttributes(nodes_attr_req_res, attr_counter); // REQUEST<-->RESPONSE
    }

    return StatusResults{StatusResults::Good};
}

#pragma endregion Getting ID attribute


#pragma region Receiving and processing reference

StatusResults NodesetExporterLoop::GetNodeReferences(const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: GetNodeReferences()");

    // Request for obtaining links of all types for each node. According to indexation of links as with attributes.
    std::ranges::copy(node_ids, std::back_inserter(node_references_req_res));
    if (m_open62541_lib.ReadNodeReferences(node_references_req_res) == StatusResults::Fail) // REQUEST<-->RESPONSE
    {
        return StatusResults::Fail;
    }

    // I check the statuses of each individual NodeID request
    for (size_t index = 0; index < node_references_req_res.size(); ++index)
    {
        // Any node must have at least one link. If it is not there, I believe that there was an error in obtaining the list or that such a node does not exist.
        if (node_references_req_res.at(index).references.empty())
        {
            // In the case of the flat list mode and the mode of recreating the starting node, I ignore the error receiving links (node does not exist).
            // Note: For flat list mode and specifying the starting node i=85, it makes no difference whether I get links from it or not.
            if (m_external_options.flat_list_of_nodes.create_missing_start_node && m_external_options.flat_list_of_nodes.is_enable && index == 0)
            {
                continue;
            }
            return StatusResults::Fail;
        }
    }
    return StatusResults::Good;
}

void NodesetExporterLoop::CorrectionUnnecessaryHasTypeDefinitionReferences(std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr)
{
    m_logger.Trace("Method called: CorrectionUnnecessaryHasTypeDefinitionReferences()");

    bool has_type_defenition_ref_detected = false;
    m_filtered_references_tmp.clear();
    m_filtered_references_tmp.reserve(ref_desc_arr.size());
    // Checking for the presence of back references and generating them from text identifiers, as well as replacing the type HasTypeDefinition = BaseVariableType(62).
    // We need to know in principle that there are no back references, even if we can't add them.
    for (auto& ref : ref_desc_arr) // References
    {
        if (UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_ns0id_hastypedefenition_node_id))
        {
            // Since it was noticed that some systems set a back reference for the HasTypeDefinition type, to avoid
            // incorrect processing for ParentID, all such links will initially be assigned direct directions.
            if (!ref.GetRef().isForward)
            {
                m_logger.Warning("A reverse reference of type HasTypeDefinition was found for node {}. Fixing...", UaIdIdentifierToStdString(ref.GetRef().nodeId.nodeId));
            }
            ref.GetRef().isForward = true;
            // Add HasTypeDefinition only once.
            // The SourceNode of this ReferenceType shall be an Object or Variable. If the SourceNode is an Object, then the TargetNode shall be an ObjectType; if the SourceNode is a Variable,
            // then the TargetNode shall be a VariableType - This means that the direction always (source) goes from the variable or object to its type
            // Each Variable and each Object shall be the SourceNode of exactly one HasTypeDefinition Reference. - but this means that each object can have only one such
            // link. https://reference.opcfoundation.org/Core/Part3/v104/docs/7.13
            if (!has_type_defenition_ref_detected)
            {
                m_filtered_references_tmp.emplace_back(std::move(ref));
            }
            else
            {
                m_logger.Warning("More than one reference of type HasTypeDefinition was found on node {}. Removing...", UaIdIdentifierToStdString(ref.GetRef().nodeId.nodeId));
            }

            has_type_defenition_ref_detected = true;
            continue;
        }
        m_filtered_references_tmp.emplace_back(std::move(ref));
    }

    // Replace the temporary array with processed links with the one that was previously received (and with missing links, in theory, after moving they should
    // be disabled.
    ref_desc_arr.swap(m_filtered_references_tmp);
}

inline bool NodesetExporterLoop::HasReverseReference(const std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr)
{
    m_logger.Trace("Method called: HasReverseReference()");
    return std::ranges::any_of(
        ref_desc_arr,
        [](const auto& ref)
        {
            return !ref.GetRef().isForward;
        });
}

inline bool NodesetExporterLoop::ReplaceBaseVariableType(const UATypesContainer<UA_ExpandedNodeId>& node_id, std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr)
{
    m_logger.Trace("Method called: ReplaceBaseVariableType()");
    return std::ranges::any_of(
        ref_desc_arr,
        [this, &node_id](auto& ref)
        {
            if (UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_ns0id_hastypedefenition_node_id) && UA_NodeId_equal(&ref.GetRef().nodeId.nodeId, &m_ns0id_basevariabletype_node_id))
            {
                m_logger.Warning("For node {} we find reference with HasTypeDefinition = BaseVariableType(62). Change to BaseDataVariableType(63).", node_id.ToString());
                ref.GetRef().nodeId.nodeId.identifier.numeric = UA_NS0ID_BASEDATAVARIABLETYPE; // NOLINT(cppcoreguidelines-pro-type-union-access)
                return true;
            }
            return false;
        });
}

inline bool NodesetExporterLoop::AddNodeReverseReference(const UATypesContainer<UA_ExpandedNodeId>& node_id, std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr)
{
    m_logger.Trace("Method called: AddNodeReverseReference()");
    if (node_id.GetRef().nodeId.identifierType != UA_NODEIDTYPE_STRING)
    {
        m_logger.Error("Node {} didn't have a string ID, so we can't build a inverse reference.", node_id.ToString());
        return false;
    }

    // I create one backlink that will point to the parent
    open62541::UATypesContainer<UA_ReferenceDescription> new_ref(UA_TYPES_REFERENCEDESCRIPTION);
    UA_NodeId_copy(&m_ns0id_hascomponent_node_id, &new_ref.GetRef().referenceTypeId);
    new_ref.GetRef().isForward = false;
    auto child_str = node_id.ToString();
    auto found_child_dot_index = child_str.find_last_of('.');
    // If a separator point is found, we delete the identifier after the last separator point in the identifier, including the point itself.
    if (found_child_dot_index != std::string::npos)
    {
        child_str = child_str.substr(0, found_child_dot_index);
    }
    else // For all nodes for which the parent cannot be further determined, I substitute the most basic node of the object.
    {
        child_str = "i=" + std::to_string(UA_NS0ID_OBJECTSFOLDER);
    }
    auto parent_node_id = UA_EXPANDEDNODEID(child_str.c_str());
    UA_ExpandedNodeId_copy(&parent_node_id, &new_ref.GetRef().nodeId);
    UA_ExpandedNodeId_clear(&parent_node_id); // Так-как не знаем, строковой (с указателем) будет объект или числовой, поэтому будем чистить.
    m_logger.Debug("For node {} adding reference:\n {}", node_id.ToString(), new_ref.ToString());
    ref_desc_arr.push_back(new_ref);
    return true;
}

StatusResults NodesetExporterLoop::KepServerRefFix(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res)
{
    m_logger.Trace("Method called: KepServerRefFix()");

    // Checking for the presence of backlinks and generating them from text identifiers, as well as replacing the type HasTypeDefinition = BaseVariableType(62).
    // We need to know in principle that there are no backlinks, even if we can't add them.
    // If a node does not have a list of links, we skip processing of such a node
    if (node_reference_req_res.references.empty())
    {
        return StatusResults::Good;
    }

    // Let's adjust the direction of HasTypeDefinition type links and remove unnecessary links. It is noticed that sometimes some Servers (MasterOPC)
    // they produce incorrect links of this type and there are more than one of them.
    CorrectionUnnecessaryHasTypeDefinitionReferences(node_reference_req_res.references);

    // For unknown reasons, in KepServer, nodes of the Variable class are set to HasTypeDefinition = BaseVariableType(62).
    // This abstract type cannot be used directly on nodes of this class. When importing nodesetloader we get
    // error. In this case, the easiest option is to change HasTypeDefinition to a more specific one, although still generic, but
    // non-abstract type BaseDataVariableType(63).
    ReplaceBaseVariableType(node_reference_req_res.exp_node_id, node_reference_req_res.references);

    // If backlinks are found, then there is no point for the current node to go further.
    if (HasReverseReference(node_reference_req_res.references))
    {
        return StatusResults::Good;
    }

    m_logger.Warning("For node {} we didn't find a inverse reference. Let's just add one.", node_reference_req_res.exp_node_id.get().ToString());
    // Algorithm for adding backlinks from text node identifiers.
    // The algorithm does not use deep analysis to identify link types. All ReferenceTypes will be of type HasComponent.
    // There is also no solution for analyzing the namespace in case the parent and child may have different namespaces.
    if (!AddNodeReverseReference(node_reference_req_res.exp_node_id, node_reference_req_res.references))
    {
        return StatusResults::Fail;
    }

    return StatusResults::Good;
}

inline void NodesetExporterLoop::DeleteFailedReferences(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res)
{
    m_logger.Trace("Method called: DeleteFailedReferences()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_reference_req_res.references.size());
    for (auto& ref : node_reference_req_res.references)
    {
        // todo When I add a list of standard ns = 0 knots, you need to make it so that we do not filter only standard
        //  ns=0 that will be on this list, otherwise we filter, as it happens that the custom nodes are added to ns=0.
        if (ref.GetRef().nodeId.nodeId.namespaceIndex != 0) // We do not filter references to ns=0
        {
            // Check for a reference to an ignored, known node
            UATypesContainer node_in_container(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
            if (m_ignored_node_ids_by_classes.contains(node_in_container))
            {
                m_logger.Warning(
                    "The {} reference {} ==> {} is IGNORED because this node is deleted",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    node_reference_req_res.exp_node_id.get().ToString(),
                    node_in_container.ToString());
                continue; // Не добавляем ссылку
            }
            // Check for a reference to a missing node filtered in the external environment
            if (!m_node_ids_set_copy.contains(node_in_container))
            {
                m_logger.Warning(
                    "The {} reference {} ==> {} is IGNORED because this node is missing",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    node_reference_req_res.exp_node_id.get().ToString(),
                    node_in_container.ToString());
                continue; // Do not add a reference
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_reference_req_res.references.swap(references_after_filter);
}

inline void NodesetExporterLoop::DeleteAllHierarchicalReferences(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res)
{
    m_logger.Trace("Method called: DeleteAllHierarchicalReferences()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    for (auto& ref : node_reference_req_res.references)
    {
        // Checking for a hierarchical link of any direction. Such links are not added to the list after the filter
        const UATypesContainer node_in_container(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
        if (m_hierarhical_references.contains(UATypesContainer(ref.GetRef().referenceTypeId, UA_TYPES_NODEID)))
        {
            m_logger.Warning(
                "{} hierarchical reference {} ==> {}  was detected and removed.",
                ref.GetRef().isForward ? "Forward" : "Reverse",
                node_reference_req_res.exp_node_id.get().ToString(),
                node_in_container.ToString());
            continue;
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_reference_req_res.references.swap(references_after_filter);
}

inline void NodesetExporterLoop::DeleteNotHasSubtypeReference(UA_NodeClass node_class, IOpen62541::NodeReferencesRequestResponse& node_reference_req_res)
{
    m_logger.Trace("Method called: DeleteNotHasSubtypeReference()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_reference_req_res.references.size());
    const auto node_class_contains_result = m_types_nodeclasses.contains(node_class);
    for (auto& ref : node_reference_req_res.references)
    {
        // In the nodes of the TYPES class, I check the back references to see if they contain references other than HasSubtype. If detected, skip adding such references to the resulting array
        // of the node in question, since the main parent references must be of this type. And all other references will be restored by the open62541 library.
        if (node_class_contains_result) // If the node class being processed is a TYPE class, then...
        {
            // Looking for a back reference with a reference type other than HasSubtype, but ignoring a reference of type i=85, since it should already be in the right place.
            // It is logical to assume that isForward(False) only have hierarchical references (no clear confirmation found in the standard), which can protect against removal of
            // NON-hierarchical references.
            if (!ref.GetRef().isForward && !UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_ns0id_hassubtype_node_id) && !UA_NodeId_equal(&ref.GetRef().nodeId.nodeId, &m_ns0id_objectfolder))
            {
                // If the referenceTypeID is some kind of custom type, then I will display its NodeID.
                // todo consider the option of requesting custom types from the server and outputting the BrowseName type.
                const auto hier_ref_in_storage = m_hierarhical_references.find(UATypesContainer(ref.GetRef().referenceTypeId, UA_TYPES_NODEID));
                const auto reference_name_of_id = hier_ref_in_storage != m_hierarhical_references.end() // NOLINT(cppcoreguidelines-pro-type-union-access)
                                                      ? hier_ref_in_storage->second // NOLINT(cppcoreguidelines-pro-type-union-access)
                                                      : UATypesContainer(ref.GetRef().referenceTypeId, UA_TYPES_NODEID).ToString();
                m_logger.Warning(
                    "Found {} ReferenceType=\"{}\"  ==> '{}' in class node {} with NodeID '{}'. Since we only need the HasSubtype inverse reference type in this node class, I`m "
                    "removing this reference.",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    reference_name_of_id,
                    UATypesContainer<UA_ExpandedNodeId>(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID).ToString(),
                    m_types_nodeclasses.at(node_class),
                    node_reference_req_res.exp_node_id.get().ToString());
                continue;
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_reference_req_res.references.swap(references_after_filter);
}

inline void NodesetExporterLoop::AddStartNodeIfNotFound(
    size_t node_index,
    UA_NodeClass node_class,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res,
    HasStartNodeSubtypeDetected& has_start_node_subtype_detected,
    StartNodeReverseReferenceCounter& start_node_reverse_reference_counter)
{
    m_logger.Trace("Method called: AddStartNodeIfNotFound()");

    start_node_reverse_reference_counter = 0;
    has_start_node_subtype_detected = false;
    // Get the first NodeReference with type Hierarchical inverse
    bool is_found_i85 = false; // Node search flag i=85
    for (auto& ref_obj : node_references_req_res.at(node_index).references)
    {
        // Search for a reference to node i=85 and add if missing (duplication protection).
        if (UA_NodeId_equal(&ref_obj.GetRef().nodeId.nodeId, &m_ns0id_objectfolder))
        {
            is_found_i85 = true;
            break;
        }
        // If a back reference is detected, add +1 to the counter. This operation should be after all deletion filters.
        start_node_reverse_reference_counter += static_cast<uint64_t>(!ref_obj.GetRef().isForward);
    }
    if (!is_found_i85) // If not found, add
    {
        // So, if necessary, I will add two links such as HasComponent to nodes 63 and 58 to solve the problem, when to solve the problem, when
        // Some servers in the structure of the nodes contain classes of the Variable class where hastypedefinition indicates an abstract node, thus, thus
        // It is believed that a variable class node is abstract. As a rule, such nodes are prohibited to create, but there is an exception in which
        // it is possible to use such nodes, if the composition has a parent (in this case, the start node) has two reverse links
        // (two additional parents) on i=58 [BaseObjectType] and i=63 [BaseVariableType].
        // This condition was found as a commentary in the library Open62541, I did not find evidence in the standard.
        if (m_external_options.flat_list_of_nodes.is_enable && m_external_options.flat_list_of_nodes.create_missing_start_node && m_external_options.flat_list_of_nodes.allow_abstract_variable)
        {
            AddCustomRefferenceToNodeID(m_ns0id_baseobjecttype_node_id, node_index, UA_NS0ID_HASCOMPONENT, false, node_references_req_res);
            AddCustomRefferenceToNodeID(m_ns0id_basedatavariabletype_node_id, node_index, UA_NS0ID_HASCOMPONENT, false, node_references_req_res);
        }

        // Addition if the starting node is a node of the class TYPES.
        AddCustomRefferenceToNodeID(m_external_options.parent_start_node_replacer, node_index, UA_NS0ID_ORGANIZES, false, node_references_req_res);

        // Addition if the starting node is a node of the class TYPES.
        if (m_types_nodeclasses.contains(node_class))
        {
            // If the starting node is the class of the TYPES node, then we mark the presence of such a starting node for further actions later.
            has_start_node_subtype_detected = true;
        }
    }
}

inline void NodesetExporterLoop::CreateAttributesForStartNode(
    std::vector<IOpen62541::NodeAttributesRequestResponse>& node_attr_res_req,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: CreateAttributesForStartNode()");

    const size_t start_node_index = 0;

    // In the event that the start node through NodeId coincides with some existing, and when queries data are obtained, we delete them, remove them,
    // As-how the knot will be created from scratch.
    if (!node_references_req_res.at(start_node_index).references.empty())
    {
        node_references_req_res.at(start_node_index).references.clear();
    }

    // Add attributes of the start node
    auto start_node_id_name = common::UaIdIdentifierToStdString(node_attr_res_req.at(start_node_index).exp_node_id.get().GetRef().nodeId);
    const auto start_node_id_namepspace = node_attr_res_req.at(start_node_index).exp_node_id.get().GetRef().nodeId.namespaceIndex;
    node_attr_res_req.at(start_node_index)
        .attrs.at(UA_ATTRIBUTEID_BROWSENAME)
        .emplace(UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(start_node_id_namepspace, start_node_id_name.data()), UA_TYPES_QUALIFIEDNAME));
    node_attr_res_req.at(start_node_index)
        .attrs.at(UA_ATTRIBUTEID_DISPLAYNAME)
        .emplace(UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT(std::string().data(), start_node_id_name.data()), UA_TYPES_LOCALIZEDTEXT));
    node_attr_res_req.at(start_node_index)
        .attrs.at(UA_ATTRIBUTEID_DESCRIPTION)
        .emplace(UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT(std::string().data(), std::string("This is autogenerated start node.").data()), UA_TYPES_LOCALIZEDTEXT));

    // Adding reference to the type of node
    UATypesContainer<UA_ReferenceDescription> insertion_ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
    auto type_def_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    auto folder_type_node_id = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE);
    insertion_ref_desc.GetRef().isForward = true;
    UA_NodeId_copy(&type_def_node_id, &insertion_ref_desc.GetRef().referenceTypeId);
    UA_ExpandedNodeId_copy(&folder_type_node_id, &insertion_ref_desc.GetRef().nodeId);
    node_references_req_res.at(start_node_index).references.emplace(node_references_req_res.at(start_node_index).references.begin(), std::move(insertion_ref_desc));


    m_logger.Info("The attributes and type reference for the start node '{}' in 'Flat Mode' have been created.", node_attr_res_req.at(start_node_index).exp_node_id.get().ToString());
}

void NodesetExporterLoop::AddCustomRefferenceToNodeID(
    const UATypesContainer<UA_ExpandedNodeId>& ref_pointing_to_node_id,
    size_t add_ref_to_node_by_index,
    uint32_t reference_type_id,
    bool is_forward,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: AddCustomRefferenceToNodeID()");

    m_logger.Info(
        "Adding to node '{}' a new reference '{}' with reference type id '{}' and is_forward '{}'.",
        node_references_req_res.at(add_ref_to_node_by_index).exp_node_id.get().ToString(),
        ref_pointing_to_node_id.ToString(),
        reference_type_id,
        is_forward ? "true" : "false");
    // Adding a reverse reference to a knot
    UATypesContainer<UA_ReferenceDescription> insertion_ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
    auto ref_type_id_node = UA_NODEID_NUMERIC(0, reference_type_id);
    insertion_ref_desc.GetRef().isForward = is_forward;
    UA_NodeId_copy(&ref_type_id_node, &insertion_ref_desc.GetRef().referenceTypeId);
    UA_ExpandedNodeId_copy(&ref_pointing_to_node_id.GetRef(), &insertion_ref_desc.GetRef().nodeId);
    node_references_req_res.at(add_ref_to_node_by_index).references.emplace(node_references_req_res.at(add_ref_to_node_by_index).references.begin(), std::move(insertion_ref_desc));
}

inline std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> NodesetExporterLoop::GetParentNodeId(
    size_t node_index,
    UA_NodeClass node_class,
    const std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: GetParentNodeId()");

    const auto node_class_contains_result = m_types_nodeclasses.contains(node_class);
    for (const auto& ref_obj : node_references_req_res.at(node_index).references)
    {
        if (!ref_obj.GetRef().isForward)
        {
            // If nodes are classes of TYPES, then for such types ParentNodeID is refType = UA_NS0ID_HASSUBTYPE
            if (node_class_contains_result && !UA_NodeId_equal(&ref_obj.GetRef().referenceTypeId, &m_ns0id_hassubtype_node_id))
            {
                continue;
            }

            // Если узлы являются классами ТИПОВ - то для таких типов ParentNodeID является refType = UA_NS0ID_HASSUBTYPE
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.5.2 - ObjectType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.6.5 - VariableType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.8.3 - DataType NodeClass (not specified, but visible from UANodeSet.xsd)
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.3 - ReferenceType NodeClass
            // If the nodes are not TYPE classes (Instance classes). Parents can be indicated by various types of references.
            return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(ref_obj.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID); // Обнаружили первую родительскую ссылку - вышли из цикла
        }
    }
    return nullptr;
}

inline std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> NodesetExporterLoop::GetBaseObjectType(UA_NodeClass node_class)
{
    m_logger.Trace("Method called: GetBaseObjectType()");

    switch (node_class)
    {
    case UA_NODECLASS_OBJECTTYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_TYPES_EXPANDEDNODEID);
    case UA_NODECLASS_VARIABLETYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE), UA_TYPES_EXPANDEDNODEID);
    case UA_NODECLASS_REFERENCETYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), UA_TYPES_EXPANDEDNODEID);
    case UA_NODECLASS_DATATYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), UA_TYPES_EXPANDEDNODEID);
    default:
        throw(std::out_of_range("node_classes_req_res[index].node.class"));
    }
}

#pragma endregion Receiving and processing references


StatusResults NodesetExporterLoop::GetNamespaces(std::vector<std::string>& namespaces)
{
    m_logger.Trace("Method called: GetNamespaces()");
    // Read all server namespaces
    const UATypesContainer<UA_ExpandedNodeId> server_namespace_array_request(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), UA_TYPES_EXPANDEDNODEID);
    UATypesContainer<UA_Variant> server_namespace_array_response(UA_TYPES_VARIANT);
    if (m_open62541_lib.ReadNodeDataValue(server_namespace_array_request, server_namespace_array_response) == StatusResults::Fail) // REQUEST<-->RESPONSE
    {
        return StatusResults::Fail;
    }

    // I remove the namespace with index 0 from the list; according to the standard, this is the OPC FOUNDATION space, which should be on every server by default.
    if (server_namespace_array_response.GetRef().arrayDimensionsSize == 0 && server_namespace_array_response.GetRef().type->typeKind == UA_TYPES_STRING)
    {
        for (size_t index = 1; index < server_namespace_array_response.GetRef().arrayLength; ++index)
        {
            // Should we compare row by row with "http://opcfoundation.org/UA/" or hope for a zero index?
            auto ua_namespace = static_cast<UA_String*>(server_namespace_array_response.GetRef().data)[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            namespaces.emplace_back(reinterpret_cast<char*>(ua_namespace.data), ua_namespace.length); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
    }
    else
    {
        m_logger.Error("Wrong array dimensions size or type kind in GetNamespaces request.");
        return StatusResults::Fail;
    }
    return StatusResults::Good;
}

StatusResults NodesetExporterLoop::GetAliases(const std::vector<NodeIntermediateModel>& node_intermediate_objs, std::map<std::string, UATypesContainer<UA_NodeId>>& aliases)
{
    m_logger.Trace("Method called: GetAliases()");
    for (const auto& node_intermediate_obj : node_intermediate_objs)
    {
        if (node_intermediate_obj.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLE || node_intermediate_obj.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLETYPE)
        {
            try
            {
                // todo I came up with the idea of producing not a string in the GetDataTypeAlias method, but a pair of values std::pair<std::string, UATypesContainer<UA_NodeId>>,
                //  then there will be no need to separately request UA_ATTRIBUTEID_DATATYPE.
                // Checking that the option is filled with the required type.
                const auto datatype_attr = node_intermediate_obj.GetAttributes().at(UA_AttributeId::UA_ATTRIBUTEID_DATATYPE);
                if (!datatype_attr.has_value())
                {
                    m_logger.Warning("DATATYPE has an empty value in NodeID: {}", node_intermediate_obj.GetExpNodeId().ToString());
                    continue;
                }

                const auto* const data_type_node_id = std::get_if<UATypesContainer<UA_NodeId>>(&datatype_attr.value());
                if (data_type_node_id == nullptr)
                {
                    m_logger.Critical("DATATYPE has wrong type in NodeID: {}", node_intermediate_obj.GetExpNodeId().ToString());
                    return StatusResults::Fail;
                }

                // I save only if the datatype belongs to the OPC UA base space.
                if (data_type_node_id->GetRef().namespaceIndex == 0)
                {
                    // Alias must be in only one instance
                    aliases.try_emplace(node_intermediate_obj.GetDataTypeAlias(), *data_type_node_id);
                }
            }
            catch (std::out_of_range&)
            {
                m_logger.Warning("DATATYPE attribute is missing from NodeID: {}", node_intermediate_obj.GetExpNodeId().ToString());
            }
        }

        // Add reference types as aliases
        for (const auto& [ref_desc, alias] : node_intermediate_obj.GetNodeReferenceTypeAliases())
        {
            if (ref_desc.GetRef().referenceTypeId.namespaceIndex == 0)
            {
                // Alias must be in only one instance
                aliases.try_emplace(alias, ref_desc.GetRef().referenceTypeId, UA_TYPES_NODEID);
            }
        }
    }
    return StatusResults::Good;
}

inline StatusResults NodesetExporterLoop::GetNodeClasses(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& list_of_nodes_from_one_start_node,
    std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res)
{
    m_logger.Trace("Method called: GetNodeClasses()");

    auto timer_nc = PREPARE_TIMER(m_external_options.is_perf_timer_enable);
    node_classes_req_res.reserve(list_of_nodes_from_one_start_node.size());
    std::ranges::copy(list_of_nodes_from_one_start_node.begin(), list_of_nodes_from_one_start_node.end(), std::back_inserter(node_classes_req_res));
    const auto status = m_open62541_lib.ReadNodeClasses(node_classes_req_res); // REQUEST<-->RESPONSE
    GET_TIME_ELAPSED_FMT_FORMAT(timer_nc, m_logger.Info, "ReadNodeClasses operation + copy nodeIDs: ", "");

    if (node_classes_req_res.empty())
    {
        m_logger.Error("Unable to get node classes from server.");
        return status;
    }

    // In the case of operation in flat components, as well as the mode of creating the starting nodes, I denote the class of the created start node. Only the start node is processed and
    // just not part of the standard, in fact, the main search goes by the node not i=85.
    if (m_external_options.flat_list_of_nodes.is_enable && m_external_options.flat_list_of_nodes.create_missing_start_node
        && !m_ns0_opcua_standard_node_ids.contains(node_classes_req_res.at(0).exp_node_id))
    {
        m_logger.Warning("NodeID '{}' is the 'Start Node' in 'Flat Mode' and will be created as an Object node class.", node_classes_req_res.at(0).exp_node_id.get().ToString());
        node_classes_req_res.at(0).node_class = UA_NodeClass::UA_NODECLASS_OBJECT;
        node_classes_req_res.at(0).result_code = UA_STATUSCODE_GOOD; // Так-как узла не существует в случае режима плоских узлов, то нужно игнорировать эту ошибку, а значит выставить хорошее значение.
    }

    // I create a list of ignored nodes and check the nodes for existence
    RESET_TIMER(timer_nc);
    for (const auto& nodes : node_classes_req_res)
    {
        // Existence check
        if (UA_StatusCode_isBad(nodes.result_code))
        {
            m_logger.Error("Node '{}' returned a bad result in the node class query: {}", nodes.exp_node_id.get().ToString(), UA_StatusCode_name(nodes.result_code));
            return StatusResults{StatusResults::Fail, StatusResults::GetNodeClassesFail};
        }

        // Creating a list of ignored nodes
        if (m_ignored_nodeclasses.contains(nodes.node_class))
        {
            m_ignored_node_ids_by_classes.insert(nodes.exp_node_id);
        }
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer_nc, m_logger.Info, "Making the lists of the ignored nodes by classes: ", "");

    return status;
}

inline StatusResults NodesetExporterLoop::GetNodesDataPreparation(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    // Receiving an attribute from the server.
    if (GetNodeAttributes(node_ids, node_classes_req_res, nodes_attr_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Prepare a request and get a list of references for each node
    // todo Is it worth getting references of absolutely all nodes from the selection, or should those that are not currently being processed not be included in the list?
    return GetNodeReferences(node_ids, node_references_req_res);
}

inline StatusResults NodesetExporterLoop::GetNodesDataFiltering(UA_NodeClass node_class, const UATypesContainer<UA_ExpandedNodeId>& node_id)
{
    // Depending on the ns0_custom_nodes_ready_to_work flag, different filters for working with ns=0 nodes are used
    if (m_external_options.ns0_custom_nodes_ready_to_work)
    {
        // Filtering of nodes that are contained in the list of nodes from the OPC UA standard. I do not add such nodes to the list for unloading.
        // User nodes ns=0 are passed by the filter.
        if (m_ns0_opcua_standard_node_ids.contains(node_id))
        {
            m_logger.Warning("The node with id {} is IGNORED because this node is part of the standard OPC UA set.", node_id.ToString());
            return StatusResults::Fail;
        }
    }
    else
    {
        // Filtering of all nodes with ns = 0. I do not add such nodes to the list for uploading.
        if (node_id.GetRef().nodeId.namespaceIndex == 0)
        {
            m_logger.Warning("The node with id {} is IGNORED because this node is from the OPC UA namespace", node_id.ToString());
            return StatusResults::Fail;
        }
    }

    // Filter: Filtering from creating non-exportable node types by class
    if (m_ignored_nodeclasses.contains(node_class))
    {
        m_logger.Warning("NodeID '{}' is IGNORED because this node has a NODE CLASS '{}' from the ignore list", node_id.ToString(), m_ignored_nodeclasses.at(node_class));
        return StatusResults::Fail;
    }
    return StatusResults::Good;
}

inline StatusResults NodesetExporterLoop::GetNodesDataReferenceCorrection(UA_NodeClass node_class, IOpen62541::NodeReferencesRequestResponse& node_reference_req_res)
{
    // Processing references for working with the KepServer server (and similar ones with similar features)
    if (KepServerRefFix(node_reference_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Depending on the mode of the formation of flat lists, filtering of reference removal is processed differently.
    if (m_external_options.flat_list_of_nodes.is_enable)
    {
        // Filter: 'Remove' all hierarchical references
        DeleteAllHierarchicalReferences(node_reference_req_res);
    }
    else
    {
        // Filter: 'Removing' broken references
        DeleteFailedReferences(node_reference_req_res);

        // Filter: In nodes of classes of type ReferenceTypes, DataTypes, ObjectTypes, VariableTypes 'Remove' back references other than the HasSubtype type.
        DeleteNotHasSubtypeReference(node_class, node_reference_req_res);
    }
    return StatusResults::Good;
}

inline auto NodesetExporterLoop::GetNodesDataStartNodeCreation(
    size_t index,
    size_t index_from_zero,
    UA_NodeClass node_class,
    const UATypesContainer<UA_ExpandedNodeId>& node_ids,
    std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    HasStartNodeSubtypeDetected has_start_node_subtype_detected = false;
    StartNodeReverseReferenceCounter start_node_reverse_reference_counter = 0;

    // The function of adding attributes to an artificially created start node
    if (m_external_options.flat_list_of_nodes.is_enable && m_external_options.flat_list_of_nodes.create_missing_start_node && index == 0)
    {
        CreateAttributesForStartNode(nodes_attr_req_res, node_references_req_res);
    }

    // The function of adding references in all subsequent nodes to an artificially borched start node or an existing node on the main
    // server as a source that will be like a start.
    if (m_external_options.flat_list_of_nodes.is_enable && index != 0)
    {
        AddCustomRefferenceToNodeID(node_ids, index_from_zero, UA_NS0ID_ORGANIZES, false, node_references_req_res);
    }

    // The function of processing references of starting nodes.
    // If the starting node does not have a binding to i=85, then such a node creates a reference to the one indicated in the variable parent_start_node_replacer.
    if (index == 0)
    {
        AddStartNodeIfNotFound(index_from_zero, node_class, node_references_req_res, has_start_node_subtype_detected, start_node_reverse_reference_counter);
    }

    return std::make_pair(has_start_node_subtype_detected, start_node_reverse_reference_counter);
}

inline auto NodesetExporterLoop::GetNodesDataParentId(
    size_t index,
    UA_NodeClass node_class,
    const std::pair<HasStartNodeSubtypeDetected, StartNodeReverseReferenceCounter>& start_node_prop,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    // Parsing and obtaining ParentID
    // Get the ParentID from the backlink. The first reverse link found is taken. Does not return the parent if the node is of class TYPE and has no backreference type
    // UA_NS0ID_HASSUBTYPE.
    auto t_parent_node_id = GetParentNodeId(index, node_class, node_references_req_res);

    // If the starting node is of type HasSubtype and this type of node does not have any back reference to the main parent of the HasSubtype type (this can happen with starting nodes),
    // then add such a reference to the base super-type, depending on the class of the node.
    // I'll check if the main parent of the super-type remains, and if not, then I'll add a parent to the base super-type.
    if (start_node_prop.first && start_node_prop.second == 0)
    {
        t_parent_node_id = GetBaseObjectType(node_class);

        m_logger.Warning("The start Node has a node TYPE class without any HasSubtype reverse reference. Adding a new HasSubtype parent reference {}.", t_parent_node_id->ToString());
        UATypesContainer<UA_ReferenceDescription> insertion_ref_desc_main(UA_TYPES_REFERENCEDESCRIPTION);
        insertion_ref_desc_main.GetRef().isForward = false;
        UA_NodeId_copy(&m_ns0id_hassubtype_node_id, &insertion_ref_desc_main.GetRef().referenceTypeId);
        UA_NodeId_copy(&t_parent_node_id->GetRef().nodeId, &insertion_ref_desc_main.GetRef().nodeId.nodeId);
        node_references_req_res.at(index).references.emplace(node_references_req_res.at(index).references.end(), std::move(insertion_ref_desc_main));
    }

    return t_parent_node_id;
}

StatusResults NodesetExporterLoop::GetNodesData(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<NodeIntermediateModel>& node_models)
{
    m_logger.Trace("Method called: GetNodesData()");

    // Logging a list of nodes for export
    if (m_logger.IsEnable(LogLevel::Debug))
    {
        for (size_t index = 0; index < node_ids.size(); ++index)
        {
            // To avoid constantly executing the loop and ToString before sending it to Debug, I check the logging level in advance.
            m_logger.Debug("GetNodesData beginning. NodeID: {}, class: {}", node_ids.at(index).ToString(), static_cast<int>(node_classes_req_res.at(index).node_class));
        }
        m_logger.Debug("Total nodes: {}", node_ids.size());
    }

    std::vector<IOpen62541::NodeAttributesRequestResponse> nodes_attr_req_res; // NODE ATTRIBUTES  (Attribute Service Set)
    std::vector<IOpen62541::NodeReferencesRequestResponse> node_references_req_res; // NODE REFERENCES (View Service Set)

    // Obtaining the necessary data for further processing.
    if (GetNodesDataPreparation(node_ids, node_classes_req_res, nodes_attr_req_res, node_references_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Processing, filtering and generating NodeIntermediateModel intermediate data for export.
    node_models.reserve(node_ids.size());
    for (size_t index = 0; index < node_ids.size(); ++index)
    {
        // Nodes that cannot be processed are eliminated.
        if (GetNodesDataFiltering(node_classes_req_res.at(index).node_class, node_ids.at(index)) == StatusResults::Fail)
        {
            continue; // If such a node is found, we skip it and move on.
        }

        // The node links are being corrected.
        if (GetNodesDataReferenceCorrection(node_classes_req_res.at(index).node_class, node_references_req_res.at(index)) == StatusResults::Fail)
        {
            return StatusResults::Fail;
        }

        // Processing the start node and its links
        auto start_node_prop = GetNodesDataStartNodeCreation(index, index, node_classes_req_res.at(index).node_class, node_ids.at(0), nodes_attr_req_res, node_references_req_res);

        // Get the parent node
        auto t_parent_node_id = GetNodesDataParentId(index, node_classes_req_res.at(index).node_class, start_node_prop, node_references_req_res);

        // Analyze the node to see if it belongs to the parent of the ignored type. If the parent was not found, then we should not add this node,
        // so as in the hierarchy if such a parent refers to some nodes that were previously deleted, and therefore child nodes
        // we shouldn't add.
        if (!t_parent_node_id)
        {
            m_logger.Warning("The node with id {} is IGNORED because this node has a PARENT NODE with wrong NODE CLASS", node_ids.at(index).ToString());
            continue;
        }

        // --- Creating an intermediate node model object and filling it with previously received and processed data ---
        m_logger.Debug("Filling NodeIntermediateModel...");
        NodeIntermediateModel nim;

        // NodeID
        nim.SetExpNodeId(node_ids.at(index).GetRef()); // Копирование (не должен изменять источник)

        // ParentNodeID
        if (t_parent_node_id == nullptr)
        {
            throw std::runtime_error("t_parent_node_id == nullptr");
        }
        nim.SetParentNodeId(std::move(*t_parent_node_id));

        // NodeClass
        nim.SetNodeClass(node_classes_req_res.at(index).node_class); // Копирование

        // NodeReferences
        if (node_references_req_res.at(index).references.empty())
        {
            throw std::runtime_error("node_references_req_res[index_from_zero].references.empty()");
        }
        nim.SetNodeReferences(std::move(node_references_req_res.at(index).references)); // Перемещение

        // NodeAttributes
        nim.SetAttributes(std::move(nodes_attr_req_res.at(index).attrs)); // Перемещение

        if (m_logger.IsEnable(LogLevel::Debug))
        {
            // To avoid constantly executing ToString before sending it to Debug, I check the logging level in advance.
            m_logger.Debug("{}", nim.ToString());
        }

        // Since many objects inside the NodeIntermediateModel are portable objects with dynamically allocated memory on the heap (for example, UATypesContainer, vector or map), when transferred,
        // all internal fields of the NodeIntermediateModel are transferred (or copied if they are simple types) to the new one created object, and the already empty nim object itself
        // is destroyed when exiting the method.
        m_logger.Debug("Move NodeIntermediateModel into std::vector<NodeIntermediateModel>");
        node_models.emplace_back(std::move(nim));
    }

    m_logger.Debug("-- Total nodes in NodeIntermediateModels: {} --", node_models.size());

    return StatusResults::Good;
}

std::set<std::reference_wrapper<ExpandedNodeId>, std::less<ExpandedNodeId>> NodesetExporterLoop::Distinct(std::vector<ExpandedNodeId>& node_ids) // NOLINT(modernize-use-transparent-functors)
{
    m_logger.Trace("Method called: Distinct()");

    std::vector<ExpandedNodeId> after_distinct_node_ids;
    // I use SET to quickly search for nodes using an algorithm of red-black wood. To reduce memory costs, the set stores signs, but the sorting and search for nodes occurs with real objects of the
    // nodes tied to the AFTER_DISTINCT_NODE_IDS through Reference_wrapper.
    std::set<std::reference_wrapper<ExpandedNodeId>, std::less<ExpandedNodeId>> fast_search_nodeid_ref_copy; // NOLINT
    after_distinct_node_ids.reserve(node_ids.size());
    // Primary initialization of storage facilities
    after_distinct_node_ids.push_back(*node_ids.begin());
    fast_search_nodeid_ref_copy.insert(std::ref(*after_distinct_node_ids.begin()));

    size_t new_index = 1;
    // DistINCT algorithm with the complexity of n * log (n)
    for (size_t index = 1; index < node_ids.size(); ++index) // сложность n
    {
        if (fast_search_nodeid_ref_copy.contains(node_ids.at(index))) // сложность log(n)
        {
            m_logger.Info("The found NodeID duplicate {} has been removed.", node_ids.at(index).ToString());
            continue;
        }
        after_distinct_node_ids.push_back(node_ids.at(index));
        fast_search_nodeid_ref_copy.insert(std::ref(after_distinct_node_ids.at(new_index)));
        ++new_index;
    }
    // I move the nodes after filtering
    node_ids.swap(after_distinct_node_ids);

    return fast_search_nodeid_ref_copy;
}

StatusResults NodesetExporterLoop::CheckStartNodesOnNS0()
{
    m_logger.Trace("Method called: CheckStartNodesOnNS0()");

    for (const auto& list_of_nodes_from_one_start_node : m_node_ids)
    {
        // In the case of the flat_list_of_nodes mode on and the determination of the starting unit of one list as i=85 - the check is passed.
        // Such a knot in this mode is permissible.
        if (m_external_options.flat_list_of_nodes.is_enable && UA_NodeId_equal(&list_of_nodes_from_one_start_node.second.at(0).GetRef().nodeId, &m_ns0id_objectfolder))
        {
            continue;
        }

        // In another case, depending on the flag of work with custom ns=0, one of two checks occurs one of the nodes
        if (m_external_options.ns0_custom_nodes_ready_to_work)
        {
            // If the mode of operation with custom nodes is included in the ns=0 space, then the belonging of such a node to the basic nodes of the OPC UA standard is checked.
            // Only custom nodes are allowed outside the standard otherwise error.
            if (m_ns0_opcua_standard_node_ids.contains(list_of_nodes_from_one_start_node.second.at(0)))
            {
                m_logger.Error("First NodeId (Start NodeId) is standard OPC UA node - '{}' in ns = 0", list_of_nodes_from_one_start_node.first);
                return {StatusResults::Fail, StatusResults::FailedCheckNs0StartNodes};
            }
        }
        else
        {
            // If work with custom nodes in the ns=0 space is not allowed, then an error is given when any such starting unit is given.
            if (list_of_nodes_from_one_start_node.second.at(0).GetRef().nodeId.namespaceIndex == 0)
            {
                m_logger.Error("First NodeId (Start NodeId) - '{}' has ns = 0", list_of_nodes_from_one_start_node.first);
                return {StatusResults::Fail, StatusResults::FailedCheckNs0StartNodes};
            }
        }
    }
    return StatusResults::Good;
}

#pragma endregion Методы получения и формирования данных

#pragma region Data Export Methods
StatusResults NodesetExporterLoop::ExportNodes(const std::vector<NodeIntermediateModel>& list_of_nodes_data)
{
    m_logger.Trace("Method called: ExportNodes()");
    m_logger.Info("Export nodes...");
    m_logger.Debug("List of added nodes:");
    StatusResults status_result = StatusResults::Good;

    for (const auto& node_model : list_of_nodes_data)
    {
        if (m_logger.IsEnable(common::LogLevel::Debug))
        {
            m_logger.Debug("Node: {}, node class: {}", node_model.GetExpNodeId().ToString(), static_cast<int>(node_model.GetNodeClass()));
        }

        switch (node_model.GetNodeClass())
        {
        case UA_NODECLASS_OBJECT:
            status_result = m_export_encoder.AddNodeObject(node_model);
            ++m_exported_nodes.object_nodes;
            break;
        case UA_NODECLASS_VARIABLE:
            status_result = m_export_encoder.AddNodeVariable(node_model);
            ++m_exported_nodes.variable_nodes;
            break;
        case UA_NODECLASS_OBJECTTYPE:
            status_result = m_export_encoder.AddNodeObjectType(node_model);
            ++m_exported_nodes.objecttype_nodes;
            break;
        case UA_NODECLASS_VARIABLETYPE:
            status_result = m_export_encoder.AddNodeVariableType(node_model);
            ++m_exported_nodes.variabletype_nodes;
            break;
        case UA_NODECLASS_REFERENCETYPE:
            status_result = m_export_encoder.AddNodeReferenceType(node_model);
            ++m_exported_nodes.referencetype_nodes;
            break;
        case UA_NODECLASS_DATATYPE:
            status_result = m_export_encoder.AddNodeDataType(node_model);
            ++m_exported_nodes.datatype_nodes;
            break;
        default:
            m_logger.Warning("NODECLASS with define {} not undefined", static_cast<uint>(node_model.GetNodeClass()));
        }
        if (status_result == StatusResults::Fail)
        {
            break;
        }
    }
    return status_result;
}

StatusResults NodesetExporterLoop::GetNodeDataAndExport(
    const std::vector<ExpandedNodeId>& list_of_nodes_from_one_start_node,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::map<std::string, UATypesContainer<UA_NodeId>>& aliases)
{
    m_logger.Trace("Method called: GetNodeDataAndExport()");

    auto timer_nde = PREPARE_TIMER(m_external_options.is_perf_timer_enable);
    RESET_TIMER(timer_nde);
    std::vector<NodeIntermediateModel> node_intermediate_obj;
    // Получение необходимых данных по узлам
    if (GetNodesData(list_of_nodes_from_one_start_node, node_classes_req_res, node_intermediate_obj) == StatusResults::Fail)
    {
        return StatusResults{StatusResults::Fail, StatusResults::GetNodesDataFail};
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer_nde, m_logger.Info, "GetNodesData operation: ", "");

    // Может быть такое, что в стартовой пачке будет один узел, который отсеется, например - метод, в итоге
    // node_intermediate_obj может быть пустой, но это не будет ошибкой.
    if (!node_intermediate_obj.empty())
    {
        // Получение данных по псевдонимам типов узлов
        RESET_TIMER(timer_nde);
        if (GetAliases(node_intermediate_obj, aliases) == StatusResults::Fail)
        {
            return StatusResults{StatusResults::Fail, StatusResults::GetAliasesFail};
        }
        GET_TIME_ELAPSED_FMT_FORMAT(timer_nde, m_logger.Info, "GetAliases and merge operation: ", "");

        // Экспорт узлов
        RESET_TIMER(timer_nde);
        if (ExportNodes(node_intermediate_obj) == StatusResults::Fail)
        {
            return StatusResults{StatusResults::Fail, StatusResults::ExportNodesFail};
        }
        GET_TIME_ELAPSED_FMT_FORMAT(timer_nde, m_logger.Info, "ExportNodes operation: ", "");
    }
    else
    {
        m_logger.Warning("node_intermediate_obj is empty.");
    }
    m_logger.Debug("End of node export step in loop");
    m_logger.Info("Exported nodes for one start node: {}", node_intermediate_obj.size());
    return StatusResults{StatusResults::Good, StatusResults::No};
}

#pragma endregion Методы экспорта данных

StatusResults NodesetExporterLoop::StartExport()
{
    m_logger.Trace("Method called: StartExport()");

    // Check for ns=0 in starting nodes. It is better to do this in a separate cycle before starting longer processing.
    // https://reference.opcfoundation.org/DI/v102/docs/11.2#_Ref252866620
    // https://documentation.unified-automation.com/uasdknet/3.0.10/html/L2UaAdrSpaceConceptNamespaces.html
    auto check_stat = CheckStartNodesOnNS0();
    if (check_stat == StatusResults::Fail)
    {
        return check_stat;
    }

    auto timer = PREPARE_TIMER(m_external_options.is_perf_timer_enable);
    // Actions before starting export
    if (Begin() == StatusResults::Fail)
    {
        return StatusResults{StatusResults::Fail, StatusResults::BeginFail};
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Begin operation: ", "");

    // Get namespace data and export
    RESET_TIMER(timer);
    std::vector<std::string> namespaces;
    if (GetNamespaces(namespaces) == StatusResults::Fail)
    {
        return StatusResults{StatusResults::Fail, StatusResults::GetNamespacesFail};
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNamespaces operation: ", "");

    RESET_TIMER(timer);
    if (ExportNamespaces(namespaces) == StatusResults::Fail)
    {
        return StatusResults{StatusResults::Fail, StatusResults::ExportNamespacesFail};
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNamespaces operation: ", "");

    std::map<std::string, UATypesContainer<UA_NodeId>> aliases;
    // Process the list of each starting node.
    for (auto& [name_of_start_node, list_of_nodes_from_one_start_node] : m_node_ids)
    {

#pragma region Node Filtering - Remove duplicates(all NodeIds are unique) and remove nodes from ns0
        RESET_TIMER(timer);
        // I move the finished copy of the set of nodes for quick search in the field for further actions.
        // For each iteration of the start node - its own set.
        m_node_ids_set_copy = Distinct(list_of_nodes_from_one_start_node);
        GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Distinct operation: ", "");
#pragma endregion Node Filtering - Remove duplicates(all NodeIds are unique) and remove nodes from ns0

        // todo Receiving data on ServerUris and exporting (relevance analysis is needed).

        // todo Receiving data on Models and exporting (relevance analysis is needed).

        // todo Receiving data on Extensions and exporting (relevance analysis is needed).

        // Collecting the necessary data on nodes into temporary structures and exporting.
        // Limit the processing of nodes at a time to a certain number.
        // This will allow you to maintain a balance between memory requirements and the number of requests over the network.
        // Since the collection of aliases coincides with requests for node types, it is rational to combine the collection of aliases and data for nodes in one request.
        // But according to the standard, data on aliases should be after data on namespaces.
        // I rely on the fact that each function for inserting the required data block knows and can be inserted into the right place.
        // It is worth noting that if data is limited by processing nodes, inserting intermediate data in a large sample can be time-consuming.

        std::vector<IOpen62541::NodeClassesRequestResponse> node_classes_req_res; // NODE CLASSES (Attribute Service Set)

        RESET_TIMER(timer);
        // It is necessary to obtain all classes before processing the rest of the data, since filtering of nodes and links is also done by node classes.
        auto status = GetNodeClasses(list_of_nodes_from_one_start_node, node_classes_req_res);
        if (status == StatusResults::Fail)
        {
            return status;
        }
        GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "get_node_classes operation: ", "");

        if (list_of_nodes_from_one_start_node.size() != node_classes_req_res.size())
        {
            throw std::runtime_error("list_of_nodes_from_one_start_node.second.size() != node_classes_req_res.size()");
        }

        // IMPORTANT: Since node classes are also index-bound, but all classes have already been obtained in advance,
        // it is necessary to synchronize the indexes of the classes and other structures of index-dependent nodes!
        // Batch retrieval of all other data and export.
        RESET_TIMER(timer);
        status = GetNodeDataAndExport(list_of_nodes_from_one_start_node, node_classes_req_res, aliases);
        if (status == StatusResults::Fail)
        {
            return status;
        }
        GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "get_node_data_and_export operations: ", "");
    }

    if (!aliases.empty())
    {
        RESET_TIMER(timer);
        // Exporting host type name aliases
        if (ExportAliases(aliases) == StatusResults::Fail)
        {
            return StatusResults{StatusResults::Fail, StatusResults::ExportAliasesFail};
        }
        GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportAliases operation: ", "");
    }
    else
    {
        m_logger.Warning("aliases is empty.");
    }

    RESET_TIMER(timer);
    // Actions at the end of the export - uploading to a buffer or to a file
    if (End() == StatusResults::Fail)
    {
        return StatusResults{StatusResults::Fail, StatusResults::EndFail};
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "End operation: ", "");
    m_logger.Info("Exported statistic:\n{}", m_exported_nodes.ToString());
    m_logger.Info("Total exported nodes: {}", m_exported_nodes.GetSumm());
    return StatusResults::Good;
}

// todo To form in Realtime through the Browse operation is so it is not clear how to isolate only hierarchical ReferenceType in statics,
//  and will also need to add custom-made RefereneType there.
const std::map<UATypesContainer<UA_NodeId>, std::string> NodesetExporterLoop::m_hierarhical_references{
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HIERARCHICALREFERENCES),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASCHILD),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_ORGANIZES),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASEVENTSOURCE),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_AGGREGATES),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASSUBTYPE),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASPROPERTY),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASCOMPONENT),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASNOTIFIER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASORDEREDCOMPONENT),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_CONTROLS),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_DATASETTOWRITER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_AGGREGATES),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASREFERENCEDESCRIPTION),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASLOWERLAYERINTERFACE),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_HASPUSHEDSECURITYGROUP),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_ALARMGROUPMEMBER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_ALARMSUPPRESSIONGROUPMEMBER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_REQUIRES)};

const std::map<std::uint32_t, std::string> NodesetExporterLoop::m_types_nodeclasses{
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_OBJECTTYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_REFERENCETYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_DATATYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_VARIABLETYPE)};

std::map<UA_NodeClass, std::string> NodesetExporterLoop::m_ignored_nodeclasses{// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
                                                                               CONSTRUCT_MAP_ITEM(UA_NODECLASS_UNSPECIFIED),
                                                                               CONSTRUCT_MAP_ITEM(UA_NODECLASS_METHOD),
                                                                               CONSTRUCT_MAP_ITEM(UA_NODECLASS_VIEW),
                                                                               CONSTRUCT_MAP_ITEM(__UA_NODECLASS_FORCE32BIT)};

// todo Try to form from the library files that are formed from the OPC UA standard.
const std::set<UATypesContainer<UA_ExpandedNodeId>> NodesetExporterLoop::m_ns0_opcua_standard_node_ids{
    CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(UA_NS0ID_ROOTFOLDER),
    CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(UA_NS0ID_OBJECTSFOLDER),
    CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(UA_NS0ID_TYPESFOLDER),
    CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(UA_NS0ID_VIEWSFOLDER)};

} // namespace nodesetexporter