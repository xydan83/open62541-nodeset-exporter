//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/encoders/UAValueTypesToXMLText.h"
#include "LogMacro.h"
#include "XmlHelperFunctions.h"
#include "nodesetexporter/common/v_1.3_Compatibility.h"
#include "nodesetexporter/common/v_1.4_Compatibility.h"
#include "nodesetexporter/open62541/TypeAliases.h"

#include <doctest/doctest.h>

#include <tinyxml2.h> // Used to generate XML.

namespace
{
TEST_LOGGER_INIT

using LogLevel = nodesetexporter::common::LogLevel;
using nodesetexporter::encoders::uavaluetypestoxmltext::AddValueToXml;
using ::nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::ByteString;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using nodesetexporter::open62541::typealiases::StatusCode;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;
using tinyxml2::XMLDocument;

// Schema for XML validation (modified with added <Value> type for wrapper)
xmlpp::XsdValidator valid("Opc.Ua.Types.xsd"); // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-const-correctness)
} // namespace

TEST_SUITE("nodesetexporter::encoders::uavaluetypestoxmltext")
{
    TEST_CASE("ValueToXml") // NOLINT
    {
        XMLDocument doc_valid_test;
        auto* decl_valid = doc_valid_test.NewDeclaration();
        doc_valid_test.InsertFirstChild(decl_valid);
        tinyxml2::XMLPrinter printer_valid;
        // Since in the general XML (together with the nodeset) the Value elements will have the prefix “uax”, for the correctness of the tests I will add a <Value> element indicating this link.
        auto* test_el = doc_valid_test.NewElement("uax:Value");
        test_el->SetAttribute("xmlns:uax", "http://opcfoundation.org/UA/2008/02/Types.xsd");

        SUBCASE("UA_Boolean")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Boolean>(true));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_SByte")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_SByte>(UA_SBYTE_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Byte")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Byte>(UA_BYTE_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Int16")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int16>(UA_INT16_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_UInt16")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt16>(UA_UINT16_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Int32")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int32>(UA_INT32_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_UInt32")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt32>(UA_UINT32_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Int64")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Int64>(UA_INT64_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_UInt64")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_UInt64>(UA_UINT64_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Float")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Float>(UA_FLOAT_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_Double")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_Double>(UA_DOUBLE_MAX));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UA_NodeClass")
        {
            const auto test_type = VariantsOfAttr(static_cast<UA_NodeClass>(UA_NodeClass::UA_NODECLASS_DATATYPE));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("StatusCode")
        {
            const auto test_type = VariantsOfAttr(StatusCode(UA_STATUSCODE_BADNOCONTINUATIONPOINTS));
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<ByteString>")
        {
            UATypesContainer<ByteString> test(ByteString(UA_BYTESTRING("Test byte string")), UA_TYPES_BYTESTRING); // NOLINT
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_DateTime>")
        {
            UATypesContainer<UA_DateTime> test(UA_DateTime_now(), UA_TYPES_DATETIME);
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_Guid>")
        {
            UATypesContainer<UA_Guid> test(UA_GUID("0097b8ca-2453-406d-bd4f-8a5acb2a1bf2"), UA_TYPES_GUID);
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_String>")
        {
            UATypesContainer<UA_String> test(UA_STRING("Test string"), UA_TYPES_STRING);
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_NodeId>")
        {
            UATypesContainer<UA_NodeId> test(UA_TYPES_NODEID);
            test.SetParamFromString(std::string("ns=1;i=100"));
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_ExpandedNodeId>")
        {
            UATypesContainer<UA_ExpandedNodeId> test(UA_TYPES_EXPANDEDNODEID);
            test.SetParamFromString(std::string("svr=3;nsu=urn:test;s=Test string"));
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("UATypesContainer<UA_QualifiedName>")
        {
            SUBCASE("Full output")
            {
                auto test = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
                test.GetRef().namespaceIndex = 1;
                test.GetRef().name = UA_String_fromChars("Some UA_QualifiedName");
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("Without namespace")
            {
                auto test = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
                test.GetRef().name = UA_String_fromChars("Some UA_QualifiedName with zero namespace");
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("Without any")
            {
                auto test = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }
        }

        SUBCASE("UATypesContainer<UA_LocalizedText>")
        {
            SUBCASE("Full output")
            {
                auto test = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
                test.GetRef().locale = UA_String_fromChars("en");
                test.GetRef().text = UA_String_fromChars("Some UA_LocalizedText");
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("Without locale")
            {
                auto test = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
                test.GetRef().text = UA_String_fromChars("Some UA_LocalizedText");
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("Without any")
            {
                auto test = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }
        }

        SUBCASE("UATypesContainer<UA_DiagnosticInfo>")
        {
            SUBCASE("One dimension")
            {
                UATypesContainer<UA_DiagnosticInfo> test(UA_TYPES_DIAGNOSTICINFO); // NOLINT
                test.GetRef().additionalInfo = UA_STRING_ALLOC("Additional info test");
                test.GetRef().hasAdditionalInfo = true;
                test.GetRef().namespaceUri = 33;
                test.GetRef().hasNamespaceUri = true;
                test.GetRef().locale = 1;
                test.GetRef().hasLocale = true;
                test.GetRef().localizedText = 32;
                test.GetRef().hasLocalizedText = true;
                test.GetRef().innerStatusCode = 22;
                test.GetRef().hasInnerStatusCode = true;
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("With inner struct")
            {
                UATypesContainer<UA_DiagnosticInfo> test(UA_TYPES_DIAGNOSTICINFO); // NOLINT
                test.GetRef().additionalInfo = UA_STRING_ALLOC("Additional info test");
                test.GetRef().hasAdditionalInfo = true;
                test.GetRef().namespaceUri = 33;
                test.GetRef().hasNamespaceUri = true;
                test.GetRef().locale = 1;
                test.GetRef().hasLocale = true;
                test.GetRef().localizedText = 32;
                test.GetRef().hasLocalizedText = true;
                test.GetRef().innerStatusCode = 22;
                test.GetRef().hasInnerStatusCode = true;

                auto* inner_struct = UA_DiagnosticInfo_new(); // NOLINT
                inner_struct->additionalInfo = UA_STRING_ALLOC("Inner struct additional info test");
                inner_struct->hasAdditionalInfo = true;
                inner_struct->namespaceUri = 21;
                inner_struct->hasNamespaceUri = true;
                inner_struct->innerStatusCode = 31;
                inner_struct->hasInnerStatusCode = true;

                test.GetRef().innerDiagnosticInfo = inner_struct;
                test.GetRef().hasInnerDiagnosticInfo = true;
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }
        }

        SUBCASE("MultidimensionalArray<UA_Boolean>")
        {
            SUBCASE("With dimensions greater than one - Exception")
            {
                MultidimensionalArray<UA_Boolean> test{{true, false, true, false, true, false, true, false}, {2, 4}};
                const auto test_type = VariantsOfAttr(test);
                CHECK_THROWS(AddValueToXml(test_type, *test_el));
            }

            SUBCASE("With one dimension")
            {
                MultidimensionalArray<UA_Boolean> test{{true, false, true, false, true, false, true, false}};
                const auto test_type = VariantsOfAttr(test);
                CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
            }
        }

        SUBCASE("MultidimensionalArray<UA_SByte>")
        {
            MultidimensionalArray<UA_SByte> test{{14, -123, 23, 40, 3, 0, -100}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Byte>")
        {
            MultidimensionalArray<UA_Byte> test{{14, 255, 123, 233, 45, 0, 200}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Int16>")
        {
            MultidimensionalArray<UA_Int16> test{{-124, 24455, UA_INT16_MIN, -2433, UA_INT16_MAX, 0, 3200}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_UInt16>")
        {
            MultidimensionalArray<UA_UInt16> test{{124, UA_UINT16_MAX, 11123, 2433, 425, 0, 3200}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Int32>")
        {
            MultidimensionalArray<UA_Int32> test{{3200, 1203, UA_INT32_MAX, 40, UA_INT32_MIN, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_UInt32>")
        {
            MultidimensionalArray<UA_UInt32> test{{3200, 1203, UA_UINT32_MAX, 40, 3, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Int64>")
        {
            MultidimensionalArray<UA_Int64> test{{3200, 1203, UA_INT64_MAX, 40, UA_INT64_MIN, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_UInt64>")
        {
            MultidimensionalArray<UA_UInt64> test{{3200, 1203, UA_UINT64_MAX, 40, 3, 0, 100}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Float>")
        {
            MultidimensionalArray<UA_Float> test{{3200.0, 1203.321, UA_FLOAT_MAX, 40.2332112, UA_FLOAT_MIN, 0, 100.42}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UA_Double>")
        {
            MultidimensionalArray<UA_Double> test{{3200.0, 1203.321, UA_DOUBLE_MAX, 40.2332112, UA_DOUBLE_MIN, 0, 100.42}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<StatusCode>")
        {
            MultidimensionalArray<StatusCode> test{
                {StatusCode(UA_STATUSCODE_BADALREADYEXISTS),
                 StatusCode(UA_STATUSCODE_GOOD),
                 StatusCode(UA_STATUSCODE_GOODCASCADEINITIALIZATIONACKNOWLEDGED),
                 StatusCode(UA_STATUSCODE_BADBROWSENAMEINVALID),
                 StatusCode(UA_STATUSCODE_UNCERTAININITIALVALUE),
                 StatusCode(UA_STATUSCODE_GOODCASCADENOTSELECTED)}};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        SUBCASE("MultidimensionalArray<UATypesContainer<ByteString>>")
        {
            std::vector<UATypesContainer<ByteString>> array;
            for (size_t index = 0; index < 10; ++index)
            {
                array.emplace_back(ByteString(UA_BYTESTRING(fmt::format("Just a byte string: {}", (index * 22230)).data())), UA_TYPES_BYTESTRING); // NOLINT
            }
            MultidimensionalArray<UATypesContainer<ByteString>> test{std::move(array)};
            const auto test_type = VariantsOfAttr(test);
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
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
            CHECK_NOTHROW(AddValueToXml(test_type, *test_el));
        }

        doc_valid_test.InsertEndChild(test_el);
        doc_valid_test.Print(&printer_valid);
        std::string valid_test_str(printer_valid.CStr(), printer_valid.CStrSize());
        // At some point the error "Line 44, column 1 (fatal): Extra content at the end of the document" appeared.
        // Swears at the presence of a line feed at the end.
        // The libxml2 library may have been updated in libxmlpp. I haven't found another solution yet.
        // Since it is used only for testing - due to its non-criticality - I will introduce a temporary solution.
        // Similar solutions will apply wherever a buffer is used. There are no such problems when checking files.
        valid_test_str.erase(valid_test_str.rfind('\n'));
        MESSAGE(valid_test_str);
        xmlpp::DomParser parser_valid;
        CHECK_NOTHROW(parser_valid.parse_memory(valid_test_str));
        CHECK_NOTHROW(valid.validate(parser_valid.get_document())); // Check according to the scheme
    }
}