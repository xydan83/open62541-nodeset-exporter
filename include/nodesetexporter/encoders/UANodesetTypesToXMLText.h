//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_ENCODERS_UANODESETTYPESTOXMLTEXT_H
#define NODESETEXPORTER_ENCODERS_UANODESETTYPESTOXMLTEXT_H

#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/open62541/TypeAliases.h"

/**
 * @brief A set of functions for transforming the contents of Open62541 library objects
 * into text suitable for placement in an XML document.
 */
namespace nodesetexporter::encoders::uanodesettypestoxmltext
{
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;

/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param node_id Object of type UATypesContainer<UA_NodeId>.
 * @return A string to place in the XML document.
 *         If NodeID is empty, then an empty string is returned.
 */
std::string UANodeIDToXMLString(const UATypesContainer<UA_NodeId>& node_id);

/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param node_id Object of type UATypesContainer<UA_ExpandedNodeId>.
 * @return A string to place in the XML document.
 *         If ExpandedNodeID is empty, then an empty string is returned.
 */
std::string UANodeIDToXMLString(const UATypesContainer<UA_ExpandedNodeId>& node_id);

/**
 * @brief Convert UA_NodeID to text variant for XML.
 * @param var An object of type std::variant containing UATypesContainer<UA_NodeId>.
 * @return A string to place in the XML document.
 *         If the NodeID is empty, or the var parameter does not contain the type UATypesContainer<UA_NodeId>, then an empty string is returned.
 */
std::string UANodeIDToXMLString(const VariantsOfAttr& var);

/**
 * @brief Convert UA_QualifiedName to text variant for XML.
 * @param var An object of type std::variant containing UATypesContainer<UA_QualifiedName>.
 * @return A string to place in the XML document.
 *         If UA_QualifiedName is empty, or the var parameter does not contain the type UATypesContainer<UA_QualifiedName>, then an empty string is returned.
 */
std::string UAQualifiedNameToXMLString(const VariantsOfAttr& var);

/**
 * @brief Convert ArrayDimensions to text variant for XML.
 * @param var An object of type std::variant containing std::vector<UA_UInt32>.
 * @return A string to place in the XML document.
 *         If std::vector<UA_UInt32> is empty, or the var parameter does not contain the type std::vector<UA_UInt32>, then an empty string is returned.
 */
std::string UAArrayDimensionToXMLString(const VariantsOfAttr& var);


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
LocalizedTextXML UALocalizedTextToXMLString(const VariantsOfAttr& var);

/**
 * @brief Convert Open62541 primitives to a string for XML output. To convert features, tinyxml2 functions are used.
 * @param var An object of type std::variant containing the types: UA_Boolean, UA_Byte, UA_UInt32, UA_Int32, UA_Double, UA_NodeClass(UA_Int32).
 * @return A string to place in the XML document.
 *         If the var parameter does not contain the listed types, an empty string is returned.
 */
std::string UAPrimitivesToXMLString(const VariantsOfAttr& var);

} // namespace nodesetexporter::encoders::uanodesettypestoxmltext

#endif // NODESETEXPORTER_ENCODERS_UANODESETTYPESTOXMLTEXT_H
