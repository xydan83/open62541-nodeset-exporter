//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_V_1_3_COMPATIBILITY_H
#define NODESETEXPORTER_V_1_3_COMPATIBILITY_H

// For Open62541 Ver1.3.x compatibility.
#ifdef OPEN62541_VER_1_3
// NOLINTBEGIN

#include <float.h>

#define UA_NS0ID_CONTROLS 25254 /* ReferenceType */
#define UA_NS0ID_HASREFERENCEDESCRIPTION 32679 /* ReferenceType */
#define UA_NS0ID_HASLOWERLAYERINTERFACE 25238 /* ReferenceType */
#define UA_NS0ID_HASLOWERLAYERINTERFACE 25238 /* ReferenceType */
#define UA_NS0ID_HASPUSHEDSECURITYGROUP 25345 /* ReferenceType */
#define UA_NS0ID_ALARMSUPPRESSIONGROUPMEMBER 32059 /* ReferenceType */
#define UA_NS0ID_REQUIRES 25256 /* ReferenceType */

/**
 * .. _write-mask:
 *
 * Write Masks
 * -----------
 * The write mask and user write mask is given by the following constants that
 * are ANDed for the overall write mask. Part 3: 5.2.7 Table 2 */

#define UA_WRITEMASK_BROWSENAME (0x01u << 2u)
#define UA_WRITEMASK_DESCRIPTION (0x01u << 5u)
#define UA_WRITEMASK_DISPLAYNAME (0x01u << 6u)
#define UA_WRITEMASK_USERACCESSLEVEL (0x01u << 16u)
#define UA_WRITEMASK_ACCESSLEVELEX (0x01u << 25u)

/**
 * Float
 * ^^^^^
 * An IEEE single precision (32 bit) floating point value. */
typedef float UA_Float;
#define UA_FLOAT_MIN FLT_MIN
#define UA_FLOAT_MAX FLT_MAX


/**
 * Double
 * ^^^^^^
 * An IEEE double precision (64 bit) floating point value. */
typedef double UA_Double;
#define UA_DOUBLE_MIN DBL_MIN
#define UA_DOUBLE_MAX DBL_MAX

static UA_Boolean UA_Variant_equal(const UA_Variant* p1, const UA_Variant* p2)
{
    return (UA_order(p1, p2, &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ);
}

static UA_Boolean UA_DiagnosticInfo_equal(const UA_DiagnosticInfo* p1, const UA_DiagnosticInfo* p2)
{
    return (UA_order(p1, p2, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]) == UA_ORDER_EQ);
}

static UA_Boolean UA_LocalizedText_equal(const UA_LocalizedText* p1, const UA_LocalizedText* p2)
{
    return (UA_order(p1, p2, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]) == UA_ORDER_EQ);
}

// NOLINTEND
#endif

#endif // NODESETEXPORTER_V_1_3_COMPATIBILITY_H
