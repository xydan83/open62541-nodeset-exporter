//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_NODEINTERMEDIATEMODEL_H
#define NODESETEXPORTER_OPEN62541_NODEINTERMEDIATEMODEL_H

#include "nodesetexporter/common/DatatypeAliases.h"
#include "nodesetexporter/open62541/TypeAliases.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/types_generated_handling.h>

#include <map>
#include <optional>
#include <vector>

// todo Consider the idea of removing direct work with attributes in the NodeIntermediateModel and replacing it with Set/Get access methods, and storing the attributes themselves in fields.
//  This way, we get rid of the need to know about the attributes of users of objects of this class, but still, the kernel will have to know about the attributes,
//  since it will form a request from the server, as well as fill out NodeIntermediateModel objects.

namespace nodesetexporter::open62541
{

using nodesetexporter::datatypealiases::data_type_aliases;
using ::nodesetexporter::datatypealiases::reference_type_aliases;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttr;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttrToString;

/**
 * @brief An intermediate data model representing the necessary information to describe a node.
 * Used to saturate the kernel with the necessary data and then pass it on to the functionality for exporting to a specific data format.
 * @warning Does not support changes to added data.
 * @attention Will RVO or NRVO be applied when returning fields? Most likely not, since the value of an object’s fields cannot, in theory, be changed,
 *            so if you try to return objects and mark the Get methods as const, there is a high probability that copying will be applied.
 *            Therefore, using Get methods I return a reference, which risks changing the contents of the object.
 */
class NodeIntermediateModel
{
public:
    NodeIntermediateModel()
        : m_node_id(UATypesContainer<UA_ExpandedNodeId>(UA_TYPES_EXPANDEDNODEID))
        , m_parent_node_id(UATypesContainer<UA_ExpandedNodeId>(UA_TYPES_EXPANDEDNODEID))
    {
    }

    NodeIntermediateModel(const UA_ExpandedNodeId& node_id, const UA_ExpandedNodeId& parent_node_id, UA_NodeClass node_class) // NOLINT(bugprone-easily-swappable-parameters)
        : m_node_id(UATypesContainer<UA_ExpandedNodeId>(node_id, UA_TYPES_EXPANDEDNODEID))
        , m_parent_node_id(UATypesContainer<UA_ExpandedNodeId>(parent_node_id, UA_TYPES_EXPANDEDNODEID))
        , m_node_class(node_class)
    {
    }

    /**
     * @brief Copies the contents of the ExpandedNodeId of the Open62541 library object, simultaneously wrapping it in a UATypesContainer container.
     * @param node_id The ID of the node that the model represents.
     */
    void SetExpNodeId(const UA_ExpandedNodeId& node_id)
    {
        m_node_id = std::move(UATypesContainer<UA_ExpandedNodeId>(node_id, UA_TYPES_EXPANDEDNODEID));
    }

    /**
     * @brief Moves the ExpandedNodeId wrapped in UATypesContainer inside the container.
     * @param node_id The ID of the node that the model represents.
     */
    void SetExpNodeId(UATypesContainer<UA_ExpandedNodeId>&& node_id)
    {
        m_node_id = std::move(node_id);
    }

    /**
     * @brief Copies the ExpandedNodeId wrapped in UATypesContainer inside the container.
     * @param node_id The ID of the node that the model represents.
     */
    void SetExpNodeId(const UATypesContainer<UA_ExpandedNodeId>& node_id)
    {
        m_node_id = node_id;
    }

    /**
     * @brief Copies the parent ExpandedNodeId of the Open62541 library object, simultaneously wrapping it in a UATypesContainer container.
     * @param parent_node_id ID of the parent node
     */
    void SetParentNodeId(const UA_ExpandedNodeId& parent_node_id)
    {
        m_parent_node_id = std::move(UATypesContainer<UA_ExpandedNodeId>(parent_node_id, UA_TYPES_EXPANDEDNODEID));
    }

    /**
     * @brief Moves the parent ExpandedNodeId wrapped in UATypesContainer inside the container.
     * @param parent_node_id ID of the parent node
     */
    void SetParentNodeId(UATypesContainer<UA_ExpandedNodeId>&& parent_node_id)
    {
        m_parent_node_id = std::move(parent_node_id);
    }

    /**
     * @brief Copies the parent ExpandedNodeId wrapped in UATypesContainer inside the container.
     * @param parent_node_id ID of the parent node
     */
    void SetParentNodeId(const UATypesContainer<UA_ExpandedNodeId>& parent_node_id)
    {
        m_parent_node_id = parent_node_id;
    }

