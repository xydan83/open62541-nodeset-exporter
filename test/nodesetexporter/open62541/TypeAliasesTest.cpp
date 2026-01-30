//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//
#include "nodesetexporter/open62541/TypeAliases.h"
#include "nodesetexporter/common/v_1.3_Compatibility.h"
#include "nodesetexporter/common/v_1.4_Compatibility.h"

#include <doctest/doctest.h>
#include <open62541/types.h>

#include <string>

using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::ByteString;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using nodesetexporter::open62541::typealiases::StatusCode;
using nodesetexporter::open62541::typealiases::UAVariantToStdVariant;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;
using nodesetexporter::open62541::typealiases::VariantsOfAttrToString;

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::typealiases::VariantsOfAttrToString") // NOLINT
    {
        SUBCASE("UA_Boolean")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Boolean>(true));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_SByte")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_SByte>(UA_SBYTE_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Byte")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Byte>(UA_BYTE_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Int16")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int16>(UA_INT16_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_UInt16")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt16>(UA_UINT16_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Int32")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int32>(UA_INT32_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_UInt32")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt32>(UA_UINT32_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Int64")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int64>(UA_INT64_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_UInt64")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt64>(UA_UINT64_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Float")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Float>(UA_FLOAT_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Double")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Double>(UA_DOUBLE_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_NodeClass")
        {
            constexpr UA_NodeClass test = UA_NodeClass::UA_NODECLASS_METHOD;
            const auto test_type = VariantsOfAttr(static_cast<UA_NodeClass>(test));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("StatusCode")
        {
            const auto test_type = VariantsOfAttr(StatusCode(UA_STATUSCODE_BADALREADYEXISTS));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<ByteString>")
        {
            UATypesContainer<ByteString> test(ByteString(UA_BYTESTRING("Test byte string")), UA_TYPES_BYTESTRING);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_DateTime>")
        {
            UATypesContainer<UA_DateTime> test(UA_DateTime_now(), UA_TYPES_DATETIME);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_Guid>")
        {
            UATypesContainer<UA_Guid> test(UA_GUID("0097b8ca-2453-406d-bd4f-8a5acb2a1bf2"), UA_TYPES_GUID);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_String>")
        {
            UATypesContainer<UA_String> test(UA_STRING("Test string"), UA_TYPES_STRING);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> integer")
        {
            UATypesContainer<UA_NodeId> test(UA_TYPES_NODEID);
            test.SetParamFromString(std::string("ns=1;i=100"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> string")
        {
            UATypesContainer<UA_NodeId> test(UA_TYPES_NODEID);
            test.SetParamFromString(std::string("ns=1;s=Test string"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> byte string")
        {
            UATypesContainer<UA_NodeId> test(UA_TYPES_NODEID);
            test.SetParamFromString(std::string("ns=1;b=VGVzdCBzdHJpbmc="));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> guid")
        {
            UATypesContainer<UA_NodeId> test(UA_TYPES_NODEID);
            test.SetParamFromString(std::string("ns=1;g=0097b8ca-2453-406d-bd4f-8a5acb2a1bf2"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId> integer")
        {
            UATypesContainer<UA_ExpandedNodeId> test(UA_TYPES_EXPANDEDNODEID);
            test.SetParamFromString(std::string("svr=2;ns=1;i=100"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId> string")
        {
            UATypesContainer<UA_ExpandedNodeId> test(UA_TYPES_EXPANDEDNODEID);
            test.SetParamFromString(std::string("svr=3;nsu=urn:test;s=Test string"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId> byte string")
        {
            UATypesContainer<UA_ExpandedNodeId> test(UA_TYPES_EXPANDEDNODEID);
            test.SetParamFromString(std::string("ns=1;b=VGVzdCBzdHJpbmc="));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId> guid")
        {
            UATypesContainer<UA_ExpandedNodeId> test(UA_TYPES_EXPANDEDNODEID);
            test.SetParamFromString(std::string("ns=1;g=0097b8ca-2453-406d-bd4f-8a5acb2a1bf2"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_QualifiedName>")
        {

            auto test = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
            test.GetRef().namespaceIndex = 1;
            test.GetRef().name = UA_String_fromChars("Some UA_QualifiedName");
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_LocalizedText>")
        {
            auto test = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
            test.GetRef().locale = UA_String_fromChars("en");
            test.GetRef().text = UA_String_fromChars("Some UA_LocalizedText");
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_Variant> scalar")
        {
            auto test = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            // Variant must be done via UA_Int64_new on the heap, otherwise there will be an error when deleting
            // (attempt to remove all links through pointers)
            auto* some_scalar = UA_Int64_new();
            *some_scalar = UA_INT64_MAX;
            UA_Variant_setScalar(&test.GetRef(), some_scalar, &UA_TYPES[UA_TYPES_INT64]);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_Variant> array")
        {
            auto test = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            auto* some_array = static_cast<UA_Int16*>(UA_Array_new(2, &UA_TYPES[UA_TYPES_INT16]));
            some_array[0] = UA_INT16_MAX; // NOLINT
            some_array[1] = 12434; // NOLINT
            size_t sz = 2; // NOLINT
            // Second option (append-resize)
            auto* some_scalar = UA_Int16_new();
            *some_scalar = 24436; // NOLINT
            CHECK(UA_StatusCode_isGood(UA_Array_append(reinterpret_cast<void**>(&some_array), &sz, some_scalar, &UA_TYPES[UA_TYPES_INT16])));
            UA_Variant_setArray(&test.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_INT16]);
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
            UA_Int16_delete(some_scalar);
        }

        SUBCASE("UATypesContainer<UA_StructureDefinition>")
        {
            auto* struct_field_test = UA_StructureField_new(); // An object is removed from the heap when the UATypesContainer container is deleted
            UA_StructureField_init(struct_field_test);
            UATypesContainer<UA_StructureDefinition> test(UA_TYPES_STRUCTUREDEFINITION); // NOLINT
            test.GetRef().fieldsSize = 1;
            test.GetRef().structureType = UA_StructureType::UA_STRUCTURETYPE_STRUCTURE;
            test.GetRef().fields = struct_field_test; // transfer of ownership of the structure
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_EnumDefinition>")
        {
            auto* enum_field_test = UA_EnumField_new(); // An object is removed from the heap when the UATypesContainer container is deleted
            UA_EnumField_init(enum_field_test);
            enum_field_test->name = UA_STRING_ALLOC("field_1");
            enum_field_test->value = 7444;
            UATypesContainer<UA_EnumDefinition> test(UA_TYPES_ENUMDEFINITION); // NOLINT
            test.GetRef().fieldsSize = 1;
            test.GetRef().fields = enum_field_test; // transfer of ownership of the structure
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_DiagnosticInfo>")
        {
            UATypesContainer<UA_DiagnosticInfo> test(UA_TYPES_DIAGNOSTICINFO); // NOLINT
            test.GetRef().additionalInfo = UA_STRING_ALLOC("Additional info test");
            test.GetRef().hasAdditionalInfo = true;
            test.GetRef().namespaceUri = 33;
            test.GetRef().innerStatusCode = 22;
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Boolean>>")
        {
            MultidimensionalArray<UA_Boolean> test{{true, false, true, false, true, false, true, false}, {2, 4}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_SByte>>")
        {
            MultidimensionalArray<UA_SByte> test{{14, -123, 23, 40, 3, 0, -100}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Byte>>")
        {
            MultidimensionalArray<UA_Byte> test{{14, 255, 123, 233, 45, 0, 200}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Int16>>")
        {
            MultidimensionalArray<UA_Int16> test{{-124, 24455, UA_INT16_MIN, -2433, UA_INT16_MAX, 0, 3200}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_UInt16>>")
        {
            MultidimensionalArray<UA_UInt16> test{{124, UA_UINT16_MAX, 11123, 2433, 425, 0, 3200}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Int32>>")
        {
            MultidimensionalArray<UA_Int32> test{{3200, 1203, UA_INT32_MAX, 40, UA_INT32_MIN, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_UInt32>>")
        {
            MultidimensionalArray<UA_UInt32> test{{3200, 1203, UA_UINT32_MAX, 40, 3, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Int64>>")
        {
            MultidimensionalArray<UA_Int64> test{{3200, 1203, UA_INT64_MAX, 40, UA_INT64_MIN, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_UInt64>>")
        {
            MultidimensionalArray<UA_UInt64> test{{3200, 1203, UA_UINT64_MAX, 40, 3, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Float>>")
        {
            MultidimensionalArray<UA_Float> test{{3200.0, 1203.321, UA_FLOAT_MAX, 40.2332112, UA_FLOAT_MIN, 0, 100.42}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UA_Double>>")
        {
            MultidimensionalArray<UA_Double> test{{3200.0, 1203.321, UA_DOUBLE_MAX, 40.2332112, UA_DOUBLE_MIN, 0, 100.42}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<StatusCode>>")
        {
            MultidimensionalArray<StatusCode> test{
                {static_cast<StatusCode>(UA_STATUSCODE_BADNOCONTINUATIONPOINTS),
                 static_cast<StatusCode>(UA_STATUSCODE_GOODCASCADEINITIALIZATIONACKNOWLEDGED),
                 static_cast<StatusCode>(UA_STATUSCODE_UNCERTAINENGINEERINGUNITSEXCEEDED),
                 static_cast<StatusCode>(UA_STATUSCODE_GOODCASCADENOTSELECTED)}};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<ByteString>>>")
        {
            std::vector<UATypesContainer<ByteString>> array;
            constexpr auto index_side = 10;
            array.reserve(index_side);
            for (size_t index = 0; index < index_side; ++index)
            {
                array.emplace_back(ByteString(UA_BYTESTRING(fmt::format("Just a string: {}", (index * 22230)).data())), UA_TYPES_BYTESTRING);
            }
            MultidimensionalArray<UATypesContainer<ByteString>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_DateTime>>")
        {
            std::vector<UATypesContainer<UA_DateTime>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                array.emplace_back(UA_DateTime_now() + (index * 24000000), UA_TYPES_DATETIME);
            }
            MultidimensionalArray<UATypesContainer<UA_DateTime>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_Guid>>")
        {
            std::vector<UATypesContainer<UA_Guid>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                array.emplace_back(UA_GUID(fmt::format("0097b8ca-{:0>4}-406d-bd4f-8a5acb2a1bf2", (index * 20)).c_str()), UA_TYPES_GUID);
            }
            MultidimensionalArray<UATypesContainer<UA_Guid>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_String>>")
        {
            std::vector<UATypesContainer<UA_String>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                array.emplace_back(UA_STRING(fmt::format("Just a string: {}", (index * 22230)).data()), UA_TYPES_STRING);
            }
            MultidimensionalArray<UATypesContainer<UA_String>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_NodeId>>")
        {
            std::vector<UATypesContainer<UA_NodeId>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                array.emplace_back(UA_NODEID(fmt::format("ns=2;i={}", index + 1).c_str()), UA_TYPES_NODEID);
            }
            MultidimensionalArray<UATypesContainer<UA_NodeId>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>")
        {
            std::vector<UATypesContainer<UA_ExpandedNodeId>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                array.emplace_back(UA_EXPANDEDNODEID(fmt::format("svr=2;ns=2;i={}", index + 1).c_str()), UA_TYPES_EXPANDEDNODEID);
            }
            MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_QualifiedName>>")
        {
            std::vector<UATypesContainer<UA_QualifiedName>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                auto cont = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
                cont.GetRef().namespaceIndex = 1 + index;
                cont.GetRef().name = UA_String_fromChars(fmt::format("Some UA_QualifiedName{}", index).c_str());
                array.push_back(std::move(cont));
            }
            MultidimensionalArray<UATypesContainer<UA_QualifiedName>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_LocalizedText>>")
        {
            std::vector<UATypesContainer<UA_LocalizedText>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                auto cont = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
                cont.GetRef().locale = UA_String_fromChars("en");
                cont.GetRef().text = UA_String_fromChars(fmt::format("Some UA_LocalizedText{}", index).c_str());
                array.push_back(std::move(cont));
            }
            MultidimensionalArray<UATypesContainer<UA_LocalizedText>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_Variant>>")
        {
            std::vector<UATypesContainer<UA_Variant>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                auto cont = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
                auto* some_scalar = UA_Int64_new();
                *some_scalar = UA_INT64_MAX - index;
                UA_Variant_setScalar(&cont.GetRef(), some_scalar, &UA_TYPES[UA_TYPES_INT64]);
                array.push_back(std::move(cont));
            }
            MultidimensionalArray<UATypesContainer<UA_Variant>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>")
        {
            std::vector<UATypesContainer<UA_DiagnosticInfo>> array;
            constexpr auto index_size = 10;
            array.reserve(index_size);
            for (size_t index = 0; index < index_size; ++index)
            {
                UATypesContainer<UA_DiagnosticInfo> cont(UA_TYPES_DIAGNOSTICINFO); // NOLINT
                cont.GetRef().additionalInfo = UA_STRING_ALLOC(fmt::format("Additional info test {}", index).c_str());
                cont.GetRef().hasAdditionalInfo = true;
                cont.GetRef().namespaceUri = 1 + index; // NOLINT
                cont.GetRef().innerStatusCode = 100 + index;
                array.push_back(std::move(cont));
            }
            MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }
    }

    TEST_CASE("nodesetexporter::open62541::typealiases::UAVariantToStdVariant") // NOLINT
    {
        auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
        SUBCASE("UA_Boolean")
        {
            auto* test_data = UA_Boolean_new();
            *test_data = true;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_BOOLEAN]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Boolean res = false;
            CHECK_NOTHROW(res = std::get<UA_Boolean>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_SByte")
        {
            auto* test_data = UA_SByte_new();
            *test_data = UA_SBYTE_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_SBYTE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_SByte res = 0;
            CHECK_NOTHROW(res = std::get<UA_SByte>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Byte")
        {
            auto* test_data = UA_Byte_new();
            *test_data = UA_BYTE_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_BYTE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Byte res = 0;
            CHECK_NOTHROW(res = std::get<UA_Byte>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Int16")
        {
            auto* test_data = UA_Int16_new();
            *test_data = UA_INT16_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_INT16]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Int16 res = 0;
            CHECK_NOTHROW(res = std::get<UA_Int16>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_UInt16")
        {
            auto* test_data = UA_UInt16_new();
            *test_data = UA_UINT16_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_UINT16]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_UInt16 res = 0;
            CHECK_NOTHROW(res = std::get<UA_UInt16>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Int32")
        {
            auto* test_data = UA_Int32_new();
            *test_data = UA_INT32_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_INT32]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Int32 res = 0;
            CHECK_NOTHROW(res = std::get<UA_Int32>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_UInt32")
        {
            auto* test_data = UA_UInt32_new();
            *test_data = UA_UINT32_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_UINT32]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_UInt32 res = 0;
            CHECK_NOTHROW(res = std::get<UA_UInt32>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Int64")
        {
            auto* test_data = UA_Int64_new();
            *test_data = UA_INT64_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_INT64]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Int64 res = 0;
            CHECK_NOTHROW(res = std::get<UA_Int64>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_UInt64")
        {
            auto* test_data = UA_UInt64_new();
            *test_data = UA_UINT64_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_UINT64]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_UInt64 res = 0;
            CHECK_NOTHROW(res = std::get<UA_UInt64>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Float")
        {
            auto* test_data = UA_Float_new();
            *test_data = UA_FLOAT_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_FLOAT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Float res = 0;
            CHECK_NOTHROW(res = std::get<UA_Float>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_Double")
        {
            auto* test_data = UA_Double_new();
            *test_data = UA_DOUBLE_MAX;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_DOUBLE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Double res = 0;
            CHECK_NOTHROW(res = std::get<UA_Double>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_NodeClass")
        {
            auto* test_data = UA_NodeClass_new();
            *test_data = UA_NodeClass::UA_NODECLASS_METHOD;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_NODECLASS]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_NodeClass res = UA_NodeClass::UA_NODECLASS_UNSPECIFIED;
            CHECK_NOTHROW(res = std::get<UA_NodeClass>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("StatusCode")
        {
            auto* test_data = UA_StatusCode_new();
            *test_data = UA_STATUSCODE_BADALREADYEXISTS;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_STATUSCODE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            StatusCode res(UA_STATUSCODE_GOOD);
            CHECK_NOTHROW(res = std::get<StatusCode>(*res_var));
            CHECK_EQ(*test_data, res.m_status_code);
        }

        SUBCASE("UATypesContainer<ByteString>")
        {
            auto test_data = UA_BYTESTRING("Test string");
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data, &UA_TYPES[UA_TYPES_BYTESTRING]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<ByteString> res(UA_TYPES_BYTESTRING);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<ByteString>>(*res_var)));
            CHECK(UA_ByteString_equal(&test_data, &res.GetRef().m_byte_string));
        }

        SUBCASE("UATypesContainer<UA_DateTime>")
        {
            auto test_data = UA_DateTime_now();
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data, &UA_TYPES[UA_TYPES_DATETIME]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_DateTime> res(UA_TYPES_DATETIME);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_DateTime>>(*res_var)));
            CHECK_EQ(test_data, res.GetRef());
        }

        SUBCASE("UATypesContainer<UA_Guid>")
        {
            auto test_data = UA_GUID("0097b8ca-2453-406d-bd4f-8a5acb2a1bf2");
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data, &UA_TYPES[UA_TYPES_GUID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_Guid> res(UA_TYPES_GUID);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_Guid>>(*res_var)));
            CHECK(UA_Guid_equal(&test_data, &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_String>")
        {
            auto test_data = UA_STRING("Test string");
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data, &UA_TYPES[UA_TYPES_STRING]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_String> res(UA_TYPES_STRING);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_String>>(*res_var)));
            CHECK(UA_String_equal(&test_data, &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_NodeId>")
        {
            auto* test_data = UA_NodeId_new();
            UA_NodeId_parse(test_data, UA_STRING("ns=1;i=100"));
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_NODEID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_NodeId> res(UA_TYPES_NODEID);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_NodeId>>(*res_var)));
            CHECK(UA_NodeId_equal(test_data, &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId>")
        {
            auto* test_data = UA_ExpandedNodeId_new();
            UA_ExpandedNodeId_parse(test_data, UA_STRING("srv=2;ns=2;s=info.test.node1"));
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_ExpandedNodeId> res(UA_TYPES_EXPANDEDNODEID);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_ExpandedNodeId>>(*res_var)));
            CHECK(UA_ExpandedNodeId_equal(test_data, &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_QualifiedName>")
        {
            auto* test_data = UA_QualifiedName_new();
            test_data->namespaceIndex = 1;
            test_data->name = UA_STRING_ALLOC("Test Data");
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_QualifiedName> res(UA_TYPES_QUALIFIEDNAME);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_QualifiedName>>(*res_var)));
            CHECK(UA_QualifiedName_equal(test_data, &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_LocalizedText>")
        {
            auto* test_data = UA_LocalizedText_new();
            test_data->text = UA_STRING_ALLOC("Test Data");
            test_data->locale = UA_STRING_ALLOC("en");
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_LocalizedText> res(UA_TYPES_QUALIFIEDNAME);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_LocalizedText>>(*res_var)));
            CHECK(UA_String_equal(&test_data->locale, &res.GetRef().locale));
            CHECK(UA_String_equal(&test_data->text, &res.GetRef().text));
        }

        SUBCASE("UATypesContainer<UA_Variant>")
        {
            auto test_data = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            // The second option is to make the scalar not on the heap, but on the stack, but use UA_Variant_setScalarCopy
            // to copy it inside the variant, then the variant will also be deleted correctly.
            UA_Int64 some_scalar = UA_INT64_MAX; // NOLINT
            UA_Variant_setScalarCopy(&test_data.GetRef(), &some_scalar, &UA_TYPES[UA_TYPES_INT64]);
            // Теперь поместим вариант в вариант
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data.GetRef(), &UA_TYPES[UA_TYPES_VARIANT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_Variant> res(UA_TYPES_VARIANT);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_Variant>>(*res_var)));
            CHECK(UA_Variant_equal(&test_data.GetRef(), &res.GetRef()));
        }

        SUBCASE("UATypesContainer<UA_StructureDefinition>")
        {
            auto* test_data = UA_StructureDefinition_new();
            auto* struct_field_test = UA_StructureField_new(); // An object is removed from the heap when the UATypesContainer container is deleted
            UA_StructureField_init(struct_field_test);
            test_data->fieldsSize = 1;
            test_data->structureType = UA_StructureType::UA_STRUCTURETYPE_STRUCTURE;
            test_data->fields = struct_field_test;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_StructureDefinition> res(UA_TYPES_STRUCTUREDEFINITION);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_StructureDefinition>>(*res_var)));
            CHECK_EQ(test_data->fieldsSize, res.GetRef().fieldsSize);
            CHECK_EQ(test_data->structureType, res.GetRef().structureType);
            CHECK_NE(res.GetRef().fields, nullptr);
        }

        SUBCASE("UATypesContainer<UA_EnumDefinition>")
        {
            auto* test_data = UA_EnumDefinition_new();
            auto* enum_field_test = UA_EnumField_new(); // An object is removed from the heap when the UATypesContainer container is deleted
            UA_EnumField_init(enum_field_test);
            test_data->fieldsSize = 1;
            test_data->fields = enum_field_test;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_ENUMDEFINITION]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_EnumDefinition> res(UA_TYPES_ENUMDEFINITION);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_EnumDefinition>>(*res_var)));
            CHECK_EQ(test_data->fieldsSize, res.GetRef().fieldsSize);
            CHECK_NE(res.GetRef().fields, nullptr);
        }

        SUBCASE("UATypesContainer<UA_DiagnosticInfo>")
        {
            UATypesContainer<UA_DiagnosticInfo> test_data(UA_TYPES_DIAGNOSTICINFO); // NOLINT
            test_data.GetRef().additionalInfo = UA_STRING_ALLOC("Additional info test");
            test_data.GetRef().hasAdditionalInfo = true;
            test_data.GetRef().namespaceUri = 33;
            test_data.GetRef().innerStatusCode = 22;
            UA_Variant_setScalarCopy(&test_type.GetRef(), &test_data.GetRef(), &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UATypesContainer<UA_DiagnosticInfo> res(UA_TYPES_DIAGNOSTICINFO);
            CHECK_NOTHROW(res = std::move(std::get<UATypesContainer<UA_DiagnosticInfo>>(*res_var)));
            CHECK(UA_DiagnosticInfo_equal(&test_data.GetRef(), &res.GetRef()));
        }

        // Arrays
        SUBCASE("MultidimensionalArray<UA_Boolean>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Boolean*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_BOOLEAN]));
            some_array[0] = true; // NOLINT
            some_array[1] = false; // NOLINT
            some_array[2] = true; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_BOOLEAN]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Boolean> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Boolean>>(*res_var));
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Boolean*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_SByte>>")
        {
            // Working out the multidimensionality of an array
            const size_t sz = 8; // NOLINT
            auto* some_array = static_cast<UA_SByte*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_SBYTE]));
            some_array[0] = UA_SBYTE_MAX - 1; // NOLINT
            some_array[1] = UA_SBYTE_MAX - 2; // NOLINT
            some_array[2] = UA_SBYTE_MAX - 3; // NOLINT
            some_array[3] = UA_SBYTE_MAX - 4; // NOLINT
            some_array[4] = UA_SBYTE_MAX - 5; // NOLINT
            some_array[5] = UA_SBYTE_MAX - 6; // NOLINT
            some_array[6] = UA_SBYTE_MAX - 7; // NOLINT
            some_array[7] = UA_SBYTE_MAX - 8; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_SBYTE]);
            test_type.GetRef().arrayDimensionsSize = 2;
            auto* dim = static_cast<UA_UInt32*>(UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]));
            dim[0] = 2;
            dim[1] = 4;
            test_type.GetRef().arrayDimensions = dim;
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_SByte> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_SByte>>(*res_var));
            CHECK_EQ(res.ArrayDimensionsLength(), 2);
            const std::vector<UA_UInt32> ch_vctr{2, 4};
            CHECK_EQ(res.GetArrayDimensions(), ch_vctr);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_SByte*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_SByte>> - incorrect array dimension indicated")
        {
            // Handling an exception when the depth is incorrectly specified
            const size_t sz = 8; // NOLINT
            auto* some_array = static_cast<UA_SByte*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_SBYTE]));
            some_array[0] = UA_SBYTE_MAX - 1; // NOLINT
            some_array[1] = UA_SBYTE_MAX - 2; // NOLINT
            some_array[2] = UA_SBYTE_MAX - 3; // NOLINT
            some_array[3] = UA_SBYTE_MAX - 4; // NOLINT
            some_array[4] = UA_SBYTE_MAX - 5; // NOLINT
            some_array[5] = UA_SBYTE_MAX - 6; // NOLINT
            some_array[6] = UA_SBYTE_MAX - 7; // NOLINT
            some_array[7] = UA_SBYTE_MAX - 8; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_SBYTE]);
            // According to the OPC UA standard, the depth for one-dimensional arrays is not specified.
            test_type.GetRef().arrayDimensionsSize = 1;
            auto* dim = static_cast<UA_UInt32*>(UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]));
            dim[0] = 8;
            test_type.GetRef().arrayDimensions = dim;
            std::optional<VariantsOfAttr> res_var;
            // An exception should be thrown
            CHECK_THROWS(res_var = UAVariantToStdVariant(test_type.GetRef()));
        }

        SUBCASE("MultidimensionalArray<UA_Byte>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Byte*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_BYTE]));
            some_array[0] = UA_SBYTE_MAX - 1; // NOLINT
            some_array[1] = UA_SBYTE_MAX - 2; // NOLINT
            some_array[2] = UA_SBYTE_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_BYTE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Byte> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Byte>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Byte*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Int16>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Int16*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_INT16]));
            some_array[0] = UA_INT16_MAX - 1; // NOLINT
            some_array[1] = UA_INT16_MAX - 2; // NOLINT
            some_array[2] = UA_INT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_INT16]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Int16> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Int16>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Int16*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_UInt16>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_UInt16*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_UINT16]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_UINT16]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_UInt16> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_UInt16>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_UInt16*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Int32>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Int32*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_INT32]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_INT32]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Int32> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Int32>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Int32*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_UInt32>>")
        {
            size_t sz = 2; // NOLINT
            auto* some_array = static_cast<UA_Int32*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_UINT32]));
            some_array[0] = UA_UINT32_MAX; // NOLINT
            some_array[1] = 12434; // NOLINT
            // Second option (append-resize)
            UA_Int32 some_scalar = 24436;
            CHECK(UA_StatusCode_isGood(UA_Array_append(reinterpret_cast<void**>(&some_array), &sz, &some_scalar, &UA_TYPES[UA_TYPES_UINT32])));
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_UINT32]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_UInt32> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_UInt32>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_UInt32*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Int64>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Int64*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_INT64]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_INT64]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Int64> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Int64>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Int64*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_UInt64>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_UInt64*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_UINT64]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_UINT64]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_UInt64> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_UInt64>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_UInt64*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Float>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Float*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_FLOAT]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_FLOAT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Float> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Float>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Float*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Double>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Double*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_DOUBLE]));
            some_array[0] = UA_UINT16_MAX - 1; // NOLINT
            some_array[1] = UA_UINT16_MAX - 2; // NOLINT
            some_array[2] = UA_UINT16_MAX - 3; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_DOUBLE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UA_Double> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UA_Double>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_Double*>(test_type.GetRef().data)[index], res.GetArray().at(index));
            }
        }

        SUBCASE("MultidimensionalArray<StatusCode>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_StatusCode*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_STATUSCODE]));
            some_array[0] = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
            some_array[1] = UA_STATUSCODE_GOODCASCADEINITIALIZATIONACKNOWLEDGED;
            some_array[2] = UA_STATUSCODE_UNCERTAINENGINEERINGUNITSEXCEEDED;
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_STATUSCODE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<StatusCode> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<StatusCode>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_StatusCode*>(test_type.GetRef().data)[index], res.GetArray().at(index).m_status_code);
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<ByteString>>")
        {
            //             Отработка многомерности массива
            const size_t sz = 8; // NOLINT
            auto* some_array = static_cast<UA_ByteString*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_BYTESTRING]));
            some_array[0] = UA_BYTESTRING_ALLOC("ByteString 1"); // NOLINT
            some_array[1] = UA_BYTESTRING_ALLOC("ByteString 2"); // NOLINT
            some_array[2] = UA_BYTESTRING_ALLOC("ByteString 3"); // NOLINT
            some_array[3] = UA_BYTESTRING_ALLOC("ByteString 4"); // NOLINT
            some_array[4] = UA_BYTESTRING_ALLOC("ByteString 5"); // NOLINT
            some_array[5] = UA_BYTESTRING_ALLOC("ByteString 6"); // NOLINT
            some_array[6] = UA_BYTESTRING_ALLOC("ByteString 7"); // NOLINT
            some_array[7] = UA_BYTESTRING_ALLOC("ByteString 8"); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_BYTESTRING]);
            test_type.GetRef().arrayDimensionsSize = 2;
            auto* dim = static_cast<UA_UInt32*>(UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]));
            dim[0] = 2;
            dim[1] = 4;
            test_type.GetRef().arrayDimensions = dim;
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<ByteString>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<ByteString>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 2);
            const std::vector<UA_UInt32> ch_vctr{2, 4};
            CHECK_EQ(res.GetArrayDimensions(), ch_vctr);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_ByteString_equal(&static_cast<UA_ByteString*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef().m_byte_string));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_DateTime>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_DateTime*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_DATETIME]));
            some_array[0] = UA_DateTime_now(); // NOLINT
            some_array[1] = UA_DateTime_now() + 5000; // NOLINT
            some_array[2] = UA_DateTime_now() + 10000; // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_DATETIME]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_DateTime>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_DateTime>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_DateTime*>(test_type.GetRef().data)[index], res.GetArray().at(index).GetRef());
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_Guid>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Guid*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_GUID]));
            some_array[0] = UA_GUID(fmt::format("0097b8ca-0321-406d-bd4f-8a5acb2a1bf2").c_str()); // NOLINT
            some_array[1] = UA_GUID(fmt::format("1197b8ca-0221-106d-bc4f-8a4acb2a1bf3").c_str()); // NOLINT
            some_array[2] = UA_GUID(fmt::format("2397b8ca-0127-48ad-bd4c-8a2acb2abaf1").c_str()); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_GUID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_Guid>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_Guid>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_Guid_equal(&static_cast<UA_Guid*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_String>>")
        {
            // Отработка многомерности массива
            const size_t sz = 8; // NOLINT
            auto* some_array = static_cast<UA_String*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_STRING]));
            some_array[0] = UA_STRING_ALLOC("String 1"); // NOLINT
            some_array[1] = UA_STRING_ALLOC("String 2"); // NOLINT
            some_array[2] = UA_STRING_ALLOC("String 3"); // NOLINT
            some_array[3] = UA_STRING_ALLOC("String 4"); // NOLINT
            some_array[4] = UA_STRING_ALLOC("String 5"); // NOLINT
            some_array[5] = UA_STRING_ALLOC("String 6"); // NOLINT
            some_array[6] = UA_STRING_ALLOC("String 7"); // NOLINT
            some_array[7] = UA_STRING_ALLOC("String 8"); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_STRING]);
            test_type.GetRef().arrayDimensionsSize = 2;
            auto* dim = static_cast<UA_UInt32*>(UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]));
            dim[0] = 2;
            dim[1] = 4;
            test_type.GetRef().arrayDimensions = dim;
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_String>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_String>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 2);
            const std::vector<UA_UInt32> ch_vctr{2, 4};
            CHECK_EQ(res.GetArrayDimensions(), ch_vctr);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_String_equal(&static_cast<UA_String*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_NodeId>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_NodeId*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_NODEID]));
            some_array[0] = UA_NODEID("ns=2;i=1"); // NOLINT
            some_array[1] = UA_NODEID("ns=2;s=some.node.id"); // NOLINT
            some_array[2] = UA_NODEID("ns=2;g=2397b8ca-0127-48ad-bd4c-8a2acb2abaf1"); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_NODEID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_NodeId>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_NodeId>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_NodeId_equal(&static_cast<UA_NodeId*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_ExpandedNodeId*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]));
            some_array[0] = UA_EXPANDEDNODEID("ns=2;i=1"); // NOLINT
            some_array[1] = UA_EXPANDEDNODEID("svr=3;ns=2;s=some.node.id"); // NOLINT
            some_array[2] = UA_EXPANDEDNODEID("ns=2;g=2397b8ca-0127-48ad-bd4c-8a2acb2abaf1"); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_ExpandedNodeId_equal(&static_cast<UA_ExpandedNodeId*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_LocalizedText>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_LocalizedText*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
            some_array[0] = UA_LOCALIZEDTEXT_ALLOC("en", "UA_LOCALIZEDTEXT test 1"); // NOLINT
            some_array[1] = UA_LOCALIZEDTEXT_ALLOC("ru", "UA_LOCALIZEDTEXT тест 2"); // NOLINT
            some_array[2] = UA_LOCALIZEDTEXT_ALLOC("de", "UA_LOCALIZEDTEXT test 3"); // NOLINT
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_LocalizedText>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_LocalizedText>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_LocalizedText_equal(&static_cast<UA_LocalizedText*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_Variant>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_Variant*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_VARIANT]));
            for (size_t index = 0; index < sz; ++index)
            {
                UA_Variant var;
                auto* some_scalar = UA_Int64_new();
                *some_scalar = UA_INT64_MAX - index;
                UA_Variant_setScalar(&var, some_scalar, &UA_TYPES[UA_TYPES_INT64]);
                some_array[index] = var;
            }
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_VARIANT]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_Variant>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_Variant>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_Variant_equal(&static_cast<UA_Variant*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>")
        {
            const size_t sz = 3; // NOLINT
            auto* some_array = static_cast<UA_DiagnosticInfo*>(UA_Array_new(sz, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]));
            for (size_t index = 0; index < sz; ++index)
            {
                UA_DiagnosticInfo var;
                UA_DiagnosticInfo_init(&var);
                var.additionalInfo = UA_STRING_ALLOC(fmt::format("Additional info test {}", index).c_str());
                var.hasAdditionalInfo = true;
                var.namespaceUri = 1 + index; // NOLINT
                var.innerStatusCode = 100 + index;
                some_array[index] = var;
            }
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>> res;
            CHECK_NOTHROW(res = std::get<MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>>(*res_var));
            CHECK_GT(res.ArrayLength(), 0);
            CHECK_EQ(res.ArrayDimensionsLength(), 0);
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK(UA_DiagnosticInfo_equal(&static_cast<UA_DiagnosticInfo*>(test_type.GetRef().data)[index], &res.GetArray().at(index).GetRef()));
            }
        }
    }
}