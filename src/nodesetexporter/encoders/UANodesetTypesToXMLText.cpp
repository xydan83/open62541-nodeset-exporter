//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/encoders/UANodesetTypesToXMLText.h"

#include <tinyxml2.h>

namespace nodesetexporter::encoders::uanodesettypestoxmltext
{
using nodesetexporter::common::UaStringToStdString;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using tinyxml2::XMLUtil;

std::string UANodeIDToXMLString(const UATypesContainer<UA_NodeId>& node_id)
{
    if (UA_NodeId_isNull(&node_id.GetRef()))
    {
        return "";
    }
    return node_id.ToString();
}

std::string UANodeIDToXMLString(const UATypesContainer<UA_ExpandedNodeId>& node_id)
{
    if (UA_NodeId_isNull(&node_id.GetRef().nodeId))
    {
        return "";
    }
    return node_id.ToString();
}

std::string UAQualifiedNameToXMLString(const VariantsOfAttr& var)
{
    if (const auto* pval = std::get_if<UATypesContainer<UA_QualifiedName>>(&var))
    {
        std::string ns_i_str;
        if (UA_QualifiedName_isNull(&pval->GetRef()))
        {
            return "";
        }
        if (pval->GetRef().namespaceIndex != 0)
        {
            ns_i_str = std::to_string(pval->GetRef().namespaceIndex) + ":";
        }
        auto name = UaStringToStdString(pval->GetRef().name);
        return ns_i_str + name;
    }
    return "";
}

std::string UANodeIDToXMLString(const VariantsOfAttr& var)
{
    return std::visit(
        []<class T>(const T& arg) -> std::string
        {
            if constexpr (std::is_same_v<T, UATypesContainer<UA_NodeId>>)
            {
                return UANodeIDToXMLString(static_cast<UATypesContainer<UA_NodeId>>(arg));
            }
            else if constexpr (std::is_same_v<T, UATypesContainer<UA_ExpandedNodeId>>)
            {
                return UANodeIDToXMLString(static_cast<UATypesContainer<UA_ExpandedNodeId>>(arg));
            }
            return "";
        },
        var);
}

std::string UAArrayDimensionToXMLString(const VariantsOfAttr& var)
{
    if (const auto* pval = std::get_if<MultidimensionalArray<UA_UInt32>>(&var))
    {
        if (pval->GetArray().empty())
        {
            return "";
        }
        std::string result;
        for (const auto val : pval->GetArray())
        {
            result.append(std::to_string(val) + ",");
        }
        result.erase(result.end() - 1);
        return result;
    }
    return "";
}

LocalizedTextXML UALocalizedTextToXMLString(const VariantsOfAttr& var)
{
    LocalizedTextXML result = {};
    if (const auto* pval = std::get_if<UATypesContainer<UA_LocalizedText>>(&var))
    {
        result.locale = UaStringToStdString(pval->GetRef().locale);
        result.text = UaStringToStdString(pval->GetRef().text);
    }
    return result;
}

std::string UAPrimitivesToXMLString(const VariantsOfAttr& var)
{
    return std::visit(
        []<class T>(const T& arg) -> std::string
        {
            // Size taken as maximum from https://github.com/open62541/open62541/blob/master/src/ua_types_encoding_xml.c
            constexpr static size_t max_encoding_chars_size = 32;
            std::string result;
            result.resize(max_encoding_chars_size);

            if constexpr (std::is_arithmetic_v<T>)
            {
                XMLUtil::ToStr(static_cast<T>(arg), result.data(), static_cast<int>(result.size()));
            }
            else if constexpr (std::is_same_v<T, UA_NodeClass>)
            {
                XMLUtil::ToStr(static_cast<UA_Int32>(arg), result.data(), static_cast<int>(result.size()));
            }
            else
            {
                return "";
            }
            return result;
        },
        var);
}

} // namespace nodesetexporter::encoders::uanodesettypestoxmltext