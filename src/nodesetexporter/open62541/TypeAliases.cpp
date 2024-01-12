//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/TypeAliases.h"

namespace nodesetexporter::open62541::typealiases
{

std::string VariantsOfAttrToString(const VariantsOfAttr& var)
{
    if (const auto* pval = std::get_if<UA_Boolean>(&var))
    {
        return std::to_string(static_cast<int>(*pval));
    }
    if (const auto* pval = std::get_if<UA_Byte>(&var))
    {
        return std::to_string(*pval);
    }
    if (const auto* pval = std::get_if<UA_UInt32>(&var))
    {
        return std::to_string(*pval);
    }
    if (const auto* pval = std::get_if<UA_Int32>(&var))
    {
        return std::to_string(*pval);
    }
    if (const auto* pval = std::get_if<UA_Double>(&var))
    {
        return std::to_string(*pval);
    }
    if (const auto* pval = std::get_if<UA_NodeClass>(&var))
    {
        return std::to_string(*pval);
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_NodeId>>(&var))
    {
        return pval->ToString();
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_QualifiedName>>(&var))
    {
        return pval->ToString();
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_LocalizedText>>(&var))
    {
        return pval->ToString();
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_Variant>>(&var))
    {
        return pval->ToString();
    }
    if (const auto* pval = std::get_if<std::vector<UA_UInt32>>(&var))
    {
        std::string out{"[ "};
        for (auto val : *pval)
        {
            out.append(std::to_string(val) + ", ");
        }
        out.erase(out.end() - 2);
        out.append("]");
        return out;
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_StructureDefinition>>(&var))
    {
        return pval->ToString();
    }
    if (const auto* pval = std::get_if<UATypesContainer<UA_EnumDefinition>>(&var))
    {
        return pval->ToString();
    }
    return "";
}

std::optional<VariantsOfAttr> UAVariantToStdVariant(const UA_Variant& variant)
{
    if (UA_Variant_isEmpty(&variant))
    {
        return {std::nullopt};
    }

    if (UA_Variant_isScalar(&variant))
    {
        if (variant.type == &UA_TYPES[UA_TYPES_BOOLEAN])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_Boolean*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_BYTE])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_Byte*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT32])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_UInt32*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT32])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_Int32*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DOUBLE])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_Double*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_NODECLASS])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<UA_NodeClass*>(variant.data))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_NODEID])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_NodeId>(*static_cast<UA_NodeId*>(variant.data), UA_TYPES_NODEID))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_QUALIFIEDNAME])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_QualifiedName>(*static_cast<UA_QualifiedName*>(variant.data), UA_TYPES_QUALIFIEDNAME))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_LocalizedText>(*static_cast<UA_LocalizedText*>(variant.data), UA_TYPES_LOCALIZEDTEXT))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_StructureDefinition>(*static_cast<UA_StructureDefinition*>(variant.data), UA_TYPES_STRUCTUREDEFINITION))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_EnumDefinition>(*static_cast<UA_EnumDefinition*>(variant.data), UA_TYPES_ENUMDEFINITION))};
        }
    }
    else
    {
        if (variant.type == &UA_TYPES[UA_TYPES_UINT32])
        {
            if (variant.data == nullptr)
            {
                return {std::nullopt};
            }
            std::vector<UA_UInt32> vec(variant.arrayLength);
            memcpy(vec.data(), variant.data, variant.arrayLength * sizeof(UA_UInt32));
            return std::optional<VariantsOfAttr>{VariantsOfAttr(vec)};
        }
    }

    return {std::nullopt};
}

} // namespace nodesetexporter::open62541::typealiases