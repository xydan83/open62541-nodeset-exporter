//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/ClientWrappers.h"
#include "LogMacro.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/logger/LogPlugin.h"
#include "nodesetexporter/open62541/TypeAliases.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include "ex_nodeset.h"
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include <doctest/doctest.h>

#include <condition_variable>
#include <iostream>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

namespace
{
TEST_LOGGER_INIT

using LoggerPlugin = nodesetexporter::logger::Open62541LogPlugin;
using Open62541ClientWrapper = nodesetexporter::open62541::Open62541ClientWrapper;
using StatusResults = nodesetexporter::common::statuses::StatusResults<>;
using NodeAttributesRequestResponse = nodesetexporter::interfaces::IOpen62541::NodeAttributesRequestResponse;
using NodeClassesRequestResponse = nodesetexporter::interfaces::IOpen62541::NodeClassesRequestResponse;
using NodeReferencesRequestResponse = nodesetexporter::interfaces::IOpen62541::NodeReferencesRequestResponse;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;
using nodesetexporter::open62541::typealiases::VariantsOfAttrToString;
using namespace std::literals;

constexpr auto SERVER_START_TIMEOUT = 10s;
volatile std::atomic_bool running = true; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::condition_variable cv_server_started; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex cv_mutex; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)


std::string UaStringToStdString(const UA_String& some_string)
{
    return std::string{reinterpret_cast<char*>(some_string.data), some_string.length};
}

// // Preparing a test server to which the client will connect
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define CHECK_ERR(res)                                                                                                                                                                                 \
    if (UA_StatusCode_isBad((res)))                                                                                                                                                                    \
    {                                                                                                                                                                                                  \
        MESSAGE("OPC Server has bad status code: ", UA_StatusCode_name((res)));                                                                                                                        \
        REQUIRE(UA_StatusCode_isGood((res)));                                                                                                                                                          \
    }
// NOLINTEND(cppcoreguidelines-macro-usage)

auto OpcUaServerStart()
{
    return std::thread(
        []
        {
            MESSAGE("Server start.");
            // When updating the Open62541 library to commit 6287f35545e397b1a7384906859f5b504db6dc25, changes were made in which the creation of a server via UA_Server_new starts writing logs to
            // std::out, which negatively affects the display of logs in CLION. Therefore, changes have been made - first I create the config, and only then the Server itself.
            // Moreover, when creating a Client there are no such changes.
            UA_ServerConfig config = {0};
            Logger logger("server-test");
#ifdef OPEN62541_VER_1_3
            config.logger = LoggerPlugin::Open62541LoggerCreator(logger);
#elif defined(OPEN62541_VER_1_4)
            auto logging = LoggerPlugin::Open62541LoggerCreator(logger);
            config.logging = &logging;
#endif
            auto retval = UA_ServerConfig_setDefault(&config);
            REQUIRE_EQ(retval, UA_STATUSCODE_GOOD);
            auto* server = UA_Server_newWithConfig(&config);
            REQUIRE_NE(server, nullptr);
            CHECK_ERR(ex_nodeset(server)); // TEST NODESET LOADER (HARDCODE)
            uint64_t callback_id = 0;
            CHECK_ERR(UA_Server_addTimedCallback(
                server,
                [](UA_Server* /*server*/, void*)
                {
                    const std::lock_guard<std::mutex> locker(cv_mutex);
                    cv_server_started.notify_all();
                },
                nullptr,
                1000,
                &callback_id));
            CHECK_ERR(UA_Server_run(server, reinterpret_cast<volatile const bool*>(&running))); // NOLINT
            UA_Server_removeRepeatedCallback(server, callback_id);

#ifdef OPEN62541_VER_1_3
            UA_Server_delete(server);
#elif defined(OPEN62541_VER_1_4)
            CHECK_ERR(UA_Server_delete(server));
#endif
            MESSAGE("Server down.");
        });
}

/**
 * @brief A function for comparing two std::optional<VariantsOfAttr> types by a simplified attribute. If there is a mismatch, doctest::CHECK is fired.
 *        First, the missing data is compared, then the comparison is compared based on the attribute.
 * @param attr_id The attribute by which the VariantsOfAttr standard will be compared.
 * @param first The first comparison object
 * @param second comparison object
 */
