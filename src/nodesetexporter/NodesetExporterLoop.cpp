//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/NodesetExporterLoop.h"
#include "nodesetexporter/common/PerformanceTimer.h"

#include <open62541/types.h>

#include <functional>

// NOLINTBEGIN
#define CONSTRUCT_MAP_ITEM(key)                                                                                                                                                                        \
    {                                                                                                                                                                                                  \
        key, #key                                                                                                                                                                                      \
    }
// NOLINTEND

namespace nodesetexporter
{

using PerformanceTimer = nodesetexporter::common::PerformanceTimer;

#pragma region Methods for obtaining and generating data


#pragma region Getting ID attribute

StatusResults NodesetExporterLoop::GetNodeAttributes(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<int64_t, int64_t>& range_for_nodes,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res)
{

    for (size_t index = range_for_nodes.first; index < range_for_nodes.second; ++index)
    {
        auto attr = GetCommonNodeAttributes();
        switch (node_classes_req_res[index].node_class)
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
                "Get attributes of node class {} not implemented. Node ID: {}", m_ignored_nodeclasses.at(node_classes_req_res[index].node_class), node_classes_req_res[index].exp_node_id.ToString());
            attr.clear();
        }
        nodes_attr_req_res.push_back(IOpen62541::NodeAttributesRequestResponse{node_ids[index], attr});
    }
    // The OPC UA standard for receiving attributes guarantees - The size and order of this list matches the size and order of the nodesToReadrequest
    // parameter. https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2 I extend this rule to the library as well.
    if (m_open62541_lib.ReadNodesAttributes(nodes_attr_req_res) == StatusResults::Fail) // REQUEST<-->RESPONSE
    {
        return StatusResults::Fail;
    }
    if (range_for_nodes.second - range_for_nodes.first != nodes_attr_req_res.size())
    {
        throw std::runtime_error("range_for_nodes.second - range_for_nodes.first != nodes_attr_req_res.size()");
    }
    return StatusResults::Good;
}

#pragma endregion Getting ID attribute


#pragma region Receiving and processing reference

