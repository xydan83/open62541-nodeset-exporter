//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_STRINGS_H
#define NODESETEXPORTER_COMMON_STRINGS_H

#include <open62541/types.h>

#include <string>

namespace nodesetexporter::common
{

/**
 * @brief Converter of strings of type UA_String of the Open62541 library to std::string
 */
[[nodiscard]] static inline std::string UaStringToStdString(const UA_String& some_string)
{
    return std::string{static_cast<char*>(static_cast<void*>(some_string.data)), some_string.length};
}

} // namespace nodesetexporter::common

#endif // NODESETEXPORTER_COMMON_STRINGS_H
