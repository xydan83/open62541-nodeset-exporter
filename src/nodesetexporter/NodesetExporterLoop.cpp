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

#include <open62541/types.h>

#include <functional>

// NOLINTBEGIN
#define CONSTRUCT_MAP_ITEM(key)                                                                                                                                                                        \
    {                                                                                                                                                                                                  \
        key, #key                                                                                                                                                                                      \
    }

#define CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(key)                                                                                                                                                        \
    {                                                                                                                                                                                                  \
        UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, key), UA_TYPES_NODEID), #key                                                                                                                  \
    }

#define CONSTRUCT_NUMERIC_EXPANDED_NODE_ID_SET_ITEM(key)                                                                                                                                               \
    {                                                                                                                                                                                                  \
        UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, key), UA_TYPES_EXPANDEDNODEID)                                                                                                \
    }
// NOLINTEND

namespace nodesetexporter
{

using PerformanceTimer = nodesetexporter::common::PerformanceTimer;

#pragma region Methods for obtaining and generating data


#pragma region Getting ID attribute

StatusResults NodesetExporterLoop::GetNodeAttributes(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<size_t, size_t>& node_range,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res)
{

    // todo It is necessary to introduce tracking the Maxarraylength server parameter, how many elements the server will maintain in its array at a time, and in the case of attributes
    //  For each node you need to request a lot of parameters, where, from the point of view of the exchange of data, each request of the attribute corresponds to one occupied element of an array of
    //  the total, it can turn out like this, that when requested by 1000 knots, about 6-8 thousand are requested by attributes and the Maxarraylength and the server error are exceeded.
    for (size_t index = node_range.first; index < node_range.second; ++index)
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
                node_classes_req_res.at(index).exp_node_id.ToString());
            attr.clear();
        }
        nodes_attr_req_res.push_back(IOpen62541::NodeAttributesRequestResponse{node_ids.at(index), attr});
    }
    // The OPC UA standard for receiving attributes guarantees - The size and order of this list matches the size and order of the nodesToReadrequest
    // parameter. https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2 I extend this rule to the library as well.
    if (!nodes_attr_req_res.at(0).attrs.empty()) // There should always be at least one node with an unnecessary number of attributes to fulfill the request.
    {
        if (m_open62541_lib.ReadNodesAttributes(nodes_attr_req_res) == StatusResults::Fail) // REQUEST<-->RESPONSE
        {
            return StatusResults::Fail;
        }
    }
    if (node_range.second - node_range.first != nodes_attr_req_res.size())
    {
        throw std::runtime_error("range_for_nodes.second - range_for_nodes.first != nodes_attr_req_res.size()");
    }
    return StatusResults::Good;
}

#pragma endregion Getting ID attribute


#pragma region Receiving and processing reference

StatusResults NodesetExporterLoop::GetNodeReferences(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<size_t, size_t>& node_range,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: GetNodeReferences()");

    // Request for obtaining links of all types for each node. According to indexation of links as with attributes.
    std::copy(node_ids.begin() + static_cast<int64_t>(node_range.first), node_ids.begin() + static_cast<int64_t>(node_range.second), std::back_inserter(node_references_req_res));
    if (m_open62541_lib.ReadNodeReferences(node_references_req_res) == StatusResults::Fail) // REQUEST<-->RESPONSE
    {
        return StatusResults::Fail;
    }
    // Check the statuses of each individual NodeId request

    for (size_t index = 0; index < node_references_req_res.size(); ++index)
    {
        // In any node, at least one link must be. If it is not there, I think that there was a mistake to obtain a list or such a node does not exist.
        if (node_references_req_res.at(index).references.empty())
        {
            // In the case of a flat list and the reconstruction mode of the starting node, I ignore the error of obtaining links (the node does not exist).
            // Note: for the mode of the flat list and the indication of the start node i=85 there is no difference whether I receive links from it or not.
            if (m_external_options.flat_list_of_nodes.create_missing_start_node && m_external_options.flat_list_of_nodes.is_enable && node_range.first == 0 && index == 0)
            {
                continue;
            }
            return StatusResults::Fail;
        }
    }
    if (node_range.second - node_range.first != node_references_req_res.size())
    {
        throw std::runtime_error("range_for_nodes.second - range_for_nodes.first != node_references_req_res.size()");
    }
    return StatusResults::Good;
}