StatusResults NodesetExporterLoop::GetNodeReferences(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<int64_t, int64_t>& range_for_nodes,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    // Request to get references of all types for each node. Indexing references is the same as with attributes.
    std::copy(node_ids.begin() + range_for_nodes.first, node_ids.begin() + range_for_nodes.second, std::back_inserter(node_references_req_res));
    if (m_open62541_lib.ReadNodeReferences(node_references_req_res) == StatusResults::Fail) // REQUEST<-->RESPONSE
    {
        return StatusResults::Fail;
    }
    if (range_for_nodes.second - range_for_nodes.first != node_references_req_res.size())
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
        bool are_we_have_inverse_ref = false;
        bool are_we_found_base_variable_type = false;
        for (auto& ref : node_ref.references) // References
        {
            // For unknown reasons, in KepServer, nodes of the Variable class are set to HasTypeDefinition = BaseVariableType(62).
            // This abstract type cannot be used directly on nodes of this class. When importing nodesetloader we get an error.
            // In this case, the easiest option is to change HasTypeDefinition to a more specific, although still generic, but not abstract type BaseDataVariableType(63).
            if (UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_hastypedefenition_node_id) && UA_NodeId_equal(&ref.GetRef().nodeId.nodeId, &m_basevariabletype_node_id))
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
            UA_NodeId_copy(&m_hascomponent_node_id, &new_ref.GetRef().referenceTypeId);
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

inline void NodesetExporterLoop::DeleteFailedReference(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_references_req_res[node_index].references.size());
    for (auto& ref : node_references_req_res[node_index].references)
    {
        if (ref.GetRef().nodeId.nodeId.namespaceIndex != 0) // We do not filter references to ns=0
        {
            // Check for a reference to an ignored, known node
            UATypesContainer node_in_container(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
            if (m_ignored_node_ids.contains(node_in_container))
            {
                m_logger.Warning(
                    "The {} reference {} ==> {} is IGNORED because this node is deleted",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    node_references_req_res[node_index].exp_node_id.ToString(),
                    node_in_container.ToString());
                continue; // Don't add a reference
            }
            // Check for a reference to a missing node filtered in the external environment
            if (!m_node_ids_set_copy.contains(node_in_container))
            {
                m_logger.Warning(
                    "The {} reference {} ==> {} is IGNORED because this node is missing",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    node_references_req_res[node_index].exp_node_id.ToString(),
                    node_in_container.ToString());
                continue; // Do not add a reference
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_references_req_res[node_index].references = std::move(references_after_filter);
}

inline void NodesetExporterLoop::DeleteNotHasSubtypeReference(size_t node_index, UA_NodeClass node_class, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    std::vector<UATypesContainer<UA_ReferenceDescription>> references_after_filter;
    references_after_filter.reserve(node_references_req_res[node_index].references.size());
    bool is_type_class_node = m_types_nodeclasses.contains(node_class);
    for (auto& ref : node_references_req_res[node_index].references)
    {
        // In the nodes of the TYPES class, I check the back references to see if they contain references other than HasSubtype. If detected, skip adding such references to the resulting array
        // of the node in question, since the main parent references must be of this type. And all other references will be restored by the open62541 library.
        if (is_type_class_node) // If the node class being processed is a TYPE class, then...
        {
            // Looking for a back reference with a reference type other than HasSubtype, but ignoring a reference of type i=85, since it should already be in the right place.
            // It is logical to assume that isForward(False) only have hierarchical references (no clear confirmation found in the standard), which can protect against removal of
            // NON-hierarchical references.
            if (!ref.GetRef().isForward && !UA_NodeId_equal(&ref.GetRef().referenceTypeId, &m_hassubtype_node_id) && !UA_NodeId_equal(&ref.GetRef().nodeId.nodeId, &m_ns0id_objectfolder))
            {
                // If the referenceTypeID is some kind of custom type, then I will display its NodeID.
                // todo consider the option of requesting custom types from the server and outputting the BrowseName type.
                const auto reference_name_of_id = ref.GetRef().referenceTypeId.namespaceIndex == 0
                                                          && m_hierarhical_references.contains(ref.GetRef().referenceTypeId.identifier.numeric) // NOLINT(cppcoreguidelines-pro-type-union-access)
                                                      ? m_hierarhical_references.at(ref.GetRef().referenceTypeId.identifier.numeric) // NOLINT(cppcoreguidelines-pro-type-union-access)
                                                      : UATypesContainer(ref.GetRef().referenceTypeId, UA_TYPES_NODEID).ToString();
                m_logger.Warning(
                    "Found {} ReferenceType=\"{}\"  ==> '{}' in class node {} with NodeID '{}'. Since we only need the HasSubtype inverse reference type in this node class, I`m "
                    "removing this reference.",
                    ref.GetRef().isForward ? "forward" : "reverse",
                    reference_name_of_id,
                    UATypesContainer<UA_ExpandedNodeId>(ref.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID).ToString(),
                    m_types_nodeclasses.at(node_class),
                    node_references_req_res[node_index].exp_node_id.ToString());
                continue;
            }
        }
        references_after_filter.emplace_back(std::move(ref));
    }
    node_references_req_res[node_index].references = std::move(references_after_filter);
}

inline void NodesetExporterLoop::AddStartNodeIfNotFound(
    size_t node_index,
    UA_NodeClass node_class,
    std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res,
    bool& has_start_node_subtype_detected,
    uint64_t& start_node_reverse_reference_counter)
{
    start_node_reverse_reference_counter = 0;
    has_start_node_subtype_detected = false;
    // Get the first NodeReference with type Hierarchical inverse
    bool is_found_i85 = false; // Node search flag i=85
    for (auto& ref_obj : node_references_req_res[node_index].references)
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
        // Add a parent to type i=85 to bind to Object.
        m_logger.Warning("Adding a new Object parent reference '{}' to StartNode.", m_parent_start_node_replacer.ToString());
        UATypesContainer<UA_ReferenceDescription> insertion_ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
        insertion_ref_desc.GetRef().isForward = false;
        auto organize_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_NodeId_copy(&organize_node_id, &insertion_ref_desc.GetRef().referenceTypeId);
        UA_NodeId_copy(&m_parent_start_node_replacer.GetRef().nodeId, &insertion_ref_desc.GetRef().nodeId.nodeId);
        node_references_req_res[node_index].references.emplace(node_references_req_res[node_index].references.begin(), std::move(insertion_ref_desc));

        // Addition if the starting node is a node of the class TYPES.
        if (m_types_nodeclasses.contains(node_class))
        {
            // If the starting node is the class of the TYPES node, then we mark the presence of such a starting node for further actions later.
            has_start_node_subtype_detected = true;
        }
    }
}

inline std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> NodesetExporterLoop::GetParentNodeId(
    size_t node_index,
    UA_NodeClass node_class,
    const std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res)
{
    for (const auto& ref_obj : node_references_req_res[node_index].references)
    {
        if (!ref_obj.GetRef().isForward)
        {
            // If nodes are classes of TYPES, then for such types ParentNodeID is refType = UA_NS0ID_HASSUBTYPE
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.5.2 - ObjectType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.6.5 - VariableType NodeClass
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.8.3 - DataType NodeClass (not specified, but visible from UANodeSet.xsd)
            // https://reference.opcfoundation.org/Core/Part3/v104/docs/5.3 - ReferenceType NodeClass
            // todo Learn to work with HasSubtype subtypes and recognize their affiliation.
            if (m_types_nodeclasses.contains(node_class) && UA_NodeId_equal(&ref_obj.GetRef().referenceTypeId, &m_hassubtype_node_id))
            {
                return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(
                    UATypesContainer<UA_ExpandedNodeId>(ref_obj.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID)); // Found the first parent reference - exited the loop
            }

            // If the nodes are not TYPE classes (Instance classes). Parents can be indicated by various types of references.
            if (!m_types_nodeclasses.contains(node_class))
            {
                return std::make_unique<UATypesContainer<UA_ExpandedNodeId>>(
                    UATypesContainer<UA_ExpandedNodeId>(ref_obj.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID)); // Found the first parent reference - exited the loop
            }
        }
    }
    return nullptr;
}

inline std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> NodesetExporterLoop::GetBaseObjectType(UA_NodeClass node_class)
{
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
                if (const auto* const data_type_node_id = std::get_if<UATypesContainer<UA_NodeId>>(&node_intermediate_obj.GetAttributes().at(UA_AttributeId::UA_ATTRIBUTEID_DATATYPE).value()))
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
                    m_logger.Critical("DATATYPE has wrong type in NodeID : {}", node_intermediate_obj.GetExpNodeId().ToString());
                    return StatusResults::Fail;
                }
            }
            catch (std::out_of_range&)
            {
                m_logger.Warning("DATATYPE empty in NodeID : {}", node_intermediate_obj.GetExpNodeId().ToString());
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
    const std::pair<int64_t, int64_t>& range_for_nodes,
    std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res)
{
    // todo I noticed that now it would be more convenient and faster to store the map from (NodeID|Class), since there is a need to obtain a class depending on the node.
    //  Doing a search cycle through an array takes a long time.
    std::copy(node_ids.begin() + range_for_nodes.first, node_ids.begin() + range_for_nodes.second, std::back_inserter(node_classes_req_res));
    return m_open62541_lib.ReadNodeClasses(node_classes_req_res); // REQUEST<-->RESPONSE
}

StatusResults NodesetExporterLoop::GetNodesData(
    const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
    const std::pair<int64_t, int64_t>& range_for_nodes,
    const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
    std::vector<NodeIntermediateModel>& node_models)
{
    m_logger.Trace("Method called: GetNodesData()");

    // Log a list of nodes for export
    if (m_logger.IsEnable(LogLevel::Debug))
    {
        for (size_t index = range_for_nodes.first; index < range_for_nodes.second; ++index)
        {
            // To avoid constantly executing the loop and ToString before sending it to Debug, check the logging level in advance.
            m_logger.Debug("Node export (ext_node_ids). Before filter : {}, class: {}", node_ids[index].ToString(), node_classes_req_res[index].node_class);
        }
        m_logger.Debug("Total nodes: {}", range_for_nodes.second - range_for_nodes.first);
    }

    // Preparing the request and getting node attributes
    std::vector<IOpen62541::NodeAttributesRequestResponse> nodes_attr_req_res; // NODE ATTRIBUTES  (Attribute Service Set)
    if (GetNodeAttributes(node_ids, range_for_nodes, node_classes_req_res, nodes_attr_req_res) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    // Prepare a request and get a list of references for each node
    // todo Is it worth getting references of absolutely all nodes from the selection, or should those that are not currently being processed not be included in the list?
    std::vector<IOpen62541::NodeReferencesRequestResponse> node_references_req_res; // NODE REFERENCES (View Service Set)
    if (GetNodeReferences(node_ids, range_for_nodes, node_references_req_res) == StatusResults::Fail)
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
    for (size_t index = range_for_nodes.first; index < range_for_nodes.second; ++index)
    {
        auto index_from_zero = index - range_for_nodes.first;
        // Filter: Filtering from creating non-exportable node types by class
        if (m_ignored_nodeclasses.contains(node_classes_req_res[index].node_class))
        {
            m_logger
                .Warning("The node with id {} is IGNORED because this node has the wrong NODE CLASS: {}", node_ids[index].ToString(), m_ignored_nodeclasses.at(node_classes_req_res[index].node_class));
            continue;
        }

        // Filter: 'Removing' broken references
        DeleteFailedReference(index_from_zero, node_references_req_res);

        // Filter: In nodes of classes of type ReferenceTypes, DataTypes, ObjectTypes, VariableTypes 'Remove' back references other than the HasSubtype type.
        DeleteNotHasSubtypeReference(index_from_zero, node_classes_req_res[index].node_class, node_references_req_res);

        // Process the start node and its references
        bool has_start_node_subtype_detected = false;
        uint64_t start_node_reverse_reference_counter = 0;
        if (index == 0 && !UA_NodeId_equal(&node_ids[index].GetRef().nodeId, &m_ns0id_objectfolder))
        {
            AddStartNodeIfNotFound(index_from_zero, node_classes_req_res[index].node_class, node_references_req_res, has_start_node_subtype_detected, start_node_reverse_reference_counter);
        }

        // Parsing and getting ParentID
        // Get the ParentID from the back reference. The first reverse reference found is taken. Does not return the parent if the node is of class TYPE and does not have a back reference of type
        // UA_NS0ID_HASSUBTYPE.
        std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> t_parent_node_id = GetParentNodeId(index_from_zero, node_classes_req_res[index].node_class, node_references_req_res);

        // If the starting node is of type HasSubtype and this type of node does not have any back reference to the main parent of the HasSubtype type (this can happen with starting nodes),
        // then add such a reference to the base super-type, depending on the class of the node.
        // I'll check if the main parent of the super-type remains, and if not, then I'll add a parent to the base super-type.
        if (has_start_node_subtype_detected && start_node_reverse_reference_counter == 0)
        {
            t_parent_node_id = GetBaseObjectType(node_classes_req_res[index].node_class);

            m_logger.Warning("The start Node has a node TYPE class without any HasSubtype reverse reference. Adding a new HasSubtype parent reference {}.", t_parent_node_id->ToString());
            UATypesContainer<UA_ReferenceDescription> insertion_ref_desc_main(UA_TYPES_REFERENCEDESCRIPTION);
            insertion_ref_desc_main.GetRef().isForward = false;
            UA_NodeId_copy(&m_hassubtype_node_id, &insertion_ref_desc_main.GetRef().referenceTypeId);
            UA_NodeId_copy(&t_parent_node_id->GetRef().nodeId, &insertion_ref_desc_main.GetRef().nodeId.nodeId);
            node_references_req_res[index_from_zero].references.emplace(node_references_req_res[index_from_zero].references.end(), std::move(insertion_ref_desc_main));
        }

        // Filter: Analyze the node to determine if it belongs to the parent of the ignored type. If the parent was not found, then we should not add this node, because according to the hierarchy,
        // if such a parent refers to some nodes that were previously deleted, which means we should not add child nodes.
        if (!t_parent_node_id)
        {
            m_logger.Warning("The node with id {} is IGNORED because this node has a PARENT NODE with wrong NODE CLASS", node_ids[index].ToString());
            continue;
        }

        m_logger.Debug("Filling NodeIntermediateModel...");
        NodeIntermediateModel nim;

        // NodeID
        nim.SetExpNodeId(node_ids[index].GetRef()); // Copy (must not change the source)

        // ParentNodeID
        if (t_parent_node_id == nullptr)
        {
            throw std::runtime_error("t_parent_node_id == nullptr");
        }
        nim.SetParentNodeId(std::move(*t_parent_node_id));

        // NodeClass
        nim.SetNodeClass(node_classes_req_res[index].node_class); // Copy

        // NodeReferences
        if (node_references_req_res[index_from_zero].references.empty())
        {
            throw std::runtime_error("node_references_req_res[index_from_zero].references.empty()");
        }
        nim.SetNodeReferences(std::move(node_references_req_res[index_from_zero].references)); // Move

        // NodeAttributes
        nim.SetAttributes(std::move(nodes_attr_req_res[index_from_zero].attrs)); // Move

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

#pragma endregion Methods for obtaining and generating data

#pragma region Data export methods
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
            m_logger.Debug("Node: {}, node class: {}", node_model.GetExpNodeId().ToString(), node_model.GetNodeClass());
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
            m_logger.Warning("NODECLASS with define {} not undefined", node_model.GetNodeClass());
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

    if (m_node_ids.empty())
    {
        m_logger.Error("The list of node IDs is empty.");
        return StatusResults::Fail;
    }

    // Check for ns=0 in starting nodes. It is better to do this in a separate cycle before starting longer processing.
    // https://reference.opcfoundation.org/DI/v102/docs/11.2#_Ref252866620
    // https://documentation.unified-automation.com/uasdknet/3.0.10/html/L2UaAdrSpaceConceptNamespaces.html
    for (const auto& list_of_nodes_from_one_start_node : m_node_ids)
    {
        if (list_of_nodes_from_one_start_node.second.at(0).GetRef().nodeId.namespaceIndex == 0)
        {
            m_logger.Error("First NodeId (Start NodeId) - '{}' has ns = 0", list_of_nodes_from_one_start_node.first);
            return StatusResults::Fail;
        }
    }

    auto timer = PREPARE_TIMER(m_is_perf_timer_enable);
    // Actions before starting export
    if (Begin() == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Begin operation: ", "");

    // Get namespace data and export
    RESET_TIMER(timer);
    std::vector<std::string> namespaces;
    if (GetNamespaces(namespaces) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNamespaces operation: ", "");

    RESET_TIMER(timer);
    if (ExportNamespaces(namespaces) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNamespaces operation: ", "");

    std::map<std::string, UATypesContainer<UA_NodeId>> aliases;
    for (auto& list_of_nodes_from_one_start_node : m_node_ids)
    {

#pragma region Node Filtering - Remove duplicates(all NodeIds are unique) and remove nodes from ns0
        RESET_TIMER(timer);
        std::vector<ExpandedNodeId> after_distinct_node_ids;
        // Use Set to find nodes faster using the red-black tree algorithm. To reduce memory costs, the set stores pointers, but sorting and searching for nodes occurs with real node objects bound
        // to the after_distinct_node_ids list through the reference_wrapper.
        std::set<std::reference_wrapper<ExpandedNodeId>, std::less<ExpandedNodeId>> fast_search_nodeid_ref_copy; // NOLINT
        after_distinct_node_ids.reserve(list_of_nodes_from_one_start_node.second.size());
        // Primary initialization of storages
        after_distinct_node_ids.push_back(*list_of_nodes_from_one_start_node.second.begin());
        fast_search_nodeid_ref_copy.insert(std::ref(*after_distinct_node_ids.begin()));

        size_t new_index = 1;
        // Distinct algorithm with complexity n * log(n)
        for (size_t index = 1; index < list_of_nodes_from_one_start_node.second.size(); ++index) // Complexity N
        {
            if (!fast_search_nodeid_ref_copy.contains(list_of_nodes_from_one_start_node.second.at(index))) // Complexity log(n)
            {
                // Filtering nodes with ns = 0.  Do not add such nodes to the list for unloading, since these are basic nodes.
                if (list_of_nodes_from_one_start_node.second.at(index).GetRef().nodeId.namespaceIndex == 0)
                {
                    m_logger.Warning("The node with id {} is IGNORED because this node is from the OPC UA namespace", list_of_nodes_from_one_start_node.second.at(index).ToString());
                }
                else
                {
                    after_distinct_node_ids.push_back(list_of_nodes_from_one_start_node.second.at(index));
                    fast_search_nodeid_ref_copy.insert(std::ref(after_distinct_node_ids.at(new_index)));
                    ++new_index;
                }
            }
            else
            {
                m_logger.Info("The found NodeID duplicate {} has been removed.", list_of_nodes_from_one_start_node.second.at(index).ToString());
            }
        }
#pragma endregion Node Filtering - Remove duplicates(all NodeIds are unique) and remove nodes from ns0

        GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Distinct operation with ns=0 filtering: ", "");

        // Bind a reference to a new container.
        list_of_nodes_from_one_start_node.second = std::move(after_distinct_node_ids);
        // Move the finished copy of the node set for quick search to the field for further actions.
        // Each iteration of the starting node has its own set.
        m_node_ids_set_copy = std::move(fast_search_nodeid_ref_copy);

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

        if (list_of_nodes_from_one_start_node.second.size() <= m_number_of_max_nodes_to_request_data
            || m_number_of_max_nodes_to_request_data == 0) // If the nodes for export are less than the limit per single request
        {
            m_logger.Debug(
                "StartExport(), the condition worked: list_of_nodes_from_one_start_node.second.size() <= m_number_of_max_nodes_to_request_data || m_number_of_max_nodes_to_request_data == 0");
            std::vector<NodeIntermediateModel> node_intermediate_obj = std::vector<NodeIntermediateModel>();
            std::pair<int64_t, int64_t> range{0, list_of_nodes_from_one_start_node.second.size()}; // Full range of nodes

            RESET_TIMER(timer);
            // Get node classes
            if (GetNodeClasses(list_of_nodes_from_one_start_node.second, range, node_classes_req_res) == StatusResults::Fail)
            {
                return StatusResults::Fail;
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
                if (m_ignored_nodeclasses.contains(nodes.node_class))
                {
                    m_ignored_node_ids.insert(nodes.exp_node_id);
                }
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Making the lists of the ignored nodes: ", "");

            RESET_TIMER(timer);
            // Obtaining the necessary data for nodes
            if (GetNodesData(list_of_nodes_from_one_start_node.second, range, node_classes_req_res, node_intermediate_obj) == StatusResults::Fail)
            {
                return StatusResults::Fail;
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodesData operation: ", "");

            RESET_TIMER(timer);
            // Retrieving data by aliases of node types
            if (GetAliases(node_intermediate_obj, aliases) == StatusResults::Fail)
            {
                return StatusResults::Fail;
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetAliases operation: ", "");

            RESET_TIMER(timer);
            // Exporting Nodes
            if (ExportNodes(node_intermediate_obj) == StatusResults::Fail)
            {
                return StatusResults::Fail;
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNodes operation: ", "");
        }
        else // If there are more nodes for export than the limit for a single request
        {
            m_logger.Debug("StartExport(), the condition worked: list_of_nodes_from_one_start_node.second.size() > m_number_of_max_nodes_to_request_data");

            // A local function that allows you to provide an algorithm for batch processing of data by working with ranges.
            // This function is used to run various routines where you need to work with NodeID, but with a certain number in one cycle.
            const auto func_in_nodes_loop = [&list_of_nodes_from_one_start_node,
                                             number_of_max_nodes_to_request_data = m_number_of_max_nodes_to_request_data](const std::function<StatusResults(std::pair<int64_t, int64_t>&)>& func)
            {
                std::pair<int64_t, int64_t> range;
                size_t number_of_nodes_per_request = 0;
                for (size_t index = 0; index < list_of_nodes_from_one_start_node.second.size(); index += number_of_nodes_per_request)
                {
                    number_of_nodes_per_request = list_of_nodes_from_one_start_node.second.size() - index >= number_of_max_nodes_to_request_data
                                                      ? number_of_max_nodes_to_request_data
                                                      : list_of_nodes_from_one_start_node.second.size() - index;
                    range.first = static_cast<int64_t>(index);
                    range.second = range.first + static_cast<int64_t>(number_of_nodes_per_request);

                    if (func(range) == StatusResults::Fail)
                    {
                        return StatusResults::Fail;
                    }
                };
                return StatusResults::Good;
            };

            // Batch retrieval of all node classes.
            const auto get_node_classes = [this, &list_of_nodes_from_one_start_node, &node_classes_req_res](const std::pair<int64_t, int64_t>& range)
            {
                auto timer = PREPARE_TIMER(m_is_perf_timer_enable);
                std::vector<IOpen62541::NodeClassesRequestResponse> part_of_node_classes_req_res;
                if (GetNodeClasses(list_of_nodes_from_one_start_node.second, range, part_of_node_classes_req_res) == StatusResults::Fail)
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
                        m_ignored_node_ids.insert(nodes.exp_node_id);
                    }
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "Making the lists of the ignored nodes: ", "");
                std::move(part_of_node_classes_req_res.begin(), part_of_node_classes_req_res.end(), std::back_inserter(node_classes_req_res));
                return StatusResults::Good;
            };

            // Batch retrieval of all other data and export
            const auto get_node_data_and_export = [this, &list_of_nodes_from_one_start_node, &node_classes_req_res, &aliases](const std::pair<int64_t, int64_t>& range)
            {
                auto timer = PREPARE_TIMER(m_is_perf_timer_enable);
                RESET_TIMER(timer);
                std::vector<NodeIntermediateModel> node_intermediate_obj;
                // Getting the data you need on the nodes
                if (GetNodesData(list_of_nodes_from_one_start_node.second, range, node_classes_req_res, node_intermediate_obj) == StatusResults::Fail)
                {
                    return StatusResults::Fail;
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetNodesData operation: ", "");

                // Retrieving Node Type Aliases
                RESET_TIMER(timer);
                if (GetAliases(node_intermediate_obj, aliases) == StatusResults::Fail)
                {
                    return StatusResults::Fail;
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "GetAliases and merge operation: ", "");

                // Exporting Nodes
                RESET_TIMER(timer);
                if (ExportNodes(node_intermediate_obj) == StatusResults::Fail)
                {
                    return StatusResults::Fail;
                }
                GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportNodes operation: ", "");

                m_logger.Debug("End of node export step in loop");
                return StatusResults::Good;
            };

            //---------------- ACTION ----------------

            RESET_TIMER(timer);
            // You need to get all the classes before you start processing the rest of the data, because you filter nodes and references by node classes in the same way.
            if (func_in_nodes_loop(get_node_classes) == StatusResults::Fail)
            {
                return StatusResults::Fail;
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
            if (func_in_nodes_loop(get_node_data_and_export) == StatusResults::Fail)
            {
                return StatusResults::Fail;
            }
            GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "get_node_data_and_export operations: ", "");
        }
    }

    RESET_TIMER(timer);
    // Exporting host type name aliases
    if (ExportAliases(aliases) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "ExportAliases operation: ", "");

    RESET_TIMER(timer);
    // Actions at the end of the export - uploading to a buffer or to a file
    if (End() == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }
    GET_TIME_ELAPSED_FMT_FORMAT(timer, m_logger.Info, "End operation: ", "");
    m_logger.Info("Exported statistic:\n{}", m_exported_nodes.ToString());
    m_logger.Info("Total exported nodes: {}", m_exported_nodes.GetSumm());
    return StatusResults::Good;
}


const std::map<std::uint32_t, std::string> NodesetExporterLoop::m_hierarhical_references{
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HIERARCHICALREFERENCES),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASCHILD),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_ORGANIZES),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASEVENTSOURCE),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_AGGREGATES),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASSUBTYPE),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASPROPERTY),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASCOMPONENT),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASNOTIFIER),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASORDEREDCOMPONENT),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_CONTROLS),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_DATASETTOWRITER),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_AGGREGATES),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASREFERENCEDESCRIPTION),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASLOWERLAYERINTERFACE),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_HASPUSHEDSECURITYGROUP),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_ALARMSUPPRESSIONGROUPMEMBER),
    //    CONSTRUCT_MAP_ITEM(UA_NS0ID_REQUIRES),
    CONSTRUCT_MAP_ITEM(UA_NS0ID_ALARMGROUPMEMBER)};

const std::map<std::uint32_t, std::string> NodesetExporterLoop::
    m_ignored_nodeclasses{CONSTRUCT_MAP_ITEM(UA_NODECLASS_UNSPECIFIED), CONSTRUCT_MAP_ITEM(UA_NODECLASS_METHOD), CONSTRUCT_MAP_ITEM(UA_NODECLASS_VIEW), CONSTRUCT_MAP_ITEM(__UA_NODECLASS_FORCE32BIT)};

const std::map<std::uint32_t, std::string> NodesetExporterLoop::m_types_nodeclasses{
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_OBJECTTYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_REFERENCETYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_DATATYPE),
    CONSTRUCT_MAP_ITEM(UA_NODECLASS_VARIABLETYPE)};

} // namespace nodesetexporter