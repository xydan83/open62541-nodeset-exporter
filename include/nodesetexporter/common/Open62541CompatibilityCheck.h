//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_OPEN62541COMPATIBILITYCHECK_H
#define NODESETEXPORTER_COMMON_OPEN62541COMPATIBILITYCHECK_H

#include <open62541/config.h>

#ifdef OPEN62541_VER_1_4
static_assert((UA_OPEN62541_VER_MAJOR == 1 && UA_OPEN62541_VER_MINOR == 4), "The Open62541 library version parameter is not set correctly");
#elif defined(OPEN62541_VER_1_3)
static_assert((UA_OPEN62541_VER_MAJOR == 1 && UA_OPEN62541_VER_MINOR == 3), "The Open62541 library version parameter is not set correctly");
#else
static_assert(false, "Check the Open62541 library version. Support only versions 1.3.x and 1.4.x");
#endif

#endif // NODESETEXPORTER_COMMON_OPEN62541COMPATIBILITYCHECK_H