StatusResults NodesetExporterLoop::KepServerRefFix(std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    // Checking for the presence of back references and generating them from text identifiers, as well as replacing the type HasTypeDefinition = BaseVariableType(62).
    // We need to know in principle that there are no back references, even if we can't add them.
    for (auto& node_ref : node_references_req_res) // Node
    {
        // If the node does not have a list of links, we miss the processing of such a node
        if (node_ref.references.empty())
        {
            continue;
        }

        bool are_we_have_inverse_ref = false;
        bool are_we_found_base_variable_type = false;
        for (auto& ref : node_ref.references) // References
        {
            // For unknown reasons, in KepServer, nodes of the Variable class are set to HasTypeDefinition = BaseVariableType(62).
            // This abstract type cannot be used directly on nodes of this class. When importing nodesetloader we get an error.
            // In this case, the easiest option is to change HasTypeDefinition to a more specific, although still generic, but not abstract type BaseDataVariableType(63).
            if (UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_ns0id_hastypedefenition_node_id) && UA_NodeId_equal(&ref.GetRef().nodeId.nodeId, &m_ns0id_basevariabletype_node_id))
            {
                m_logger.Warning("For node {} we find reference with HasTypeDefinition = BaseVariableType(62). Change to BaseDataVariableType(63).", node_ref.exp_node_id.ToString());
                ref.GetRef().nodeId.nodeId.identifier.numeric = UA_NS0ID_BASEDATAVARIABLETYPE; // NOLINT(cppcoreguidelines-pro-type-union-access)
                are_we_found_base_variable_type = true;
            }

            // Only interested in back references
            if (!ref.GetRef().isForward)
            {
                are_we_have_inverse_ref = true;
            }

            // If both search criteria found what they were looking for, we can end the loop early, ensuring O(n) in the worst case.
            if (are_we_found_base_variable_type && are_we_have_inverse_ref)
            {
                break;
            }
        }

        if (are_we_have_inverse_ref)
        {
            continue;
        }

        m_logger.Warning("For node {} we didn't find a inverse reference. Let's just add one.", node_ref.exp_node_id.ToString());
        // Algorithm for adding back references from text node identifiers.
        // The algorithm does not use deep analysis to identify reference types. All ReferenceTypes will be of type HasComponent.
        // There is also no solution for analyzing the namespace in case the parent and child may have different namespaces.
        if (node_ref.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING)
        {
            // Create one back reference that will point to the parent
            UATypesContainer<UA_ReferenceDescription> new_ref(UA_TYPES_REFERENCEDESCRIPTION);
            UA_NodeId_copy(&m_ns0id_hascomponent_node_id, &new_ref.GetRef().referenceTypeId);
            new_ref.GetRef().isForward = false;
            auto child_str = node_ref.exp_node_id.ToString();
            auto found_child_dot_index = child_str.find_last_of('.');
            // If a separator dot is found, remove the identifier after the last separator dot in the identifier, including the dot itself.
            if (found_child_dot_index != std::string::npos)
            {
                child_str = child_str.substr(0, found_child_dot_index);
            }
            else // All nodes for which the parent cannot be determined further, I substitute the most basic node of the object.
            {
                child_str = "i=" + std::to_string(UA_NS0ID_OBJECTSFOLDER);
            }
            auto parent_node_id = UA_EXPANDEDNODEID(child_str.c_str());
            UA_ExpandedNodeId_copy(&parent_node_id, &new_ref.GetRef().nodeId);
            UA_ExpandedNodeId_clear(&parent_node_id); // Since we don’t know whether the object will be a string object (with a pointer) or a numeric one, so we’ll clean it up.
            m_logger.Debug("For node {} adding reference:\n {}", node_ref.exp_node_id.ToString(), new_ref.ToString());
            node_ref.references.push_back(new_ref);
        }
        else
        {
            m_logger.Error("Node {} didn't have a string ID, so we can't build a inverse reference.", node_ref.exp_node_id.ToString());
            return StatusResults::Fail;
        }
    }
    return StatusResults::Good;
}

inline void NodesetExporterLoop::DeleteFailedReferences(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: DeleteFailedReferences()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_references_req_res.at(node_index).references.size());
    for (auto& ref : node_references_req_res.at(node_index).references)
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
                    node_references_req_res.at(node_index).exp_node_id.ToString(),
                    node_in_container.ToString());
                continue; // Don't add a reference
            }
            // Check for a reference to a missing node filtered in the external environment
            if (!m_node_ids_set_copy.contains(node_in_container))
            {
                m_logger.Warning(
                    "The {} reference {} ==> {} is IGNORED because this node is missing",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    node_references_req_res.at(node_index).exp_node_id.ToString(),
                    node_in_container.ToString());
                continue; // Do not add a reference
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_references_req_res.at(node_index).references.swap(references_after_filter);
}

