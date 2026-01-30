//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_V_1_4_COMPATIBILITY_H
#define NODESETEXPORTER_V_1_4_COMPATIBILITY_H

#include <open62541/config.h>

// For earlier Open62541 Ver1.4.x compatibility.
#ifdef OPEN62541_VER_1_4

#if UA_OPEN62541_VER_PATCH < 5
// NOLINTBEGIN
/**
 * Float
 * ^^^^^
 * An IEEE single precision (32 bit) floating point value. */
#define UA_FLOAT_MIN FLT_MIN
#define UA_FLOAT_MAX FLT_MAX

/**
 * Double
 * ^^^^^^
 * An IEEE double precision (64 bit) floating point value. */
#define UA_DOUBLE_MIN DBL_MIN
#define UA_DOUBLE_MAX DBL_MAX
// NOLINTEND
#endif

#endif

#endif // NODESETEXPORTER_V_1_4_COMPATIBILITY_H