    /**
     * @brief Assigns a node class to the model
     * @param node_class The class of the node that the model represents.
     */
    void SetNodeClass(UA_NodeClass node_class)
    {
        m_node_class = node_class;
    }

    /**
     * @brief Copies a list of pointers to a reference object to other nodes of the Open62541 library object, simultaneously wrapping it in a UATypesContainer container.
     * @attention Typically, Open62541 library UA_xxx objects are created on the heap via the UA_xxxx_new methods, and deleted via UA_xxx_delete,
     *            so they are passed as pointers to objects. Since many structures include pointers inside to other objects, it is not recommended to create such objects on the stack
     *            to avoid memory leaks or errors when trying to delete them through delete functions.
     * @param references Links (forward-backward) associated with the node described by the model. Direct - when a given node refers to other nodes, reverse - when other nodes refer to a given node.
     *                   Typically the first node in the list is the ParentID.
     */
    void SetNodeReferences(const std::vector<UA_ReferenceDescription*>& references)
    {
        for (const auto& ref : references)
        {
            m_references.emplace_back(*ref, UA_TYPES_REFERENCEDESCRIPTION);
        }
    }

    /**
     * @brief Moves a list of references to other nodes wrapped in a UATypesContainer inside the container.
     * @param references Links (forward-backward) associated with the node described by the model. Direct - when a given node refers to other nodes,
     * reverse - when other nodes refer to this node. Typically the first node in the list is the ParentID.
     */
    void SetNodeReferences(std::vector<UATypesContainer<UA_ReferenceDescription>>&& references)
    {
        m_references = std::move(references);
    }

    /**
     * @brief Copies a list of references to other nodes wrapped in UATypesContainer inside the container.
     * @param references Links (forward-backward) associated with the node described by the model. Direct - when a given node refers to other nodes,
     * reverse - when other nodes refer to this node. Typically the first node in the list is the ParentID.
     */
    void SetNodeReferences(const std::vector<UATypesContainer<UA_ReferenceDescription>>& references)
    {
        m_references = references;
    }

    /**
     * @brief Moves an associative container containing node attributes and their values.
     * @param attributes List of attributes of the node described by the model.
     */
    void SetAttributes(std::map<UA_AttributeId, std::optional<VariantsOfAttr>>&& attributes)
    {
        m_attributes = std::move(attributes);
    }

    /**
     * @brief Copies an associative container with node attributes and their values.
     * @param attributes List of attributes of the node described by the model.
     */
    void SetAttributes(const std::map<UA_AttributeId, std::optional<VariantsOfAttr>>& attributes)
    {
        m_attributes = attributes;
    }

    /**
     * @brief Returns the model's NodeID reference.
     */
    [[nodiscard]] const UATypesContainer<UA_ExpandedNodeId>& GetExpNodeId() const
    {
        return m_node_id;
    }

    /**
     * @brief Returns the NodeID reference of the parent node.
     */
    [[nodiscard]] const UATypesContainer<UA_ExpandedNodeId>& GetParentNodeId() const
    {
        return m_parent_node_id;
    }

    /**
     * @brief Returns the model node class.
     */
    [[nodiscard]] UA_NodeClass GetNodeClass() const
    {
        return m_node_class;
    }

    /**
     * @brief Returns a references to a list of objects describing references to other nodes in the model.
     */
    [[nodiscard]] const std::vector<UATypesContainer<UA_ReferenceDescription>>& GetNodeReferences() const
    {
        return m_references;
    }

    /**
     * @brief Returns the model node attributes
     * @return A reference to a dictionary, where the key is the attribute ID and the value is the attribute variable.
     */
    [[nodiscard]] const std::map<UA_AttributeId, std::optional<VariantsOfAttr>>& GetAttributes() const
    {
        return m_attributes;
    }

