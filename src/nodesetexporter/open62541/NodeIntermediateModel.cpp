//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/NodeIntermediateModel.h"

namespace nodesetexporter::open62541
{

std::string NodeIntermediateModel::GetDataTypeAlias() const
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

    if (data_type_node_id.GetRef().namespaceIndex == 0 && data_type_aliases.contains(data_type_node_id.GetRef().identifier.numeric)) // NOLINT(cppcoreguidelines-pro-type-union-access)
    {
        // Standard type
        return data_type_aliases.at(data_type_node_id.GetRef().identifier.numeric); // NOLINT(cppcoreguidelines-pro-type-union-access)
    }
    return data_type_node_id.ToString(); // Type not found, returning NodeID in text form.
}

std::vector<std::pair<const UATypesContainer<UA_ReferenceDescription>&, std::string>> NodeIntermediateModel::GetNodeReferenceTypeAliases() const
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
            ref_types_aliases_tmp.emplace_back(m_reference, std::string{reinterpret_cast<char*>(node_id_txt.data), node_id_txt.length}); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            UA_String_clear(&node_id_txt);
        }
    }
    return ref_types_aliases_tmp;
}

std::string NodeIntermediateModel::ToString() const
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
} // namespace nodesetexporter::open62541