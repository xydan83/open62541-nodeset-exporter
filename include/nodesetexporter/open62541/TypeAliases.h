//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_TYPEALIASES_H
#define NODESETEXPORTER_OPEN62541_TYPEALIASES_H

#include "nodesetexporter/open62541/UATypesContainer.h"

#include <optional>
#include <variant>
#include <vector>

namespace nodesetexporter::open62541::typealiases
{
using ::nodesetexporter::open62541::UATypesContainer;

using VariantsOfAttr = std::variant<
    UA_Boolean, // Used by the IsAbstract, Symmetric, ContainsNoLoops, Historizing, Executable, UserExecutable attribute
    UA_Byte, // Used by the EventNotifier, AccessLevel, UserAccessLevel attribute
    UA_UInt32, // Used by the WriteMask, UserWriteMask attribute
    UA_Int32, // Used by the ValueRank attribute
    UA_Double, // Used by the MinimumSamplingInterval attribute
    UA_NodeClass, // Used by the NodeClass attribute
    UATypesContainer<UA_NodeId>, // Used by the DataType attribute
    UATypesContainer<UA_QualifiedName>, // Used by the BrowseName attribute
    UATypesContainer<UA_LocalizedText>, // Used by the DisplayName, Description, InverseName attribute
    UATypesContainer<UA_Variant>, // Used by the Value attribute. The library itself transfers all attributes in the UA_Variant type, but in the same way as the values of the UA_ATTRIBUTEID_VALUE
                                  // attribute can have absolutely any value or an array of values of any depth, then it does not lend itself well to static description, therefore the decision was
                                  // made to pass such values unchanged, in packaging inside std::vector. (perhaps the approach will change)
    std::vector<UA_UInt32>, // Used by the ArrayDimensions attribute - carries an UInt32 array.
    UATypesContainer<UA_StructureDefinition>, // Used by the DataTypeDefinition attribute (no separate function)
    UATypesContainer<UA_EnumDefinition>>; // Used by the DataTypeDefinition attribute (no separate function)

/**
 * @brief Function for converting variant variable values into a string.
 * @param var
 * @return A string representing the value of the variant variable.
 */
std::string VariantsOfAttrToString(const VariantsOfAttr& var);

/**
 * @brief Function to convert UA_Variant to std::optional<VariantsOfAttr>.
 * @param variant UA_Variant.
 * @return Returns a std::variant described as VariantsOfAttr with an optional wrapper to allow null content to be expressed (similar to a monostate).
 *         If there is no value or if there is content not supported in VariantsOfAttr, the value will be nullopt.
 */
std::optional<VariantsOfAttr> UAVariantToStdVariant(const UA_Variant& variant);

} // namespace nodesetexporter::open62541::typealiases
#endif // NODESETEXPORTER_OPEN62541_TYPEALIASES_H