    /**
     * @brief Returns a text description of the data type being stored. Valid only for Variable and VariableType nodes.
     * @return Alias for the data types that the node stores. If the data type is not found, an empty string object will be returned. Currently only standard data types are supported.
     * Custom types will be returned as a text description of the NodeID, for example: ns=1;i=2.
     * @warning When the function is called, the NodeIntermediateModel class object must have the UA_ATTRIBUTEID_DATATYPE attribute value set, otherwise an empty string will be returned.
     */
    [[nodiscard]] std::string GetDataTypeAlias() const
    {
        if (m_node_class != UA_NodeClass::UA_NODECLASS_VARIABLE && m_node_class != UA_NodeClass::UA_NODECLASS_VARIABLETYPE || m_attributes.empty())
        {
            return "";
        }

        if (!m_attributes.contains(UA_AttributeId::UA_ATTRIBUTEID_DATATYPE))
        {
            return "";
        }
        auto data_type_optional_variant = m_attributes.at(UA_AttributeId::UA_ATTRIBUTEID_DATATYPE);
        if (!data_type_optional_variant.has_value())
        {
            return "";
        }

        auto data_type_node_id = std::get<UATypesContainer<UA_NodeId>>(data_type_optional_variant.value());
        // Option using a library function. But as practice has shown, its array contains fewer types.
        // Trying to get a text alias of a standard type
        //        const UA_DataType* const data_type_alias = UA_findDataType( // todo Perhaps it needs to be cached, since UA_findDataType uses linear search (O(n) worst case) + comparison of NodeId
        //        structures
        //            &data_type_node_id.GetRef());

        if (data_type_node_id.GetRef().namespaceIndex == 0 && data_type_aliases.contains(data_type_node_id.GetRef().identifier.numeric)) // NOLINT(cppcoreguidelines-pro-type-union-access)
        {
            // Standard type
            return data_type_aliases.at(data_type_node_id.GetRef().identifier.numeric); // NOLINT(cppcoreguidelines-pro-type-union-access)
        }
        return data_type_node_id.ToString(); // Type not found, returning NodeID in text form.
    }

    /**
     * @brief Returns a list of tuples describing references to other model nodes and an alias for such references.
     * For example: a reference with NodeID "i=47" will return with the alias "HasComponent".
     * @return List of tuple with structure UATypesContainer<UA_ReferenceDescription> and Alias in std::string.
     * If the reference type is not found, the NodeID of the reference will be returned in text form.
     * If there are no references, an empty array will be returned.
     * Currently only standard reference types are supported.
     * @warning Each std::pair object returns a reference to a UATypesContainer<UA_ReferenceDescription> object, so the tuple itself does not own the object. The owner remains an object of the
     * NodeIntermediateModel class. This should be taken into account when using. This is done to avoid unnecessary copying of reference objects.
     */
    [[nodiscard]] std::vector<std::pair<const UATypesContainer<UA_ReferenceDescription>&, std::string>> GetNodeReferenceTypeAliases() const
    {
        if (m_references.empty())
        {
            return {};
        }
        std::vector<std::pair<const UATypesContainer<UA_ReferenceDescription>&, std::string>> ref_types_aliases_tmp;
        for (const auto& m_reference : m_references)
        {
            if (m_reference.GetRef().referenceTypeId.namespaceIndex == 0
                && reference_type_aliases.contains(m_reference.GetRef().referenceTypeId.identifier.numeric)) // NOLINT(cppcoreguidelines-pro-type-union-access)
            {
                ref_types_aliases_tmp.emplace_back(m_reference, reference_type_aliases.at(m_reference.GetRef().referenceTypeId.identifier.numeric)); // NOLINT(cppcoreguidelines-pro-type-union-access)
            }
            else
            {
                UA_String node_id_txt = UA_STRING_NULL;
                UA_NodeId_print(&m_reference.GetRef().referenceTypeId, &node_id_txt);
                ref_types_aliases_tmp.emplace_back(m_reference, std::string{static_cast<char*>(static_cast<void*>(node_id_txt.data)), node_id_txt.length});
            }
        }
        return ref_types_aliases_tmp;
    }

    /**
     * @brief Method for string information about the model.
     * @return.
     */
    [[nodiscard]] std::string ToString() const
    {
        std::string output;
        output = "NodeIntermediateModel consists:\nNodeId: " + m_node_id.ToString() + "\nParentNodeId: " + m_parent_node_id.ToString() + "\nNodeClass: " + std::to_string(m_node_class)
                 + "\nNodeReferenceDescriptions:";
        for (const auto& reference : m_references)
        {
            output.append("\n" + reference.ToString());
        }
        output.append("\nNode Attributes:");
        for (const auto& attributes : m_attributes)
        {
            output.append("\nAttributeID: " + std::to_string(attributes.first) + " : " + (attributes.second.has_value() ? VariantsOfAttrToString(attributes.second.value()) : "none"));
        }
        return output;
    }

private:
    UATypesContainer<UA_ExpandedNodeId> m_node_id;
    UATypesContainer<UA_ExpandedNodeId> m_parent_node_id;
    UA_NodeClass m_node_class = UA_NodeClass::UA_NODECLASS_UNSPECIFIED;
    std::vector<UATypesContainer<UA_ReferenceDescription>> m_references;
    std::map<UA_AttributeId, std::optional<VariantsOfAttr>> m_attributes;
};
} // namespace nodesetexporter::open62541

#endif // NODESETEXPORTER_OPEN62541_NODEINTERMEDIATEMODEL_H
