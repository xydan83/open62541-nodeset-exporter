//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/common/Strings.h"


namespace nodesetexporter::common
{
// NOLINTBEGIN
#if defined(OPEN62541_VER_1_3)

static const u_int8_t hexmapLower[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static const u_int8_t hexmapUpper[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static inline void UA_Guid_to_hex(const UA_Guid* guid, u_int8_t* out, UA_Boolean lower)
{
    const u_int8_t* hexmap = (lower) ? hexmapLower : hexmapUpper;
    size_t i = 0, j = 28;
    for (; i < 8; i++, j -= 4) /* pos 0-7, 4byte, (a) */
        out[i] = hexmap[(guid->data1 >> j) & 0x0Fu];
    out[i++] = '-'; /* pos 8 */
    for (j = 12; i < 13; i++, j -= 4) /* pos 9-12, 2byte, (b) */
        out[i] = hexmap[(uint16_t)(guid->data2 >> j) & 0x0Fu];
    out[i++] = '-'; /* pos 13 */
    for (j = 12; i < 18; i++, j -= 4) /* pos 14-17, 2byte (c) */
        out[i] = hexmap[(uint16_t)(guid->data3 >> j) & 0x0Fu];
    out[i++] = '-'; /* pos 18 */
    for (j = 0; i < 23; i += 2, j++)
    { /* pos 19-22, 2byte (d) */
        out[i] = hexmap[(guid->data4[j] & 0xF0u) >> 4u];
        out[i + 1] = hexmap[guid->data4[j] & 0x0Fu];
    }
    out[i++] = '-'; /* pos 23 */
    for (j = 2; i < 36; i += 2, j++)
    { /* pos 24-35, 6byte (e) */
        out[i] = hexmap[(guid->data4[j] & 0xF0u) >> 4u];
        out[i + 1] = hexmap[guid->data4[j] & 0x0Fu];
    }
}

static inline UA_StatusCode UA_Guid_print(const UA_Guid* guid, UA_String* output)
{
    if (output->length == 0)
    {
        UA_StatusCode res = UA_ByteString_allocBuffer((UA_ByteString*)output, 36);
        if (res != UA_STATUSCODE_GOOD)
            return res;
    }
    else
    {
        if (output->length < 36)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        output->length = 36;
    }
    UA_Guid_to_hex(guid, output->data, true);
    return UA_STATUSCODE_GOOD;
}

UA_Boolean UA_String_isEmpty(const UA_String* str)
{
    return (str->length == 0 || str->data == NULL);
}
#endif
// NOLINTEND

std::string UaIdIdentifierToStdString(const UA_NodeId& node_id)
{
    std::string id_text{};
    switch (node_id.identifierType)
    {
    case UA_NODEIDTYPE_NUMERIC:
        id_text = std::to_string(node_id.identifier.numeric); // NOLINT(cppcoreguidelines-pro-type-union-access)
        break;
    case UA_NODEIDTYPE_STRING:
        id_text = UaStringToStdString(node_id.identifier.string); // NOLINT(cppcoreguidelines-pro-type-union-access)
        break;
    case UA_NODEIDTYPE_GUID:
    {
        auto guid = UA_STRING_NULL;
        UA_Guid_print(&node_id.identifier.guid, &guid); // NOLINT(cppcoreguidelines-pro-type-union-access)
        id_text = UaStringToStdString(guid);
        UA_String_clear(&guid);
    }
    break;
    case UA_NODEIDTYPE_BYTESTRING:
        id_text = UaStringToStdString(node_id.identifier.byteString); // NOLINT(cppcoreguidelines-pro-type-union-access)
        break;
    }
    return id_text;
}
} // namespace nodesetexporter::common