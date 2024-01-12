//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

// todo Move the implementation to .cpp

#ifndef NODESETEXPORTER_ENCODERS_GETATTRIBUTETOXMLTEXT_H
#define NODESETEXPORTER_ENCODERS_GETATTRIBUTETOXMLTEXT_H

#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/open62541/TypeAliases.h"

#include <tinyxml2.h>

/**
 * @brief A set of functions for converting the contents of Open62541 library objects into text suitable for placement in an XML document.
 */
namespace nodesetexporter::encoders::getattributetoxmltext
{

using nodesetexporter::common::UaStringToStdString;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;
using tinyxml2::XMLUtil;


/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param node_id Object of type UATypesContainer<UA_NodeId>.
 * @return A string to place in the XML document.
 *         If NodeID is empty, then an empty string is returned.
 */
static std::string UANodeIDToXMLString(const UATypesContainer<UA_NodeId>& node_id)
{
    if (UA_NodeId_isNull(&node_id.GetRef()))
    {
        return "";
    }
    return node_id.ToString();
}

/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param node_id Object of type UATypesContainer<UA_ExpandedNodeId>.
 * @return A string to place in the XML document.
 *         If ExpandedNodeID is empty, then an empty string is returned.
 */
static std::string UANodeIDToXMLString(const UATypesContainer<UA_ExpandedNodeId>& node_id)
{
    if (UA_NodeId_isNull(&node_id.GetRef().nodeId))
    {
        return "";
    }
    return node_id.ToString();
}

/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param var An object of type std::variant containing UATypesContainer<UA_NodeId>.
 * @return A string to place in the XML document.
 *         If the NodeID is empty, or the var parameter does not contain the type UATypesContainer<UA_NodeId>, then an empty string is returned.
 */
static std::string UANodeIDToXMLString(const VariantsOfAttr& var)
{
    if (const auto* pval = std::get_if<UATypesContainer<UA_NodeId>>(&var))
    {
        if (UA_NodeId_isNull(&pval->GetRef()))
        {
            return "";
        }
        return pval->ToString();
    }
    return "";
}

/**
 * @brief Convert UA_QualifiedName to text variant for XML.
 * @param var An object of type std::variant containing UATypesContainer<UA_QualifiedName>.
 * @return A string to place in the XML document.
 *         If UA_QualifiedName is empty, or the var parameter does not contain the type UATypesContainer<UA_QualifiedName>, then an empty string is returned.
 */
static std::string UAQualifiedNameToXMLString(const VariantsOfAttr& var)
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

/**
 * @brief Convert ArrayDimensions to text variant for XML.
 * @param var An object of type std::variant containing std::vector<UA_UInt32>.
 * @return A string to place in the XML document.
 *         If std::vector<UA_UInt32> is empty, or the var parameter does not contain the type std::vector<UA_UInt32>, then an empty string is returned.
 */
static std::string UAArrayDimensionToXMLString(const VariantsOfAttr& var)
{
    if (const auto* pval = std::get_if<std::vector<UA_UInt32>>(&var))
    {
        if (pval->empty())
        {
            return "";
        }
        std::string result;
        for (auto val : *pval)
        {
            result.append(std::to_string(val) + ",");
        }
        result.erase(result.end() - 1);
        return result;
    }
    return "";
}


struct LocalizedTextXML
{
    std::string locale;
    std::string text;
};
/**
 * @brief Convert UA_LocalizedText into a LocalizedTextXML structure representing text fields for output in XML.
 * @param var An object of type std::variant containing a UATypesContainer<UA_LocalizedText>.
 * @return A string to place in the XML document.
 *         If UATypesContainer<UA_LocalizedText> is empty, or the var parameter does not contain the type UATypesContainer<UA_LocalizedText>, then an empty object is returned.
 */
static LocalizedTextXML UALocalizedTextToXMLString(const VariantsOfAttr& var)
{
    LocalizedTextXML result = {};
    if (const auto* pval = std::get_if<UATypesContainer<UA_LocalizedText>>(&var))
    {
        result.locale = UaStringToStdString(pval->GetRef().locale);
        result.text = UaStringToStdString(pval->GetRef().text);
    }
    return result;
}

/**
 * @brief Convert Open62541 primitives to a string for XML output. To convert features, tinyxml2 functions are used.
 * @param var An object of type std::variant containing the types: UA_Boolean, UA_Byte, UA_UInt32, UA_Int32, UA_Double, UA_NodeClass(UA_Int32).
 * @return A string to place in the XML document.
 *         If the var parameter does not contain the listed types, an empty string is returned.
 */
static std::string UAPrimitivesToXMLString(const VariantsOfAttr& var)
{
    std::string result;
    // Dimensions taken from https://github.com/open62541/open62541/blob/master/src/ua_types_encoding_xml.c
    constexpr static size_t bolean_chrs = 6; // there was clearly missing one more symbol for false.
    constexpr static size_t byte_chrs = 4;
    constexpr static size_t uint32_chrs = 11;
    constexpr static size_t int32_chrs = 12;
    constexpr static size_t double_chrs = 2000;

    if (const auto* pval = std::get_if<UA_Boolean>(&var))
    {
        result.resize(bolean_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }
    if (const auto* pval = std::get_if<UA_Byte>(&var))
    {
        result.resize(byte_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }
    if (const auto* pval = std::get_if<UA_UInt32>(&var))
    {
        result.resize(uint32_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }
    if (const auto* pval = std::get_if<UA_Int32>(&var))
    {
        result.resize(int32_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }
    if (const auto* pval = std::get_if<UA_Double>(&var))
    {
        result.resize(double_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }
    if (const auto* pval = std::get_if<UA_NodeClass>(&var)) // UA_Int32
    {
        result.resize(int32_chrs);
        XMLUtil::ToStr(*pval, result.data(), static_cast<int>(result.size()));
    }

    return result;
}

} // namespace nodesetexporter::encoders::getattributetoxmltext

#endif // NODESETEXPORTER_ENCODERS_GETATTRIBUTETOXMLTEXT_H