void CheckAttrValueEqual(UA_AttributeId attr_id, const std::optional<VariantsOfAttr>& first, const std::optional<VariantsOfAttr>& second)
{
    CHECK_EQ(first.has_value(), second.has_value());
    if (first.has_value())
    {
        switch (attr_id)
        {
        case UA_AttributeId::UA_ATTRIBUTEID_NODEID: // NODE_ID
        case UA_AttributeId::UA_ATTRIBUTEID_DATATYPE: // NODE_ID
            CHECK(UA_NodeId_equal(&std::get<UATypesContainer<UA_NodeId>>(first.value()).GetRef(), &std::get<UATypesContainer<UA_NodeId>>(second.value()).GetRef()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_NODECLASS: // NodeClass
            CHECK_EQ(std::get<UA_NodeClass>(first.value()), std::get<UA_NodeClass>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_BROWSENAME: // QualifiedName
            CHECK(UA_QualifiedName_equal(&std::get<UATypesContainer<UA_QualifiedName>>(first.value()).GetRef(), &std::get<UATypesContainer<UA_QualifiedName>>(second.value()).GetRef()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_DISPLAYNAME: // LocalizedText
        case UA_AttributeId::UA_ATTRIBUTEID_DESCRIPTION: // LocalizedText
        case UA_AttributeId::UA_ATTRIBUTEID_INVERSENAME: // LocalizedText
        {
            const UA_LocalizedText loc_text1 = std::get<UATypesContainer<UA_LocalizedText>>(first.value()).GetRef();
            const UA_LocalizedText loc_text2 = std::get<UATypesContainer<UA_LocalizedText>>(second.value()).GetRef();
            CHECK(UA_String_equal(&loc_text1.text, &loc_text2.text));
            CHECK(UA_String_equal(&loc_text1.locale, &loc_text2.locale));
        }
        break;
        case UA_AttributeId::UA_ATTRIBUTEID_WRITEMASK: // UInt32
        case UA_AttributeId::UA_ATTRIBUTEID_USERWRITEMASK: // UInt32
            CHECK_EQ(std::get<UA_UInt32>(first.value()), std::get<UA_UInt32>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_ISABSTRACT: // Boolean
        case UA_AttributeId::UA_ATTRIBUTEID_SYMMETRIC: // Boolean
        case UA_AttributeId::UA_ATTRIBUTEID_CONTAINSNOLOOPS: // Boolean
        case UA_AttributeId::UA_ATTRIBUTEID_HISTORIZING: // Boolean
        case UA_AttributeId::UA_ATTRIBUTEID_EXECUTABLE: // Boolean
        case UA_AttributeId::UA_ATTRIBUTEID_USEREXECUTABLE: // Boolean
            CHECK_EQ(std::get<UA_Boolean>(first.value()), std::get<UA_Boolean>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_EVENTNOTIFIER: // Byte
        case UA_AttributeId::UA_ATTRIBUTEID_ACCESSLEVEL: // Byte
        case UA_AttributeId::UA_ATTRIBUTEID_USERACCESSLEVEL: // Byte
            CHECK_EQ(std::get<UA_Byte>(first.value()), std::get<UA_Byte>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_VALUE: // VariantsOfAttr(Любые имеющиеся значения)
        {
            // Проверка на чтение узла типа Double
            const auto* value_double_1 = std::get_if<UA_Double>(&first.value());
            const auto* value_double_2 = std::get_if<UA_Double>(&second.value());
            if (value_double_1 != nullptr && value_double_2 != nullptr)
            {
                CHECK_EQ(*value_double_1, *value_double_2);
                break;
            }
            // Проверка на чтение узла типа String
            const auto* value_string_1 = std::get_if<UATypesContainer<UA_String>>(&first.value());
            const auto* value_string_2 = std::get_if<UATypesContainer<UA_String>>(&second.value());
            if (value_string_1 != nullptr && value_string_2 != nullptr)
            {
                CHECK(UA_String_equal(&value_string_1->GetRef(), &value_string_2->GetRef()));
                break;
            }

            // Проверка на чтение узла типа Int64
            const auto* value_int64_tp_1 = std::get_if<UA_Int64>(&first.value());
            const auto* value_int64_tp_2 = std::get_if<UA_Int64>(&second.value());
            if (value_int64_tp_1 != nullptr && value_int64_tp_2 != nullptr)
            {
                CHECK_EQ(*value_int64_tp_1, *value_int64_tp_2);
                break;
            }

            // Проверка на чтение узла типа bool
            const auto* value_bool_tp_1 = std::get_if<UA_Boolean>(&first.value());
            const auto* value_bool_tp_2 = std::get_if<UA_Boolean>(&second.value());
            if (value_bool_tp_1 != nullptr && value_bool_tp_2 != nullptr)
            {
                CHECK_EQ(*value_bool_tp_1, *value_bool_tp_2);
                break;
            }

            // Проверка на чтение узла типа String array
            const auto* value_string_arr_tp_1 = std::get_if<MultidimensionalArray<UATypesContainer<UA_String>>>(&first.value());
            const auto* value_string_arr_tp_2 = std::get_if<MultidimensionalArray<UATypesContainer<UA_String>>>(&second.value());
            if (value_string_arr_tp_1 != nullptr && value_string_arr_tp_2 != nullptr)
            {
                for (size_t index = 0; index < value_string_arr_tp_1->ArrayLength(); ++index)
                {
                    CHECK(UA_String_equal(&value_string_arr_tp_1->GetArray().at(index).GetRef(), &value_string_arr_tp_2->GetArray().at(index).GetRef()));
                }
                break;
            }

            // Проверка на чтение узла типа DateTime
            const auto* value_dt_tp_1 = std::get_if<UATypesContainer<UA_DateTime>>(&first.value());
            const auto* value_dt_tp_2 = std::get_if<UATypesContainer<UA_DateTime>>(&second.value());
            if (value_dt_tp_1 != nullptr && value_dt_tp_2 != nullptr)
            {
                CHECK_EQ(value_dt_tp_1->GetRef(), value_dt_tp_2->GetRef());
                break;
            }

            // Проверка на чтение узла типа UInt32 array
            const auto* value_uint32_arr_tp_1 = std::get_if<MultidimensionalArray<UA_UInt32>>(&first.value());
            const auto* value_uint32_arr_tp_2 = std::get_if<MultidimensionalArray<UA_UInt32>>(&second.value());
            if (value_uint32_arr_tp_1 != nullptr && value_uint32_arr_tp_2 != nullptr)
            {
                for (size_t index = 0; index < value_uint32_arr_tp_1->ArrayLength(); ++index)
                {
                    CHECK_EQ(value_uint32_arr_tp_1->GetArray().at(index), value_uint32_arr_tp_2->GetArray().at(index));
                }
                break;
            }

            FAIL(true); // Если никакой вариант не пришел
        }
        break;
        case UA_AttributeId::UA_ATTRIBUTEID_VALUERANK: // Int32
            CHECK_EQ(std::get<UA_Int32>(first.value()), std::get<UA_Int32>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_ARRAYDIMENSIONS: // [UInt32]
        {
            const auto vec1 = std::get<MultidimensionalArray<UA_UInt32>>(first.value());
            const auto vec2 = std::get<MultidimensionalArray<UA_UInt32>>(second.value());
            CHECK_EQ(vec1.GetArrayDimensions(), vec2.GetArrayDimensions());
            CHECK_EQ(vec1.ArrayDimensionsLength(), vec2.ArrayDimensionsLength());
            CHECK_EQ(vec1.GetArray(), vec2.GetArray());
        }
        break;
        case UA_AttributeId::UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL: // Double
            CHECK_EQ(std::get<UA_Double>(first.value()), std::get<UA_Double>(second.value()));
            break;
        case UA_AttributeId::UA_ATTRIBUTEID_DATATYPEDEFINITION: // DataTypeDefinition
        {
            const UATypesContainer<UA_StructureDefinition>* value_str1 = std::get_if<UATypesContainer<UA_StructureDefinition>>(&first.value());
            const UATypesContainer<UA_StructureDefinition>* value_str2 = std::get_if<UATypesContainer<UA_StructureDefinition>>(&second.value());
            UA_ByteString b_str1 = {0};
            UA_ByteString b_str2 = {0};
            if (value_str1 != nullptr && value_str2 != nullptr)
            {
                UA_encodeBinary(&value_str1->GetRef(), &UA_TYPES[UA_TYPES_VARIANT], &b_str1);
                UA_encodeBinary(&value_str2->GetRef(), &UA_TYPES[UA_TYPES_VARIANT], &b_str2);
                CHECK(UA_ByteString_equal(&b_str1, &b_str2));
            }
            else
            {
                const UATypesContainer<UA_EnumDefinition>* value_enum1 = std::get_if<UATypesContainer<UA_EnumDefinition>>(&first.value());
                const UATypesContainer<UA_EnumDefinition>* value_enum2 = std::get_if<UATypesContainer<UA_EnumDefinition>>(&second.value());
                if (value_enum1 != nullptr && value_enum2 != nullptr)
                {
                    UA_encodeBinary(&value_enum1->GetRef(), &UA_TYPES[UA_TYPES_VARIANT], &b_str1);
                    UA_encodeBinary(&value_enum2->GetRef(), &UA_TYPES[UA_TYPES_VARIANT], &b_str2);
                    CHECK(UA_ByteString_equal(&b_str1, &b_str2));
                }
                else
                {
                    FAIL(true);
                }
            }
            UA_ByteString_clear(&b_str1);
            UA_ByteString_clear(&b_str2);
        }
        break;
        default:
            MESSAGE("Attribute with ID:", attr_id, " is not supported by the method.");
            FAIL(true);
        };
    }
}
} // namespace

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::Open62541ClientWrapper") // NOLINT
    {
        std::unique_lock<std::mutex> locker(cv_mutex);
        running = true;
        auto server_thread = OpcUaServerStart();
        const std::chrono::time_point<std::chrono::system_clock> server_start_timeout = std::chrono::system_clock::now() + SERVER_START_TIMEOUT;
        REQUIRE_EQ(cv_server_started.wait_until(locker, server_start_timeout), std::cv_status::no_timeout); // I expect the start of the server
        locker.unlock();

        Logger cli_logger("client-test");
        auto logging = LoggerPlugin::Open62541LoggerCreator(cli_logger);
        UA_ClientConfig cli_config = {};
        UA_ClientConfig_setDefault(&cli_config);
        auto* client = UA_Client_newWithConfig(&cli_config);
#ifdef OPEN62541_VER_1_4
        cli_config.logging = &logging;
        cli_config.eventLoop->logger = &logging;
#elif defined(OPEN62541_VER_1_3)
        cli_config.logger = logging;
#endif
        REQUIRE(UA_StatusCode_isGood(UA_Client_connect(client, "opc.tcp://localhost:4840")));

        auto client_wrapper = Open62541ClientWrapper(*client, cli_logger);

#pragma region Test data
        struct ReadNodesAtrrubutesTestData
        {
            UATypesContainer<UA_ExpandedNodeId> node_id;
            std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_results;
            size_t type_of_ua_variant_data{};
        };

        // For UA_ATTRIBUTEID_VALUE tests
        constexpr UA_Double node_i_2_value = 45.52951; // NOLINT

        // Attributes relative to the node class are presented in the mapping table https://www.open62541.org/doc/master/core_concepts.html#information-modelling
        std::map<UA_NodeClass, ReadNodesAtrrubutesTestData> test_nodes_attributes_data{
            {UA_NodeClass::UA_NODECLASS_VARIABLE,
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID),
              .attrs_results =
                  {
                      {UA_ATTRIBUTEID_NODECLASS, UA_NodeClass::UA_NODECLASS_VARIABLE}, // mandatory
                      {UA_ATTRIBUTEID_BROWSENAME, UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(2, "temperature"), UA_TYPES_QUALIFIEDNAME)}, // mandatory
                      {UA_ATTRIBUTEID_DISPLAYNAME, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "temperature"), UA_TYPES_LOCALIZEDTEXT)}, // mandatory
                      {UA_ATTRIBUTEID_DESCRIPTION, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description temperature"), UA_TYPES_LOCALIZEDTEXT)}, // optional
                      {UA_ATTRIBUTEID_WRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_USERWRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_VALUE, std::optional<VariantsOfAttr>{VariantsOfAttr(node_i_2_value)}}, // mandatory
                      {UA_ATTRIBUTEID_DATATYPE, UATypesContainer<UA_NodeId>(UA_NODEID("i=11"), UA_TYPES_NODEID)}, // mandatory
                      {UA_ATTRIBUTEID_VALUERANK, static_cast<UA_Int32>(UA_VALUERANK_ANY)}, // mandatory
                      {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::nullopt}, // optional
                      {UA_ATTRIBUTEID_ACCESSLEVEL, static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ)}, // mandatory
                      {UA_ATTRIBUTEID_USERACCESSLEVEL, static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ)}, // mandatory
                      {UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, static_cast<UA_Double>(0)}, // optional
                      {UA_ATTRIBUTEID_HISTORIZING, static_cast<UA_Boolean>(false)}, // mandatory
                      {UA_ATTRIBUTEID_INVERSENAME, std::nullopt}, // none
                      {UA_ATTRIBUTEID_EVENTNOTIFIER, std::nullopt}, // none
                  },
              .type_of_ua_variant_data = UA_TYPES_DOUBLE}},
            {UA_NodeClass::UA_NODECLASS_OBJECT,
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID),
              .attrs_results =
                  {
                      {UA_ATTRIBUTEID_NODECLASS, UA_NodeClass::UA_NODECLASS_OBJECT}, // mandatory
                      {UA_ATTRIBUTEID_BROWSENAME, UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(2, "vPLC1"), UA_TYPES_QUALIFIEDNAME)}, // mandatory
                      {UA_ATTRIBUTEID_DISPLAYNAME, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "vPLC1"), UA_TYPES_LOCALIZEDTEXT)}, // mandatory
                      {UA_ATTRIBUTEID_DESCRIPTION, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description vPLC1"), UA_TYPES_LOCALIZEDTEXT)}, // optional
                      {UA_ATTRIBUTEID_WRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_USERWRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_EVENTNOTIFIER, static_cast<UA_Byte>(UA_EVENTNOTIFIERTYPE_NONE)}, // mandatory
                      {UA_ATTRIBUTEID_ISABSTRACT, std::nullopt}, // none
                      {UA_ATTRIBUTEID_VALUE, std::nullopt}, // none
                      {UA_ATTRIBUTEID_DATATYPE, std::nullopt}, // none
                      {UA_ATTRIBUTEID_VALUERANK, std::nullopt}, // none
                      {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::nullopt}, // none
                      {UA_ATTRIBUTEID_HISTORIZING, std::nullopt}, // none
                      {UA_ATTRIBUTEID_INVERSENAME, std::nullopt}, // none
                  },
              .type_of_ua_variant_data = 0}},
            {UA_NodeClass::UA_NODECLASS_METHOD,
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=12"), UA_TYPES_EXPANDEDNODEID),
              .attrs_results =
                  {
                      {UA_ATTRIBUTEID_NODECLASS, UA_NodeClass::UA_NODECLASS_METHOD}, // mandatory
                      {UA_ATTRIBUTEID_BROWSENAME, UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(2, "TestMethod"), UA_TYPES_QUALIFIEDNAME)}, // mandatory
                      {UA_ATTRIBUTEID_DISPLAYNAME, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "TestMethod"), UA_TYPES_LOCALIZEDTEXT)}, // mandatory
                      {UA_ATTRIBUTEID_DESCRIPTION, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description TestMethod"), UA_TYPES_LOCALIZEDTEXT)}, // optional
                      {UA_ATTRIBUTEID_WRITEMASK, static_cast<UA_UInt32>(0)}, // mandatory-optional
                      {UA_ATTRIBUTEID_USERWRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_EVENTNOTIFIER, std::nullopt}, // mandatory
                      {UA_ATTRIBUTEID_VALUERANK, std::nullopt}, // mandatory
                      {UA_ATTRIBUTEID_ACCESSLEVEL, std::nullopt}, // mandatory
                      {UA_ATTRIBUTEID_EXECUTABLE, static_cast<UA_Boolean>(true)}, // mandatory
                      {UA_ATTRIBUTEID_USEREXECUTABLE, static_cast<UA_Boolean>(true)}, // mandatory
                      {UA_ATTRIBUTEID_VALUE, std::nullopt}, // none
                      {UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, std::nullopt}, // none
                      {UA_ATTRIBUTEID_DATATYPEDEFINITION, std::nullopt}, // none
                  },
              .type_of_ua_variant_data = 0}},
            {UA_NodeClass::UA_NODECLASS_DATATYPE,
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=21"), UA_TYPES_EXPANDEDNODEID),
              .attrs_results =
                  {
                      {UA_ATTRIBUTEID_NODECLASS, UA_NodeClass::UA_NODECLASS_DATATYPE}, // mandatory
                      {UA_ATTRIBUTEID_BROWSENAME, UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(2, "Union"), UA_TYPES_QUALIFIEDNAME)}, // mandatory
                      {UA_ATTRIBUTEID_DISPLAYNAME, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Union"), UA_TYPES_LOCALIZEDTEXT)}, // mandatory
                      {UA_ATTRIBUTEID_DESCRIPTION, UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description Union"), UA_TYPES_LOCALIZEDTEXT)}, // optional
                      {UA_ATTRIBUTEID_WRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_USERWRITEMASK, static_cast<UA_UInt32>(0)}, // optional
                      {UA_ATTRIBUTEID_ISABSTRACT, static_cast<UA_Boolean>(true)}, // mandatory
                      {UA_ATTRIBUTEID_DATATYPEDEFINITION, std::nullopt}, // optional
                      {UA_ATTRIBUTEID_EXECUTABLE, std::nullopt}, // none
                      {UA_ATTRIBUTEID_VALUE, std::nullopt}, // none
                  },
              .type_of_ua_variant_data = 0}}};

        struct ReadNodeDataValueTestData
        {
            UATypesContainer<UA_ExpandedNodeId> node_id;
            size_t type_of_ua_variant_data{};
            std::variant<std::string, UA_Double, UA_Int64> result{};
        };
        std::map<std::string, ReadNodeDataValueTestData> test_read_node_data_val{
            {"UA_TYPES_STRING",
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=4"), UA_TYPES_EXPANDEDNODEID), .type_of_ua_variant_data = UA_TYPES_STRING, .result = "speed"}},
            {"UA_TYPES_DOUBLE",
             {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID), .type_of_ua_variant_data = UA_TYPES_DOUBLE, .result = 45.52951}},
            {"UA_TYPES_INT64", {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=18"), UA_TYPES_EXPANDEDNODEID), .type_of_ua_variant_data = UA_TYPES_INT64, .result = 9953}},
            {"NO_DATA", {.node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID), .type_of_ua_variant_data = 0, .result = 0}}};

#pragma region for the ReadNodeReferences test
        std::vector<NodeReferencesRequestResponse> test_node_references_structure_lists;
        const auto test_parent_node1 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=15"), UA_TYPES_EXPANDEDNODEID);
        const auto test_parent_node2 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=8"), UA_TYPES_EXPANDEDNODEID);
        const auto test_parent_node3 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=21"), UA_TYPES_EXPANDEDNODEID);
        const auto test_parent_node4 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=17"), UA_TYPES_EXPANDEDNODEID);

        test_node_references_structure_lists.emplace_back(NodeReferencesRequestResponse{
            test_parent_node1,
            {UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = false,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=9"),
                  .browseName = UA_QUALIFIEDNAME(2, "myNewStaticObject1_1"),
                  .displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1_1"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_OBJECT,
                  .typeDefinition = UA_EXPANDEDNODEID("i=58")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=40"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("i=58"),
                  .browseName = UA_QUALIFIEDNAME(0, "BaseObjectType"),
                  .displayName = UA_LOCALIZEDTEXT("", "BaseObjectType"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_OBJECTTYPE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=0")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=46"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=16"),
                  .browseName = UA_QUALIFIEDNAME(2, "MyProperty2"),
                  .displayName = UA_LOCALIZEDTEXT("", "MyProperty2"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=68")},
                 UA_TYPES_REFERENCEDESCRIPTION)}});

        test_node_references_structure_lists.emplace_back(NodeReferencesRequestResponse{
            test_parent_node2,
            {UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = false,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=5"),
                  .browseName = UA_QUALIFIEDNAME(2, "myNewStaticObject1"),
                  .displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_OBJECT,
                  .typeDefinition = UA_EXPANDEDNODEID("i=58")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=40"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("i=63"),
                  .browseName = UA_QUALIFIEDNAME(0, "BaseDataVariableType"),
                  .displayName = UA_LOCALIZEDTEXT("", "BaseDataVariableType"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLETYPE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=0")},
                 UA_TYPES_REFERENCEDESCRIPTION)}});

        test_node_references_structure_lists.emplace_back(NodeReferencesRequestResponse{
            test_parent_node3,
            {UATypesContainer<UA_ReferenceDescription>(
                {.referenceTypeId = UA_NODEID("i=45"),
                 .isForward = false,
                 .nodeId = UA_EXPANDEDNODEID("i=22"),
                 .browseName = UA_QUALIFIEDNAME(0, "Structure"),
                 .displayName = UA_LOCALIZEDTEXT("", "Structure"),
                 .nodeClass = UA_NodeClass::UA_NODECLASS_DATATYPE,
                 .typeDefinition = UA_EXPANDEDNODEID("i=0")},
                UA_TYPES_REFERENCEDESCRIPTION)}});

        test_node_references_structure_lists.emplace_back(NodeReferencesRequestResponse{
            test_parent_node4,
            {UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = false,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=5"),
                  .browseName = UA_QUALIFIEDNAME(2, "myNewStaticObject1"),
                  .displayName = UA_LOCALIZEDTEXT("", "myNewStaticObject1"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_OBJECT,
                  .typeDefinition = UA_EXPANDEDNODEID("i=58")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=40"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("i=58"),
                  .browseName = UA_QUALIFIEDNAME(0, "BaseObjectType"),
                  .displayName = UA_LOCALIZEDTEXT("", "BaseObjectType"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_OBJECTTYPE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=0")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=20"),
                  .browseName = UA_QUALIFIEDNAME(2, "static_param3"),
                  .displayName = UA_LOCALIZEDTEXT("", "static_param3"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=63")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=19"),
                  .browseName = UA_QUALIFIEDNAME(2, "static_text_param2"),
                  .displayName = UA_LOCALIZEDTEXT("", "static_text_param2"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=63")},
                 UA_TYPES_REFERENCEDESCRIPTION),
             UATypesContainer<UA_ReferenceDescription>(
                 {.referenceTypeId = UA_NODEID("i=47"),
                  .isForward = true,
                  .nodeId = UA_EXPANDEDNODEID("ns=2;i=18"),
                  .browseName = UA_QUALIFIEDNAME(2, "static_param1"),
                  .displayName = UA_LOCALIZEDTEXT("", "static_param1"),
                  .nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE,
                  .typeDefinition = UA_EXPANDEDNODEID("i=63")},
                 UA_TYPES_REFERENCEDESCRIPTION)}});
#pragma endregion for the ReadNodeReferences test

#pragma endregion Test data

        SUBCASE("ReadNodeReferences")
        {
            std::vector<NodeReferencesRequestResponse> node_references_structure_lists;
            UA_ByteString b_str1 = {0};
            UA_ByteString b_str2 = {0};

            SUBCASE("Requesting single node links")
            {
                // Preparing the Query Array
                node_references_structure_lists.emplace_back(test_parent_node1);
                // Query
                CHECK_EQ(client_wrapper.ReadNodeReferences(node_references_structure_lists).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                size_t index = 0;
                MESSAGE("parent node_id: ", test_node_references_structure_lists.at(0).exp_node_id.get().ToString());
                for (const auto& test_ref : test_node_references_structure_lists.at(0).references)
                {
                    MESSAGE("\nRESULT DATA: ", node_references_structure_lists.at(0).references.at(index).ToString(), "\nTEST DATA: ", test_ref.ToString());
                    UA_encodeBinary(&test_ref.GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str1);
                    UA_encodeBinary(&node_references_structure_lists.at(0).references.at(index).GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str2);
                    CHECK(UA_ByteString_equal(&b_str1, &b_str2));
                    index++;
                }
            }

            SUBCASE("Querying multiple node links")
            {
                // Preparing the Query Array
                node_references_structure_lists.emplace_back(test_parent_node1);
                node_references_structure_lists.emplace_back(test_parent_node2);
                node_references_structure_lists.emplace_back(test_parent_node3);
                node_references_structure_lists.emplace_back(test_parent_node4);
                // Query
                CHECK_EQ(client_wrapper.ReadNodeReferences(node_references_structure_lists).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                size_t parent_node_id_index = 0;
                for (const auto& test_parent_node_id : test_node_references_structure_lists)
                {
                    size_t ref_index = 0;
                    MESSAGE("parent node_id: ", test_parent_node_id.exp_node_id.get().ToString());
                    for (const auto& test_ref : test_parent_node_id.references)
                    {
                        MESSAGE("\nRESULT DATA: ", node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).ToString(), "\nTEST DATA: ", test_ref.ToString());
                        UA_encodeBinary(&test_ref.GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str1);
                        UA_encodeBinary(&node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str2);
                        CHECK(UA_ByteString_equal(&b_str1, &b_str2));
                        ref_index++;
                    }
                    parent_node_id_index++;
                }
            }

            SUBCASE("Request multi-site references with RequestedMaxReferencesPerNode = 1")
            {
                // Preparing the Query Array
                node_references_structure_lists.emplace_back(test_parent_node1);
                node_references_structure_lists.emplace_back(test_parent_node2);
                node_references_structure_lists.emplace_back(test_parent_node3);
                node_references_structure_lists.emplace_back(test_parent_node4);
                // Query
                client_wrapper.SetRequestedMaxReferencesPerNode(1);
                CHECK_EQ(client_wrapper.ReadNodeReferences(node_references_structure_lists).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                size_t parent_node_id_index = 0;
                for (const auto& test_parent_node_id : test_node_references_structure_lists)
                {
                    size_t ref_index = 0;
                    MESSAGE("parent node_id: ", test_parent_node_id.exp_node_id.get().ToString());
                    for (const auto& test_ref : test_parent_node_id.references)
                    {
                        MESSAGE("\nRESULT DATA: ", node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).ToString(), "\nTEST DATA: ", test_ref.ToString());
                        UA_encodeBinary(&test_ref.GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str1);
                        UA_encodeBinary(&node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str2);
                        CHECK(UA_ByteString_equal(&b_str1, &b_str2));
                        ref_index++;
                    }
                    parent_node_id_index++;
                }
            }

            SUBCASE("Request multi-site references with RequestedMaxReferencesPerNode from 0 to 5")
            {
                for (size_t count_of_ref_per_node = 0; count_of_ref_per_node <= 5; count_of_ref_per_node++)
                {
                    // Preparing the Query Array
                    node_references_structure_lists.emplace_back(test_parent_node1);
                    node_references_structure_lists.emplace_back(test_parent_node2);
                    node_references_structure_lists.emplace_back(test_parent_node3);
                    node_references_structure_lists.emplace_back(test_parent_node4);
                    // Query
                    client_wrapper.SetRequestedMaxReferencesPerNode(count_of_ref_per_node);
                    CHECK_EQ(client_wrapper.GetRequestedMaxReferencesPerNode(), count_of_ref_per_node);
                    CHECK_EQ(client_wrapper.ReadNodeReferences(node_references_structure_lists).GetStatus(), StatusResults::Good);
                    // Reconciliation of results
                    size_t parent_node_id_index = 0;
                    for (const auto& test_parent_node_id : test_node_references_structure_lists)
                    {
                        size_t ref_index = 0;
                        MESSAGE("parent node_id: ", test_parent_node_id.exp_node_id.get().ToString());
                        for (const auto& test_ref : test_parent_node_id.references)
                        {
                            MESSAGE("\nRESULT DATA: ", node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).ToString(), "\nTEST DATA: ", test_ref.ToString());
                            UA_encodeBinary(&test_ref.GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str1);
                            UA_encodeBinary(&node_references_structure_lists.at(parent_node_id_index).references.at(ref_index).GetRef(), &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], &b_str2);
                            CHECK(UA_ByteString_equal(&b_str1, &b_str2));
                            ref_index++;
                        }
                        parent_node_id_index++;
                    }
                    node_references_structure_lists.clear();
                }
            }
            UA_ByteString_clear(&b_str1);
            UA_ByteString_clear(&b_str2);
        }

        SUBCASE("ReadNodeClasses")
        {
            std::vector<NodeClassesRequestResponse> node_class_structure_lists;
            SUBCASE("Test for retrieving data from multiple nodes")
            {
                // Preparing the Query Array
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    node_class_structure_lists.emplace_back(node_test.second.node_id);
                }
                // Query
                CHECK_EQ(client_wrapper.ReadNodeClasses(node_class_structure_lists).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                size_t index = 0;
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    auto result_node_class = node_class_structure_lists.at(index).node_class;
                    MESSAGE("node_id: ", node_test.second.node_id.ToString(), " get NodeClass.\nRESULT DATA: ", result_node_class, "\nTEST DATA: ", node_test.first);
                    CHECK_EQ(node_test.first, result_node_class);
                    index++;
                }
            }

            SUBCASE("Test for missing node class data when trying to access a non-existent node")
            {
                // Preparing the Query Array
                const auto node1 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=500"), UA_TYPES_EXPANDEDNODEID);
                const auto node2 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=501"), UA_TYPES_EXPANDEDNODEID);
                const auto node3 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=502"), UA_TYPES_EXPANDEDNODEID);
                const auto node4 = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=503"), UA_TYPES_EXPANDEDNODEID);
                node_class_structure_lists.emplace_back(node1);
                node_class_structure_lists.emplace_back(node2);
                node_class_structure_lists.emplace_back(node3);
                node_class_structure_lists.emplace_back(node4);
                // Запрос
                CHECK_EQ(client_wrapper.ReadNodeClasses(node_class_structure_lists).GetStatus(), StatusResults::Good);
                // Сверка результатов
                for (const auto& node_class_rr : node_class_structure_lists)
                {
                    MESSAGE("node_id: ", node_class_rr.exp_node_id.get().ToString(), " node class: ", node_class_rr.node_class, " should be: ", UA_NodeClass::UA_NODECLASS_UNSPECIFIED);
                    CHECK_EQ(node_class_rr.node_class, UA_NodeClass::UA_NODECLASS_UNSPECIFIED);
                }
            }
        }

        SUBCASE("ReadNodeDataValue")
        {
            SUBCASE("UA_TYPES_STRING")
            {
                auto test_loca_data = test_read_node_data_val.at("UA_TYPES_STRING");
                auto out = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
                CHECK_EQ(client_wrapper.ReadNodeDataValue(test_loca_data.node_id, out).GetStatus(), StatusResults::Good);
                CHECK_NE(out.GetRef().data, nullptr);
                CHECK_EQ(out.GetRef().type, &UA_TYPES[test_loca_data.type_of_ua_variant_data]);
                auto result = *static_cast<UA_String*>(out.GetRef().data);
                MESSAGE("RESULT DATA: ", UaStringToStdString(result));
                MESSAGE("TEST DATA: ", std::get<std::string>(test_loca_data.result));
                CHECK_EQ(UaStringToStdString(result), std::get<std::string>(test_loca_data.result));
            }
            SUBCASE("UA_TYPES_DOUBLE")
            {
                auto test_loca_data = test_read_node_data_val.at("UA_TYPES_DOUBLE");
                auto out = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
                CHECK_EQ(client_wrapper.ReadNodeDataValue(test_loca_data.node_id, out).GetStatus(), StatusResults::Good);
                CHECK_EQ(out.GetRef().type, &UA_TYPES[test_loca_data.type_of_ua_variant_data]);
                auto result = *static_cast<UA_Double*>(out.GetRef().data);
                MESSAGE("RESULT DATA: ", result);
                MESSAGE("TEST DATA: ", std::get<UA_Double>(test_loca_data.result));
                CHECK_EQ(result, std::get<UA_Double>(test_loca_data.result));
            }
            SUBCASE("UA_TYPES_INT64")
            {
                auto test_loca_data = test_read_node_data_val.at("UA_TYPES_INT64");
                auto out = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
                CHECK_EQ(client_wrapper.ReadNodeDataValue(test_loca_data.node_id, out).GetStatus(), StatusResults::Good);
                CHECK_EQ(out.GetRef().type, &UA_TYPES[test_loca_data.type_of_ua_variant_data]);
                auto result = *static_cast<UA_Int64*>(out.GetRef().data);
                MESSAGE("RESULT DATA: ", result);
                MESSAGE("TEST DATA: ", std::get<UA_Int64>(test_loca_data.result));
                CHECK_EQ(result, std::get<UA_Int64>(test_loca_data.result));
            }
            SUBCASE("NO DATA = StatusResults::Fail")
            {
                auto test_loca_data = test_read_node_data_val.at("NO_DATA");
                auto out = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
                CHECK_EQ(client_wrapper.ReadNodeDataValue(test_loca_data.node_id, out).GetStatus(), StatusResults::Fail);
            }
        }

        SUBCASE("ReadNodesAtrrubutes")
        {
            std::vector<NodeAttributesRequestResponse> node_attr_structure_lists;

            SUBCASE("Read a single attribute of a single node")
            {
                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_NODECLASS")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(UA_NodeClass::UA_NODECLASS_VARIABLE);

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_NODECLASS, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_NODECLASS).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_NODECLASS).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_NODECLASS, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_NODECLASS));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_BROWSENAME")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(2, "temperature"), UA_TYPES_QUALIFIEDNAME));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_BROWSENAME, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_BROWSENAME).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_BROWSENAME).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_BROWSENAME, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_BROWSENAME));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_DISPLAYNAME")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "temperature"), UA_TYPES_LOCALIZEDTEXT));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_DISPLAYNAME, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DISPLAYNAME).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DISPLAYNAME).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_DISPLAYNAME, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DISPLAYNAME));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_DESCRIPTION")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description temperature"), UA_TYPES_LOCALIZEDTEXT));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_DESCRIPTION, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DESCRIPTION).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DESCRIPTION).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_DESCRIPTION, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DESCRIPTION));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_WRITEMASK")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_UInt32>(0));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_WRITEMASK, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_WRITEMASK).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_WRITEMASK).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_WRITEMASK, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_WRITEMASK));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_USERWRITEMASK")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_UInt32>(0));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_USERWRITEMASK, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERWRITEMASK).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERWRITEMASK).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_USERWRITEMASK, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERWRITEMASK));
                }

                SUBCASE("NodeID(ns=2;i=21), attr = UA_ATTRIBUTEID_ISABSTRACT")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(true));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=21"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_ISABSTRACT, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ISABSTRACT).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ISABSTRACT).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_ISABSTRACT, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ISABSTRACT));
                }

                SUBCASE("NodeID(ns=2;i=28), attr = UA_ATTRIBUTEID_SYMMETRIC")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(true));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=28"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_SYMMETRIC, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_SYMMETRIC).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_SYMMETRIC).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_SYMMETRIC, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_SYMMETRIC));
                }

                SUBCASE("NodeID(ns=2;i=29), attr = UA_ATTRIBUTEID_INVERSENAME")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "EventSourceOf"), UA_TYPES_LOCALIZEDTEXT));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=29"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_INVERSENAME, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_INVERSENAME).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_INVERSENAME).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_INVERSENAME, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_INVERSENAME));
                }

                SUBCASE("NodeID(ns=2;i=29), attr = UA_ATTRIBUTEID_CONTAINSNOLOOPS - NO VIEW")
                {
                    // todo В момент разработки не нашел Viewe для проверки. Проверяю на отсутствие данных
                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=29"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_CONTAINSNOLOOPS, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    CHECK_FALSE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_CONTAINSNOLOOPS).has_value());
                }

                SUBCASE("NodeID(ns=2;i=1), attr = UA_ATTRIBUTEID_EVENTNOTIFIER")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Byte>(UA_EVENTNOTIFIERTYPE_NONE));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_EVENTNOTIFIER, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EVENTNOTIFIER).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EVENTNOTIFIER).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_EVENTNOTIFIER, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EVENTNOTIFIER));
                }

                SUBCASE("attr = UA_ATTRIBUTEID_VALUE")
                {
                    // todo Добавить все типы поддерживаемых узлов на тестовый сервер и добавить сами тесты
                    SUBCASE("NodeID(ns=2;i=2) Double")
                    {
                        constexpr UA_Double test = 45.52951; // NOLINT
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=2;i=4) String")
                    {
                        char* test = "speed"; // NOLINT
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(UATypesContainer<UA_String>(UA_STRING(test), UA_TYPES_STRING)));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=4"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=2;i=6) Int64")
                    {
                        constexpr UA_Int64 test = 311;
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=6"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=3;i=101) Boolean")
                    {
                        constexpr UA_Boolean test = true;
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=3;i=101"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=2;i=35) String Array")
                    {
                        std::vector<UATypesContainer<UA_String>> array;
                        array.emplace_back(UA_STRING("21433"), UA_TYPES_STRING);
                        array.emplace_back(UA_STRING("test 1"), UA_TYPES_STRING);
                        array.emplace_back(UA_STRING("test 2"), UA_TYPES_STRING);
                        array.emplace_back(UA_STRING("Тест русской локали 3"), UA_TYPES_STRING);
                        MultidimensionalArray<UATypesContainer<UA_String>> test{std::move(array)};
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=35"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=2;i=33) DateTime")
                    {
                        UATypesContainer<UA_DateTime> test(UA_DateTime_fromStruct({0, 0, 0, 55, 12, 16, 13, 11, 2024}), UA_TYPES_DATETIME);
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=33"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }

                    SUBCASE("NodeID(ns=2;i=34) UInt32 Array")
                    {
                        MultidimensionalArray<UA_UInt32> test{{21433, 121433, 8423}};
                        auto test_loca_data = std::optional<VariantsOfAttr>(VariantsOfAttr(test));
                        auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=34"), UA_TYPES_EXPANDEDNODEID);
                        node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUE, std::nullopt}}}));
                        CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                        REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).has_value());
                        MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE).value()));
                        CheckAttrValueEqual(UA_ATTRIBUTEID_VALUE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUE));
                    }
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_DATATYPE")
                {

                    auto test_loca_data = std::optional<VariantsOfAttr>(UATypesContainer<UA_NodeId>(UA_NODEID("i=11"), UA_TYPES_NODEID));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_DATATYPE, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPE).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPE).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_DATATYPE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPE));
                }

                SUBCASE("NodeID(ns=2;i=13), attr = UA_ATTRIBUTEID_VALUERANK")
                {

                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Int32>(UA_VALUERANK_ONE_OR_MORE_DIMENSIONS));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=13"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_VALUERANK, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUERANK).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUERANK).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_VALUERANK, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_VALUERANK));
                }

                SUBCASE("NodeID(ns=2;i=22), attr = UA_ATTRIBUTEID_ARRAYDIMENSIONS")
                {

                    auto test_loca_data = std::optional<VariantsOfAttr>(MultidimensionalArray<UA_UInt32>{{3}});

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=22"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ARRAYDIMENSIONS).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ARRAYDIMENSIONS).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_ARRAYDIMENSIONS, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ARRAYDIMENSIONS));
                }

                SUBCASE("NodeID(ns=2;i=3), attr = UA_ATTRIBUTEID_ACCESSLEVEL")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=3"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_ACCESSLEVEL, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ACCESSLEVEL).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ACCESSLEVEL).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_ACCESSLEVEL, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_ACCESSLEVEL));
                }

                SUBCASE("NodeID(ns=2;i=3), attr = UA_ATTRIBUTEID_USERACCESSLEVEL")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=3"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_USERACCESSLEVEL, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERACCESSLEVEL).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERACCESSLEVEL).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_USERACCESSLEVEL, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USERACCESSLEVEL));
                }

                SUBCASE("NodeID(ns=2;i=20), attr = UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Double>(1000));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=20"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL));
                }

                SUBCASE("NodeID(ns=2;i=2), attr = UA_ATTRIBUTEID_HISTORIZING")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(false));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=2"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_HISTORIZING, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_HISTORIZING).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_HISTORIZING).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_HISTORIZING, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_HISTORIZING));
                }

                SUBCASE("NodeID(ns=2;i=12), attr = UA_ATTRIBUTEID_EXECUTABLE")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(true));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=12"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_EXECUTABLE, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EXECUTABLE).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EXECUTABLE).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_EXECUTABLE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EXECUTABLE));
                }

                SUBCASE("NodeID(ns=2;i=12), attr = UA_ATTRIBUTEID_USEREXECUTABLE")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(true));

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=12"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_USEREXECUTABLE, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USEREXECUTABLE).has_value());
                    MESSAGE(VariantsOfAttrToString(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USEREXECUTABLE).value()));
                    CheckAttrValueEqual(UA_ATTRIBUTEID_USEREXECUTABLE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_USEREXECUTABLE));
                }

                SUBCASE("NodeID(i=376), attr = UA_ATTRIBUTEID_DATATYPEDEFINITION (UA_StructureDefinition)")
                {
                    // todo I haven't figured out how to quickly and correctly assemble a test structure, so the test goes according to a shortened scheme.

                    // // I tried to use my node with the data attribute, but for some reason the Python structure generator does not create C code to create this structure on the Server.
                    // Moreover, somehow it is in standard types, which seem to be generated by the same generator. (?)
                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("i=376"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_DATATYPEDEFINITION, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    REQUIRE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPEDEFINITION).has_value());
                    VariantsOfAttr value;
                    CHECK_NOTHROW(value = node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPEDEFINITION).value()); // I take it out of std::options
                    MESSAGE(VariantsOfAttrToString(value));
                    CheckAttrValueEqual(
                        UA_ATTRIBUTEID_DATATYPEDEFINITION,
                        node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPEDEFINITION),
                        node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_DATATYPEDEFINITION));
                }

                SUBCASE("NodeID(ns=2;i=1), attr = NO_ATTR")
                {
                    auto test_loca_data = std::optional<VariantsOfAttr>(std::nullopt);

                    auto node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_id, .attrs = {{UA_ATTRIBUTEID_EXECUTABLE, std::nullopt}}}));
                    CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                    CHECK_FALSE(node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EXECUTABLE).has_value());
                    CheckAttrValueEqual(UA_ATTRIBUTEID_EXECUTABLE, test_loca_data, node_attr_structure_lists.at(0).attrs.at(UA_ATTRIBUTEID_EXECUTABLE));
                }
            }


            SUBCASE("Чтение множества атрибутов одного узла")
            {
                // Preparing the Query Array
                auto test_loca_data = test_nodes_attributes_data.at(UA_NODECLASS_VARIABLE);
                node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = test_loca_data.node_id, .attrs = test_loca_data.attrs_results}));
                auto attr_sum = std::accumulate( // NOLINT(boost-use-ranges)
                    node_attr_structure_lists.begin(),
                    node_attr_structure_lists.end(),
                    0,
                    [](size_t init, NodeAttributesRequestResponse& node_attr)
                    {
                        return init + node_attr.attrs.size();
                    });
                // Query
                CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, attr_sum).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                for (const auto& test_attr : test_loca_data.attrs_results)
                {
                    auto result_value = node_attr_structure_lists.at(0).attrs[test_attr.first];
                    MESSAGE("test attr id: ", test_attr.first);
                    if (result_value.has_value())
                    {
                        MESSAGE("RESULT DATA: ", VariantsOfAttrToString(result_value.value()));
                        MESSAGE("TEST DATA: ", VariantsOfAttrToString(test_attr.second.value()));
                    }
                    else
                    {
                        MESSAGE("No result data");
                    }
                    CHECK_NOTHROW(CheckAttrValueEqual(test_attr.first, test_attr.second, result_value));
                }
            }

            SUBCASE("Reading a Single Attribute of Multiple Nodes")
            {
                // Preparing the Query Array
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    node_attr_structure_lists.emplace_back(NodeAttributesRequestResponse({.exp_node_id = node_test.second.node_id, .attrs = {{UA_ATTRIBUTEID_BROWSENAME, std::nullopt}}}));
                }
                // Query
                CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, node_attr_structure_lists.size()).GetStatus(), StatusResults::Good);
                // For UA_ATTRIBUTEID_VALUE tests
                size_t index = 0;
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    auto result_value = node_attr_structure_lists.at(index).attrs[UA_ATTRIBUTEID_BROWSENAME];
                    MESSAGE("node_id: ", node_test.second.node_id.ToString(), " test attr UA_ATTRIBUTEID_BROWSENAME");
                    if (result_value.has_value())
                    {
                        MESSAGE("RESULT DATA: ", VariantsOfAttrToString(result_value.value()));
                        MESSAGE("TEST DATA: ", VariantsOfAttrToString(node_test.second.attrs_results.at(UA_ATTRIBUTEID_BROWSENAME).value()));
                    }
                    else
                    {
                        MESSAGE("No result data");
                    }
                    CHECK_NOTHROW(CheckAttrValueEqual(UA_ATTRIBUTEID_BROWSENAME, node_test.second.attrs_results.at(UA_ATTRIBUTEID_BROWSENAME), result_value));
                    index++;
                }
            }

            SUBCASE("Reading multiple attributes of many nodes")
            {
                // Preparing the Query Array
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    auto narr = NodeAttributesRequestResponse({.exp_node_id = node_test.second.node_id, .attrs = {}});
                    for (const auto& test_attr : node_test.second.attrs_results)
                    {
                        narr.attrs[test_attr.first] = std::nullopt;
                    }
                    node_attr_structure_lists.push_back(narr);
                }

                auto attr_sum = std::accumulate( // NOLINT(boost-use-ranges)
                    node_attr_structure_lists.begin(),
                    node_attr_structure_lists.end(),
                    0,
                    [](size_t init, NodeAttributesRequestResponse& node_attr)
                    {
                        return init + node_attr.attrs.size();
                    });
                // Запрос
                CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, attr_sum).GetStatus(), StatusResults::Good);
                CHECK_EQ(client_wrapper.ReadNodesAttributes(node_attr_structure_lists, attr_sum).GetStatus(), StatusResults::Good);
                // Reconciliation of results
                size_t index = 0;
                for (const auto& node_test : test_nodes_attributes_data)
                {
                    for (const auto& test_attr : node_test.second.attrs_results)
                    {
                        auto result_value = node_attr_structure_lists.at(index).attrs[test_attr.first];
                        MESSAGE("node_id: ", node_test.second.node_id.ToString(), " test attr id: ", test_attr.first);
                        if (result_value.has_value())
                        {
                            MESSAGE("RESULT DATA: ", VariantsOfAttrToString(result_value.value()));
                            MESSAGE("TEST DATA: ", VariantsOfAttrToString(test_attr.second.value()));
                        }
                        else
                        {
                            MESSAGE("No result data");
                        }
                        CHECK_NOTHROW(CheckAttrValueEqual(test_attr.first, test_attr.second, result_value));
                    }
                    index++;
                }
            }
        }

        REQUIRE(UA_StatusCode_isGood(UA_Client_disconnect(client)));
        UA_Client_delete(client);
        running = false;
        if (server_thread.joinable())
        {
            server_thread.join();
        }
        sleep(1); // NOLINT(clang-analyzer-unix.BlockInCriticalSection)
    }

    /**
     * nodeToBrowse - list of NodeIDs that need to be passed to the OPC UA server
     * max_nodes_per_browse - the maximum number of NodeIDs that the OPC UA server can accept during a Browsing request
     * max_browse_continuation_points - the maximum number of points that the OPC UA server can return after the Browsing operation.
     * These points must be used for further reading through the BrowseNext operation. Each dot is a response to one NodeID request
     * from the Browsing operation. For example: we sent a request from 10 NodeIDs to Browsing, received a partial response based on the sent NodeIDs,
     * if max_browse_continuation_points = 10, then these points are used further for BrowseNext and further reading.
     * requested_max_references_per_node - The client reports how many maximum child NodeIDs it is willing to receive in each requested
     * parent NodeID. For example: requested_max_references_per_node = 10, the client requests references with 50 NodeID, sending
     * given the number of nodes in the Browse operation, the server responds with 50 messages in each of which no more than requested_max_references_per_node
     * links in the form of child NodeIDs. If the server did not have time to send all child links, you must do BrowsingNext.
     * If the client imposes a restriction through requested_max_references_per_node, then in this operation
     * you must pay attention to the server limitation max_browse_continuation_points and send the request as a NodeID in a number less than or equal to
     * the max_browse_continuation_points value.
     */
    TEST_CASE("nodesetexporter::open62541::Open62541ClientWrapper::CalculateBrowseLimit") // NOLINT
    {
        std::vector<size_t> some_data;
        constexpr auto index_size = 15;
        some_data.reserve(index_size);
        for (size_t i = 0; i < index_size; i++)
        {
            some_data.push_back(i);
        }

        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 0, max_browse_continuation_points = 0, requested_max_references_per_node = 0")
        {
            constexpr auto max_nodes_per_browse = 0;
            constexpr auto max_browse_continuation_points = 0;
            constexpr auto requested_max_references_per_node = 0;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, some_data.size());
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 10, max_browse_continuation_points = 0, requested_max_references_per_node = 0")
        {
            constexpr auto max_nodes_per_browse = 10;
            constexpr auto max_browse_continuation_points = 0;
            constexpr auto requested_max_references_per_node = 0;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, max_nodes_per_browse);
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 0, max_browse_continuation_points = 10, requested_max_references_per_node = 0")
        {
            constexpr auto max_nodes_per_browse = 0;
            constexpr auto max_browse_continuation_points = 10;
            constexpr auto requested_max_references_per_node = 0;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, some_data.size());
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 0, max_browse_continuation_points = 10, requested_max_references_per_node = 5")
        {
            constexpr auto max_nodes_per_browse = 0;
            constexpr auto max_browse_continuation_points = 10;
            constexpr auto requested_max_references_per_node = 5;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, max_browse_continuation_points);
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 100, max_browse_continuation_points = 10, requested_max_references_per_node = 5")
        {
            constexpr auto max_nodes_per_browse = 100;
            constexpr auto max_browse_continuation_points = 10;
            constexpr auto requested_max_references_per_node = 5;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, max_browse_continuation_points);
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 5, max_browse_continuation_points = 10, requested_max_references_per_node = 5")
        {
            constexpr auto max_nodes_per_browse = 5;
            constexpr auto max_browse_continuation_points = 10;
            constexpr auto requested_max_references_per_node = 5;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, max_nodes_per_browse);
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 150, max_browse_continuation_points = 100, requested_max_references_per_node = 5")
        {
            constexpr auto max_nodes_per_browse = 150;
            constexpr auto max_browse_continuation_points = 100;
            constexpr auto requested_max_references_per_node = 5;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, some_data.size());
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 150, max_browse_continuation_points = 100, requested_max_references_per_node = 0")
        {
            constexpr auto max_nodes_per_browse = 150;
            constexpr auto max_browse_continuation_points = 100;
            constexpr auto requested_max_references_per_node = 0;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, some_data.size());
        }
        SUBCASE("nodeToBrowse = 15, max_nodes_per_browse = 0, max_browse_continuation_points = 100, requested_max_references_per_node = 0")
        {
            constexpr auto max_nodes_per_browse = 0;
            constexpr auto max_browse_continuation_points = 100;
            constexpr auto requested_max_references_per_node = 0;
            auto result = Open62541ClientWrapper::CalculateBrowseLimit(some_data.size(), max_nodes_per_browse, max_browse_continuation_points, requested_max_references_per_node);
            CHECK_EQ(result, some_data.size());
        }
    }
}