inline void NodesetExporterLoop::DeleteAllHierarhicalReferences(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: DeleteAllHierarhicalReferences()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    for (auto& ref : node_references_req_res.at(node_index).references)
    {
        // Checking for a hierarchical link of any direction. Such links are not added to the list after the filter
        UATypesContainer node_in_container(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
        if (m_hierarhical_references.contains(UATypesContainer(ref.GetRef().referenceTypeId, UA_TYPES_NODEID)))
        {
            m_logger.Warning(
                "{} hierarchical reference {} ==> {}  was detected and removed.",
                ref.GetRef().isForward ? "Forward" : "Reverse",
                node_references_req_res.at(node_index).exp_node_id.ToString(),
                node_in_container.ToString());
            continue;
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_references_req_res.at(node_index).references.swap(references_after_filter);
}

inline void NodesetExporterLoop::DeleteNotHasSubtypeReference(size_t node_index, UA_NodeClass node_class, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    m_logger.Trace("Method called: DeleteNotHasSubtypeReference()");

    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_references_req_res.at(node_index).references.size());
    bool is_type_class_node = m_types_nodeclasses.contains(node_class);
    for (auto& ref : node_references_req_res.at(node_index).references)
    {
        // In the nodes of the TYPES class, I check the back references to see if they contain references other than HasSubtype. If detected, skip adding such references to the resulting array
        // of the node in question, since the main parent references must be of this type. And all other references will be restored by the open62541 library.
        if (is_type_class_node) // If the node class being processed is a TYPE class, then...
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
                    node_references_req_res.at(node_index).exp_node_id.ToString());
                continue;
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_references_req_res.at(node_index).references.swap(references_after_filter);
}

inline void NodesetExporterLoop::AddStartNodeIfNotFound(
    size_t node_index,
    UA_NodeClass node_class,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res,
    bool& has_start_node_subtype_detected,
    uint64_t& start_node_reverse_reference_counter)
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

        // Дополнение, если стартовый узел является узлом класса ТИПОв.
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
    auto start_node_id_name = common::UaGuidIdentifierToStdString(node_attr_res_req.at(start_node_index).exp_node_id.GetRef().nodeId);
    const auto start_node_id_namepspace = node_attr_res_req.at(start_node_index).exp_node_id.GetRef().nodeId.namespaceIndex;
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


    m_logger.Info("The attributes and type reference for the start node '{}' in 'Flat Mode' have been created.", node_attr_res_req.at(start_node_index).exp_node_id.ToString());
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
        node_references_req_res.at(add_ref_to_node_by_index).exp_node_id.ToString(),
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

    for (const auto& ref_obj : node_references_req_res.at(node_index).references)
    {
        if (!ref_obj.GetRef().isForward)
        {
            // If nodes are classes of TYPES, then for such types ParentNodeID is refType = UA_NS0ID_HASSUBTYPE
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.5.2 - ObjectType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.6.5 - VariableType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.8.3 - DataType NodeClass (not specified, but visible from UANodeSet.xsd)
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.3 - ReferenceType NodeClass
            // todo Learn to work with HasSubtype subtypes and recognize their affiliation.
            if (m_types_nodeclasses.contains(node_class) && UA_NodeId_equal(&ref_obj.GetRef().referenceTypeId, &m_ns0id_hassubtype_node_id))
            {
                return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(ref_obj.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID));
            }

            // If the nodes are not TYPE classes (Instance classes). Parents can be indicated by various types of references.
            if (!m_types_nodeclasses.contains(node_class))
            {
                return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(ref_obj.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID));
            }
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
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_TYPES_EXPANDEDNODEID));
    case UA_NODECLASS_VARIABLETYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE), UA_TYPES_EXPANDEDNODEID));
    case UA_NODECLASS_REFERENCETYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), UA_TYPES_EXPANDEDNODEID));
    case UA_NODECLASS_DATATYPE:
        return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), UA_TYPES_EXPANDEDNODEID));
    default:
        throw(std::out_of_range("node_classes_req_res[index].node.class"));
    }
}

#pragma endregion Receiving and processing references


