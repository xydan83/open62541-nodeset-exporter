//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_ENCODERS_UAVALUETYPESTOXMLTEXT_H
#define NODESETEXPORTER_ENCODERS_UAVALUETYPESTOXMLTEXT_H

#include "nodesetexporter/open62541/TypeAliases.h"

#include <tinyxml2.h>

/**
 * @brief Module for outputting the values of "<Value/>" nodes to an XML tree.
 */
namespace nodesetexporter::encoders::uavaluetypestoxmltext
{
using nodesetexporter::open62541::typealiases::VariantsOfAttr;
using tinyxml2::XMLElement;

/**
* @brief Method for adding XML elements with values based on OPC UA characteristics.
* @param var Data to be added to the XML tree.
* @param xml_root_el to which other elements with values should be added.
 */
void AddValueToXml(const VariantsOfAttr& var, XMLElement& xml_root_el);

} // namespace nodesetexporter::encoders::uavaluetypestoxmltext

#endif // NODESETEXPORTER_ENCODERS_UAVALUETYPESTOXMLTEXT_H
