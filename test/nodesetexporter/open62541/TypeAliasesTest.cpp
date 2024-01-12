//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//
#include "nodesetexporter/open62541/TypeAliases.h"

#include <doctest/doctest.h>

#include <string>

using nodesetexporter::open62541::UATypesContainer;
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

        SUBCASE("UA_Byte")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Byte>(UA_BYTE_MAX));
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

        SUBCASE("UA_Int32")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int32>(UA_INT32_MAX));
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UA_Double")
        {
            constexpr double test = 311421.21;
            const auto test_type = VariantsOfAttr(static_cast<UA_Double>(test));
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

        SUBCASE("UATypesContainer<UA_NodeId> integer")
        {

            auto test = UATypesContainer<UA_NodeId>(UA_TYPES_NODEID);
            UA_NodeId_parse(&test.GetRef(), UA_String_fromChars("ns=1;i=100"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> string")
        {

            auto test = UATypesContainer<UA_NodeId>(UA_TYPES_NODEID);
            UA_NodeId_parse(&test.GetRef(), UA_String_fromChars("ns=1;s=Test string"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> byte string")
        {

            auto test = UATypesContainer<UA_NodeId>(UA_TYPES_NODEID);
            UA_NodeId_parse(&test.GetRef(), UA_String_fromChars("ns=1;b=Test string"));
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_NodeId> guid")
        {

            auto test = UATypesContainer<UA_NodeId>(UA_TYPES_NODEID);
            UA_NodeId_parse(&test.GetRef(), UA_String_fromChars("ns=1;g=0097b8ca-2453-406d-bd4f-8a5acb2a1bf2"));
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
            // Variant must be done UA_Int64_new on the heap, otherwise there will be an error when deleting (trying to remove all links through pointers)
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
        }

        SUBCASE("std::vector<UA_UInt32>>")
        {
            std::vector<UA_UInt32> test{3200, 1203, 123, 40, 3, 0, 100}; // NOLINT
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }

        SUBCASE("UATypesContainer<UA_StructureDefinition>")
        {
            auto* struct_field_test = UA_StructureField_new(); // An object is removed from the heap when the UATypesContainer container is deleted
            UA_StructureField_init(struct_field_test);
            UATypesContainer<UA_StructureDefinition> test(UA_TYPES_STRUCTUREDEFINITION); // NOLINT
            test.GetRef().fieldsSize = 1;
            test.GetRef().structureType = UA_StructureType::UA_STRUCTURETYPE_STRUCTURE;
            test.GetRef().fields = struct_field_test; // Transfer of ownership of the structure
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
            UATypesContainer<UA_EnumDefinition> test(UA_TYPES_ENUMDEFINITION); // NOLINT
            test.GetRef().fieldsSize = 1;
            test.GetRef().fields = enum_field_test; // Transfer of ownership of the structure
            const auto test_type = VariantsOfAttr(test);
            std::string test_str;
            CHECK_NOTHROW(test_str = VariantsOfAttrToString(test_type));
            CHECK_FALSE(test_str.empty());
            MESSAGE(test_str);
        }
    }

    TEST_CASE("nodesetexporter::open62541::typealiases::UAVariantToStdVariant") // NOLINT
    {
        SUBCASE("UA_Boolean")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("UA_Byte")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            auto* test_data = UA_Byte_new();
            *test_data = 0xFE;
            UA_Variant_setScalar(&test_type.GetRef(), test_data, &UA_TYPES[UA_TYPES_BYTE]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            UA_Byte res = 0;
            CHECK_NOTHROW(res = std::get<UA_Byte>(*res_var));
            CHECK_EQ(*test_data, res);
        }

        SUBCASE("UA_UInt32")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("UA_Int32")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("UA_Double")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            auto* test_data = UA_Double_new();
            *test_data = 3141.123;
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
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("UATypesContainer<UA_NodeId>")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("UATypesContainer<UA_QualifiedName>")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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

        SUBCASE("std::vector<UA_UInt32>>")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
            auto* some_array = static_cast<UA_Int32*>(UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]));
            some_array[0] = UA_UINT32_MAX; // NOLINT
            some_array[1] = 12434; // NOLINT
            size_t sz = 2; // NOLINT
            // Second option (append-resize)
            auto* some_scalar = UA_Int16_new();
            *some_scalar = 24436; // NOLINT
            CHECK(UA_StatusCode_isGood(UA_Array_append(reinterpret_cast<void**>(&some_array), &sz, some_scalar, &UA_TYPES[UA_TYPES_UINT32])));
            UA_Variant_setArray(&test_type.GetRef(), some_array, sz, &UA_TYPES[UA_TYPES_UINT32]);
            std::optional<VariantsOfAttr> res_var;
            CHECK_NOTHROW(res_var = UAVariantToStdVariant(test_type.GetRef()));
            CHECK(res_var.has_value());
            std::vector<UA_UInt32> res;
            CHECK_NOTHROW(res = std::get<std::vector<UA_UInt32>>(*res_var));
            for (size_t index = 0; index < test_type.GetRef().arrayLength; index++)
            {
                CHECK_EQ(static_cast<UA_UInt32*>(test_type.GetRef().data)[index], res.at(index));
            }
        }

        SUBCASE("UATypesContainer<UA_StructureDefinition>")
        {
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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
            auto test_type = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
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
    }
}