StatusResults NodesetExporterLoop::GetNamespaces(std::vector<std::string>& namespaces)
{
    m_logger.Trace("Method called: GetNamespaces()");
    // Read all server namespaces
    UATypesContainer<UA_ExpandedNodeId> server_namespace_array_request(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), UA_TYPES_EXPANDEDNODEID);
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
            namespaces.emplace_back(static_cast<char*>(static_cast<void*>(ua_namespace.data)), ua_namespace.length);
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
                if (const auto* const data_type_node_id = std::get_if<UATypesContainer<UA_NodeId>>(&datatype_attr.value()))
                {
                    // Save only if the datatype belongs to the OPC UA base space.
                    if (data_type_node_id->GetRef().namespaceIndex == 0)
                    {
                        auto alias_str = node_intermediate_obj.GetDataTypeAlias();
                        // Alias must be in only one instance
                        if (!aliases.contains(alias_str))
                        {
                            aliases.insert({alias_str, *data_type_node_id});
                        }
                    }
                }
                else
                {
                    m_logger.Critical("DATATYPE has wrong type in NodeID: {}", node_intermediate_obj.GetExpNodeId().ToString());
                    return StatusResults::Fail;
                }
            }
            catch (std::out_of_range&)
            {
                m_logger.Warning("DATATYPE attribute is missing from NodeID: {}", node_intermediate_obj.GetExpNodeId().ToString());
            }
        }

        // Add reference types as aliases
        for (const auto& ref : node_intermediate_obj.GetNodeReferenceTypeAliases())
        {
            if (ref.first.GetRef().referenceTypeId.namespaceIndex == 0)
            {
                // Alias must be in only one instance
                if (!aliases.contains(ref.second))
                {
                    aliases.insert({ref.second, UATypesContainer<UA_NodeId>(ref.first.GetRef().referenceTypeId, UA_TYPES_NODEID)});
                }
            }
        }
    }
    return StatusResults::Good;
}

inline StatusResults NodesetExporterLoop::GetNodeClasses(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<size_t, size_t>& node_range,
    std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res)
{
    m_logger.Trace("Method called: GetNodeClasses()");

    // todo I noticed that now it would be more convenient and faster to store the map from (NodeID|Class), since there is a need to obtain a class depending on the node.
    //  Doing a search cycle through an array takes a long time.
    std::copy(node_ids.begin() + static_cast<int64_t>(node_range.first), node_ids.begin() + static_cast<int64_t>(node_range.second), std::back_inserter(node_classes_req_res));
    const auto status = m_open62541_lib.ReadNodeClasses(node_classes_req_res); // REQUEST<-->RESPONSE

    if (node_classes_req_res.empty())
    {
        m_logger.Error("Unable to get node classes from server.");
        return status;
    }

    // In the case of operation in flat components, as well as the mode of creating the starting nodes, I denote the class of the created start node. Only the start node is processed and
    // just not part of the standard, in fact, the main search goes by the node not i=85.
    if (m_external_options.flat_list_of_nodes.is_enable && m_external_options.flat_list_of_nodes.create_missing_start_node && node_range.first == 0
        && !m_ns0_opcua_standard_node_ids.contains(node_classes_req_res.at(0).exp_node_id))
    {
        m_logger.Warning("NodeID '{}' is the 'Start Node' in 'Flat Mode' and will be created as an Object node class.", node_classes_req_res.at(0).exp_node_id.ToString());
        node_classes_req_res.at(0).node_class = UA_NodeClass::UA_NODECLASS_OBJECT;
        node_classes_req_res.at(0).result_code = UA_STATUSCODE_GOOD; // Так-как узла не существует в случае режима плоских узлов, то нужно игнорировать эту ошибку, а значит выставить хорошее значение.
    }

    return status;
}

// todo The method below is very huge and smeared, refactoring to break the method into individual entities, which are more clearly
//  could describe the process.
StatusResults NodesetExporterLoop::GetNodesData(
    const std::pair<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>& node_ids,
    const std::pair<size_t, size_t>& node_range,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<NodeIntermediateModel>& node_models)
{
    // todo Transfer filtration by class classes after receiving classes so as not to receive extra data, and filtering by Neymsums before receiving classes, but
    //  without unnecessary copies of the nodes.
    m_logger.Trace("Method called: GetNodesData()");

    // Log a list of nodes for export
    if (m_logger.IsEnable(LogLevel::Debug))
    {
        for (size_t index = node_range.first; index < node_range.second; ++index)
        {
            // To avoid constantly executing the loop and ToString before sending it to Debug, check the logging level in advance.
            m_logger.Debug("GetNodesData beginning. NodeID: {}, class: {}", node_ids.second.at(index).ToString(), static_cast<int>(node_classes_req_res.at(index).node_class));
        }
        m_logger.Debug("Total nodes: {}", node_range.second - node_range.first);
    }

    // Preparing the request and getting node attributes
    std::vector<IOpen62541::NodeAttributesRequestResponse> nodes_attr_req_res; // NODE ATTRIBUTES  (Attribute Service Set)
    if (GetNodeAttributes(node_ids.second, node_range, node_classes_req_res, nodes_attr_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Prepare a request and get a list of references for each node
    // todo Is it worth getting references of absolutely all nodes from the selection, or should those that are not currently being processed not be included in the list?
    std::vector<IOpen62541::NodeReferencesRequestResponse> node_references_req_res; // NODE REFERENCES (View Service Set)
    if (GetNodeReferences(node_ids.second, node_range, node_references_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Processing references for working with the KepServer server (and similar ones with similar features)
    if (KepServerRefFix(node_references_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }


#pragma region Analyze and package data into a container node_models
    // Working from the node_id list with offset
    for (size_t index = node_range.first; index < node_range.second; ++index)
    {
        auto index_from_zero = index - node_range.first;

        // Different filters of work with ns=0 nodes are used depending on the flag ns0_custom_nodes_ready_to_work
        if (m_external_options.ns0_custom_nodes_ready_to_work)
        {
            // Filtering of the nodes that are contained in the list of nodes from the OPC UA standard. I do not add such nodes to the list for unloading.
            // User nodes ns=0 are passed by a filter.
            if (m_ns0_opcua_standard_node_ids.contains(node_ids.second.at(index)))
            {
                m_logger.Warning("The node with id {} is IGNORED because this node is part of the standard OPC UA set.", node_ids.second.at(index).ToString());
                continue;
            }
        }
        else
        {
            // The filtering of all components with ns=0. I do not add such components to the list for unloading.
            if (node_ids.second.at(index).GetRef().nodeId.namespaceIndex == 0)
            {
                m_logger.Warning("The node with id {} is IGNORED because this node is from the OPC UA namespace", node_ids.second.at(index).ToString());
                continue;
            }
        }

        // Filter: filtering from creating non -export types of nodes by classes
        if (m_ignored_nodeclasses.contains(node_classes_req_res.at(index).node_class))
        {
            m_logger.Warning(
                "NodeID '{}' is IGNORED because this node has a NODE CLASS '{}' from the ignore list",
                node_ids.second.at(index).ToString(),
                m_ignored_nodeclasses.at(node_classes_req_res.at(index).node_class));
            continue;
        }

        // Depending on the mode of the formation of flat lists, filtering of reference removal is processed differently.
        if (m_external_options.flat_list_of_nodes.is_enable)
        {
            // Filter: 'Remove' all hierarchical references
            DeleteAllHierarhicalReferences(index_from_zero, node_references_req_res);
        }
        else
        {
            // Filter: 'Removing' broken references
            DeleteFailedReferences(index_from_zero, node_references_req_res);

            // Filter: In nodes of classes of type ReferenceTypes, DataTypes, ObjectTypes, VariableTypes 'Remove' back references other than the HasSubtype type.
            DeleteNotHasSubtypeReference(index_from_zero, node_classes_req_res.at(index).node_class, node_references_req_res);
        }

#pragma region Processing the start nodes and it references
        // Process the start node and its references
        bool has_start_node_subtype_detected = false;
        uint64_t start_node_reverse_reference_counter = 0;

        // The function of adding attributes to an artificially created start node
        if (m_external_options.flat_list_of_nodes.is_enable && m_external_options.flat_list_of_nodes.create_missing_start_node && index == 0)
        {
            CreateAttributesForStartNode(nodes_attr_req_res, node_references_req_res);
        }

        // The function of adding references in all subsequent nodes to an artificially borched start node or an existing node on the main
        // server as a source that will be like a start.
        if (m_external_options.flat_list_of_nodes.is_enable && index != 0)
        {
            AddCustomRefferenceToNodeID(node_ids.second.at(0), index_from_zero, UA_NS0ID_ORGANIZES, false, node_references_req_res);
        }

        // The function of processing references of starting nodes.
        // If the starting node does not have a binding to i=85, then such a node creates a reference to the one indicated in the variable parent_start_node_replacer.
        if (index == 0)
        {
            AddStartNodeIfNotFound(index_from_zero, node_classes_req_res.at(index).node_class, node_references_req_res, has_start_node_subtype_detected, start_node_reverse_reference_counter);
        }

        // Analysis and obtaining PARENTID
        // Obtaining PARENTID from reverse references. The first detected reverse type reference is taken. He does not return the parent if the type of type is a type and does not have a reverse
        // reference like UA_ns0id_hassubtype.
        std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> t_parent_node_id = GetParentNodeId(index_from_zero, node_classes_req_res.at(index).node_class, node_references_req_res);

        // If the starting node is of type HasSubtype and this type of node does not have any back reference to the main parent of the HasSubtype type (this can happen with starting nodes),
        // then add such a reference to the base super-type, depending on the class of the node.
        // I'll check if the main parent of the super-type remains, and if not, then I'll add a parent to the base super-type.
        if (has_start_node_subtype_detected && start_node_reverse_reference_counter == 0)
        {
            t_parent_node_id = GetBaseObjectType(node_classes_req_res.at(index).node_class);

            m_logger.Warning("The start Node has a node TYPE class without any HasSubtype reverse reference. Adding a new HasSubtype parent reference {}.", t_parent_node_id->ToString());
            UATypesContainer<UA_ReferenceDescription> insertion_ref_desc_main(UA_TYPES_REFERENCEDESCRIPTION);
            insertion_ref_desc_main.GetRef().isForward = false;
            UA_NodeId_copy(&m_ns0id_hassubtype_node_id, &insertion_ref_desc_main.GetRef().referenceTypeId);
            UA_NodeId_copy(&t_parent_node_id->GetRef().nodeId, &insertion_ref_desc_main.GetRef().nodeId.nodeId);
            node_references_req_res.at(index_from_zero).references.emplace(node_references_req_res.at(index_from_zero).references.end(), std::move(insertion_ref_desc_main));
        }

        // Filter: Analyze the node to determine if it belongs to the parent of the ignored type. If the parent was not found, then we should not add this node, because according to the hierarchy,
        // if such a parent refers to some nodes that were previously deleted, which means we should not add child nodes.
        if (!t_parent_node_id)
        {
            m_logger.Warning("The node with id {} is IGNORED because this node has a PARENT NODE with wrong NODE CLASS", node_ids.second.at(index).ToString());
            continue;
        }
#pragma endregion Processing the start nodes and it references

        m_logger.Debug("Filling NodeIntermediateModel...");
        NodeIntermediateModel nim;

        // NodeID
        nim.SetExpNodeId(node_ids.second.at(index).GetRef()); // Copy (must not change the source)

        // ParentNodeID
        if (t_parent_node_id == nullptr)
        {
            throw std::runtime_error("t_parent_node_id == nullptr");
        }
        nim.SetParentNodeId(std::move(*t_parent_node_id));

        // NodeClass
        nim.SetNodeClass(node_classes_req_res.at(index).node_class); // Copy

        // NodeReferences
        if (node_references_req_res[index_from_zero].references.empty())
        {
            throw std::runtime_error("node_references_req_res[index_from_zero].references.empty()");
        }
        nim.SetNodeReferences(std::move(node_references_req_res.at(index_from_zero).references)); // Перемещение

        // NodeAttributes
        nim.SetAttributes(std::move(nodes_attr_req_res.at(index_from_zero).attrs)); // Перемещение

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
#pragma endregion Analysis and packaging of data into the node_models container

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
        if (!fast_search_nodeid_ref_copy.contains(node_ids.at(index))) // сложность log(n)
        {
            after_distinct_node_ids.push_back(node_ids.at(index));
            fast_search_nodeid_ref_copy.insert(std::ref(after_distinct_node_ids.at(new_index)));
            ++new_index;
        }
        else
        {
            m_logger.Info("The found NodeID duplicate {} has been removed.", node_ids.at(index).ToString());
        }
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

#pragma endregion Data export methods

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
    for (auto& list_of_nodes_from_one_start_node : m_node_ids)
    {

#pragma region Node Filtering - Remove duplicates(all NodeIds are unique) and remove nodes from ns0
        RESET_TIMER(timer);
        // I move the finished copy of the set of nodes for quick search in the field for further actions.
        // For each iteration of the start node - its own set.
        m_node_ids_set_copy = Distinct(list_of_nodes_from_one_start_node.second);
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

        // todo Consider the option to remove the crushing according to the m_number_of_max_nodes_to_request_data parameter, as I do not take so much memory, and the difficulty of developing
        // increases.
        //  To realize crushing only at the level of OPC UA queries.
        if (list_of_nodes_from_one_start_node.second.size() <= m_number_of_max_nodes_to_request_data
            || m_number_of_max_nodes_to_request_data == 0) // If the nodes for export are less than the limit per single request
        {
#pragma region If the nodes for export are less than the limit per single request
            m_logger.Debug(
                "StartExport(), the condition worked: list_of_nodes_from_one_start_node.second.size() <= m_number_of_max_nodes_to_request_data || m_number_of_max_nodes_to_request_data == 0");
            std::vector<NodeIntermediateModel> node_intermediate_obj = std::vector<NodeIntermediateModel>();
            std::pair<size_t, size_t> range{0, list_of_nodes_from_one_start_node.second.size()}; // Full range of nodes

            RESET_TIMER(timer);
            // Get node classes
            if (GetNodeClasses(list_of_nodes_from_one_start_node.second, range, node_classes_req_res) == StatusResults::Fail)
            {
                return StatusResults{StatusResults::Fail, StatusResults::GetNodeClassesFail};
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodeClasses operation: ", "");

            if (list_of_nodes_from_one_start_node.second.size() != node_classes_req_res.size())
            {
                throw std::runtime_error("list_of_nodes_from_one_start_node.second.size() != node_classes_req_res.size()");
            }

            RESET_TIMER(timer);
            // Create a list of ignored nodes
            for (const auto& nodes : node_classes_req_res)
            {
                // Проверка на существование
                if (UA_StatusCode_isBad(nodes.result_code))
                {
                    m_logger.Error("Node '{}' returned a bad result in the node class query: {}", nodes.exp_node_id.ToString(), UA_StatusCode_name(nodes.result_code));
                    return StatusResults::Fail;
                }

                // Create a list of ignored nodes
                if (m_ignored_nodeclasses.contains(nodes.node_class))
                {
                    m_ignored_node_ids_by_classes.insert(nodes.exp_node_id);
                }
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Making the lists of the ignored nodes by classes: ", "");

            RESET_TIMER(timer);
            // Получение необходимых данных по узлам
            if (GetNodesData(list_of_nodes_from_one_start_node, range, node_classes_req_res, node_intermediate_obj) == StatusResults::Fail)
            {
                return StatusResults{StatusResults::Fail, StatusResults::GetNodesDataFail};
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodesData operation: ", "");

            // It may be that in the starting pack there will be one knot, which is eliminated, for example, a method, in the end
            // node_Intermediate_obj can be empty, but it will not be a mistake.
            if (node_intermediate_obj.empty())
            {
                m_logger.Debug("node_intermediate_obj is empty.");
                continue;
            }

            RESET_TIMER(timer);
            // Retrieving data by aliases of node types
            if (GetAliases(node_intermediate_obj, aliases) == StatusResults::Fail)
            {
                return {StatusResults::Fail, StatusResults::SubStatus::GetAliasesFail};
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetAliases operation: ", "");

            RESET_TIMER(timer);
            // Exporting Nodes
            if (ExportNodes(node_intermediate_obj) == StatusResults::Fail)
            {
                return {StatusResults::Fail, StatusResults::SubStatus::ExportNodesFail};
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNodes operation: ", "");
#pragma endregion If the nodes for export are less than the limit per single request
        }
        else // If there are more nodes for export than the limit for a single request
        {
#pragma region If the export nodes are larger than the limit for a single request
            m_logger.Debug("StartExport(), the condition worked: list_of_nodes_from_one_start_node.second.size() > m_number_of_max_nodes_to_request_data");

            // A local function that allows you to provide an algorithm for batch processing of data by working with ranges.
            // This function is used to run various routines where you need to work with NodeID, but with a certain number in one cycle.
            const auto func_in_nodes_loop =
                [&list_of_nodes_from_one_start_node, number_of_max_nodes_to_request_data = m_number_of_max_nodes_to_request_data](const std::function<StatusResults(std::pair<size_t, size_t>&)>& func)
            {
                std::pair<size_t, size_t> node_range;
                size_t number_of_nodes_per_request = 0;
                for (size_t index = 0; index < list_of_nodes_from_one_start_node.second.size(); index += number_of_nodes_per_request)
                {
                    number_of_nodes_per_request = list_of_nodes_from_one_start_node.second.size() - index >= number_of_max_nodes_to_request_data
                                                      ? number_of_max_nodes_to_request_data
                                                      : list_of_nodes_from_one_start_node.second.size() - index;
                    node_range.first = index;
                    node_range.second = node_range.first + number_of_nodes_per_request;

                    auto status = func(node_range);
                    if (status == StatusResults::Fail)
                    {
                        return status;
                    }
                };
                return StatusResults{StatusResults::Good, StatusResults::No};
            };

            // Batch retrieval of all node classes.
            const auto get_node_classes = [this, &list_of_nodes_from_one_start_node, &node_classes_req_res](const std::pair<size_t, size_t>& node_range)
            {
                auto timer = PREPARE_TIMER(m_external_options.is_perf_timer_enable);
                std::vector<IOpen62541::NodeClassesRequestResponse> part_of_node_classes_req_res;
                if (GetNodeClasses(list_of_nodes_from_one_start_node.second, node_range, part_of_node_classes_req_res) == StatusResults::Fail)
                {
                    return StatusResults::Fail;
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodeClasses operation: ", "");

                // Creating a list of ignored nodes
                RESET_TIMER(timer);
                for (const auto& nodes : part_of_node_classes_req_res)
                {
                    if (m_ignored_nodeclasses.contains(nodes.node_class))
                    {
                        m_ignored_node_ids_by_classes.insert(nodes.exp_node_id);
                    }
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Making the lists of the ignored nodes by classes: ", "");
                std::move(part_of_node_classes_req_res.begin(), part_of_node_classes_req_res.end(), std::back_inserter(node_classes_req_res));
                return StatusResults::Good;
            };

            // Batch retrieval of all other data and export
            const auto get_node_data_and_export = [this, &list_of_nodes_from_one_start_node, &node_classes_req_res, &aliases](const std::pair<size_t, size_t>& node_range)
            {
                auto timer = PREPARE_TIMER(m_external_options.is_perf_timer_enable);
                RESET_TIMER(timer);
                std::vector<NodeIntermediateModel> node_intermediate_obj;
                // Getting the data you need on the nodes
                if (GetNodesData(list_of_nodes_from_one_start_node, node_range, node_classes_req_res, node_intermediate_obj) == StatusResults::Fail)
                {
                    return StatusResults{StatusResults::Fail, StatusResults::GetNodesDataFail};
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodesData operation: ", "");

                // It may be that in the starting pack there will be one knot, which is eliminated, for example, a method, in the end
                // node_Intermediate_obj can be empty, but it will not be a mistake.
                if (!node_intermediate_obj.empty())
                {
                    // Retrieving Node Type Aliases
                    RESET_TIMER(timer);
                    if (GetAliases(node_intermediate_obj, aliases) == StatusResults::Fail)
                    {
                        return StatusResults{StatusResults::Fail, StatusResults::GetAliasesFail};
                    }
                    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetAliases and merge operation: ", "");

                    // Exporting Nodes
                    RESET_TIMER(timer);
                    if (ExportNodes(node_intermediate_obj) == StatusResults::Fail)
                    {
                        return StatusResults{StatusResults::Fail, StatusResults::ExportNodesFail};
                    }
                    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNodes operation: ", "");
                }
                else
                {
                    m_logger.Warning("node_intermediate_obj is empty.");
                }
                m_logger.Debug("End of node export step in loop");
                m_logger.Info("Part of exported nodes: {}", node_intermediate_obj.size());
                return StatusResults{StatusResults::Good, StatusResults::No};
            };

            //---------------- ACTION ----------------

            RESET_TIMER(timer);
            // You need to get all the classes before you start processing the rest of the data, because you filter nodes and references by node classes in the same way.
            if (func_in_nodes_loop(get_node_classes) == StatusResults::Fail)
            {
                return StatusResults{StatusResults::Fail, StatusResults::GetNodeClassesFail};
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "get_node_classes operation: ", "");

            if (list_of_nodes_from_one_start_node.second.size() != node_classes_req_res.size())
            {
                throw std::runtime_error("list_of_nodes_from_one_start_node.second.size() != node_classes_req_res.size()");
            }

            // IMPORTANT: Since node classes are also index-bound, but all classes have already been obtained in advance,
            // it is necessary to synchronize the indexes of the classes and other structures of index-dependent nodes!
            // Batch retrieval of all other data and export.
            RESET_TIMER(timer);
            const auto status = func_in_nodes_loop(get_node_data_and_export);
            if (status == StatusResults::Fail)
            {
                return status;
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "get_node_data_and_export operations: ", "");
#pragma endregion If the export nodes are larger than the limit for a single request
        }
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
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_ALARMGROUPMEMBER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_DATASETTOWRITER),
    CONSTRUCT_NUMERIC_NODE_ID_MAP_ITEM(UA_NS0ID_AGGREGATES)};

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