//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/NodesetExporter.h"
#include "XmlHelperFunctions.h"
#include "nodesetexporter/logger/LogPlugin.h"
#include "nodesetexporter/open62541/BrowseOperations.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include "ex_nodeset.h"
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <thread>

namespace
{
using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LoggerPlugin = nodesetexporter::logger::Open62541LogPlugin;
using LogLevel = nodesetexporter::common::LogLevel;
using nodesetexporter::ExportNodesetFromClient;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::browseoperations::GrabChildNodeIdsFromStartNodeId;

std::atomic_bool running = true; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class Logger final : public LoggerBase
{
public:
    explicit Logger(std::string&& logger_name)
        : LoggerBase(std::move(logger_name))
    {
    }

private:
    void VTrace(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VDebug(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VInfo(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VWarning(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VError(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VCritical(std::string&& message) override
    {
        MESSAGE(message);
    }
};

// Preparing a test server to which the client will connect
auto OpcUaServerStart()
{
    return std::thread(
        []
        {
            MESSAGE("Server start.");
            // Since when updating the Open62541 library to commit 6287f35545e397b1a7384906859f5b504db6dc25, changes were made in which the creation of a server via UA_Server_new
            // starts writing the log to std::out, which negatively affects the display of logs in CLION. Therefore, changes have been made - first I create the config,
            // and only then the Server itself. Moreover, when creating a Client there are no such changes.
            UA_ServerConfig config = {nullptr};
            Logger logger("server-test");
            config.logger = LoggerPlugin::Open62541LoggerCreator(logger);
            auto retval = UA_ServerConfig_setDefault(&config);
            REQUIRE_EQ(retval, UA_STATUSCODE_GOOD);
            auto* server = UA_Server_newWithConfig(&config);
            MESSAGE(std::string(UA_StatusCode_name(retval)));
            REQUIRE(UA_StatusCode_isGood(retval));
            REQUIRE(UA_StatusCode_isGood(ex_nodeset(server))); // TEST NODESET LOADER (HARDCODE)
            REQUIRE(UA_StatusCode_isGood(UA_Server_run(server, reinterpret_cast<volatile const bool*>(&running)))); // NOLINT
            UA_Server_delete(server);
            MESSAGE("Server down.");
        });
}

/**
 * @brief Method for checking the main elements of the generated test case with two starting nodes.
 * @param namespaces List of nodes to compare.
 * @param aliases List of type aliases and references to compare.
 * @param parser The parser object into which the XML elements should be loaded.
 */
void CheckElements( // NOLINT(google-readability-function-size,readability-function-size)
    const std::vector<std::string>& namespaces,
    const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases,
    xmlpp::DomParser& parser)
{
    std::string xpath;
    std::string log_message;
    xmlpp::Node::NodeSet xml_nodes;


#pragma region UANodeSet
    xpath = "//xmlns:UANodeSet"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1); // UANodeSet is only one
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UANodeSet"));
        MESSAGE(log_message);
    }
#pragma endregion UANodeSet

#pragma region NamespaceUris
    // Item-by-Item Validation
    xpath = "//xmlns:UANodeSet/xmlns:NamespaceUris"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "NamespaceUris"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:NamespaceUris/xmlns:Uri"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), namespaces.size());
    if (!xml_nodes.empty())
    {
        for (size_t index = 0; index < namespaces.size(); index++)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Uri", namespaces[index]));
            MESSAGE(log_message);
            log_message.clear();
        }
    }
#pragma endregion NamespaceUris

#pragma region Aliases
    xpath = "//xmlns:UANodeSet/xmlns:Aliases"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:Aliases/xmlns:Alias"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 12);
    if (!xml_nodes.empty())
    {
        size_t index = 0;
        for (const auto& alias : aliases)
        {
            // Map sorts by key, the order will be saved in XML, and then it will also be transferred to std::vector when searching, based on this we can use the index to synchronize the test and
            // element under test.
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Alias", alias.second.ToString(), std::map<std::string, std::string>({{"Alias", alias.first}})));
            index++;
            MESSAGE(log_message);
            log_message.clear();
        }
    }
#pragma endregion Aliases

#pragma region UAObject
    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=1"}, {"BrowseName", "2:vPLC1"}, {"ParentNodeId", "i=85"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(
            CheckXMLNode(log_message, xml_nodes[1], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=5"}, {"BrowseName", "2:myNewStaticObject1"}, {"ParentNodeId", "ns=2;i=1"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message, xml_nodes[2], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=9"}, {"BrowseName", "2:myNewStaticObject1_1"}, {"ParentNodeId", "ns=2;i=5"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message, xml_nodes[3], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=17"}, {"BrowseName", "2:myNewStaticObject1_2"}, {"ParentNodeId", "ns=2;i=5"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message, xml_nodes[4], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=15"}, {"BrowseName", "2:myNewStaticObject1_1_1"}, {"ParentNodeId", "ns=2;i=9"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[5], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=3;i=100"}, {"BrowseName", "3:vPLC2"}, {"ParentNodeId", "i=85"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "vPLC1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "myNewStaticObject1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "DisplayName", "myNewStaticObject1_1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "DisplayName", "myNewStaticObject1_2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "DisplayName", "myNewStaticObject1_1_1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[5], "DisplayName", "vPLC2"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:Description"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description vPLC1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Description", "Description myNewStaticObject1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Description", "Description myNewStaticObject1_1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Description", "Description myNewStaticObject1_2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Description", "Description myNewStaticObject1_1_1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[5], "Description", "Description. Testing multiple start nodes."));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 38);
#pragma endregion UAObject

#pragma region UAVariable
    xpath = "//xmlns:UANodeSet/xmlns:UAVariable"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 15);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[0],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=3"}, {"BrowseName", "2:pressure"}, {"ParentNodeId", "ns=2;i=1"}, {"DataType", "Double"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[1],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=4"}, {"BrowseName", "2:pumpsetting"}, {"ParentNodeId", "ns=2;i=1"}, {"DataType", "String"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[2],
            "UAVariable",
            "",
            std::map<std::string, std::string>(
                {{"NodeId", "ns=2;i=22"}, {"BrowseName", "EnumValues"}, {"ParentNodeId", "ns=2;i=1"}, {"DataType", "EnumValueType"}, {"ValueRank", "1"}, {"ArrayDimensions", "3"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[3],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=2"}, {"BrowseName", "2:temperature"}, {"ParentNodeId", "ns=2;i=1"}, {"DataType", "Double"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[4],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=8"}, {"BrowseName", "2:static_param3"}, {"ParentNodeId", "ns=2;i=5"}, {"DataType", "Double"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[5],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=7"}, {"BrowseName", "2:static_text_param2"}, {"ParentNodeId", "ns=2;i=5"}, {"DataType", "String"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[6],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=6"}, {"BrowseName", "2:static_param1"}, {"ParentNodeId", "ns=2;i=5"}, {"DataType", "Int64"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[7],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=10"}, {"BrowseName", "2:static_param1"}, {"ParentNodeId", "ns=2;i=9"}, {"DataType", "Int64"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[8],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=11"}, {"BrowseName", "2:MyProperty"}, {"ParentNodeId", "ns=2;i=9"}, {"DataType", "Double"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[9],
            "UAVariable",
            "",
            std::map<std::string, std::string>(
                {{"NodeId", "ns=2;i=20"}, {"BrowseName", "2:static_param3"}, {"ParentNodeId", "ns=2;i=17"}, {"DataType", "Double"}, {"ValueRank", "-2"}, {"MinimumSamplingInterval", "1000"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[10],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=19"}, {"BrowseName", "2:static_text_param2"}, {"ParentNodeId", "ns=2;i=17"}, {"DataType", "String"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[11],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=18"}, {"BrowseName", "2:static_param1"}, {"ParentNodeId", "ns=2;i=17"}, {"DataType", "Int64"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[12],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=16"}, {"BrowseName", "2:MyProperty2"}, {"ParentNodeId", "ns=2;i=15"}, {"DataType", "String"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[13],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=3;i=102"}, {"BrowseName", "3:integer32"}, {"ParentNodeId", "ns=3;i=100"}, {"DataType", "Int32"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[14],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=3;i=101"}, {"BrowseName", "3:boolean"}, {"ParentNodeId", "ns=3;i=100"}, {"DataType", "Boolean"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 15);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "pressure"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "pumpsetting"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "DisplayName", "EnumValues"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "DisplayName", "temperature"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "DisplayName", "static_param3"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[5], "DisplayName", "static_text_param2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[6], "DisplayName", "static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[7], "DisplayName", "static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[8], "DisplayName", "MyProperty"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[9], "DisplayName", "static_param3"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[10], "DisplayName", "static_text_param2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[11], "DisplayName", "static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[12], "DisplayName", "MyProperty2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[13], "DisplayName", "integer32"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[14], "DisplayName", "boolean"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:Description"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 14);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description pressure"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Description", "Description pumpsetting"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Description", "Description temperature"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Description", "Description static_param3"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Description", "Description static_text_param2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[5], "Description", "Description static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[6], "Description", "Description static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[7], "Description", "Description MyProperty"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[8], "Description", "Description static_param3"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[9], "Description", "Description static_text_param2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[10], "Description", "Description static_param1"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[11], "Description", "Description MyProperty2"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[12], "Description", "Description integer32"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[13], "Description", "Description boolean"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 15);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 33);
#pragma endregion UAVariable

#pragma region UAReferenceType
    xpath = "//xmlns:UANodeSet/xmlns:UAReferenceType"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(
            CheckXMLNode(log_message, xml_nodes[0], "UAReferenceType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=30"}, {"BrowseName", "2:HasChild_test"}, {"IsAbstract", "true"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "UAReferenceType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=29"}, {"BrowseName", "2:HasEventSource_test"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[2],
            "UAReferenceType",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=28"}, {"BrowseName", "2:SomeReferences_test"}, {"IsAbstract", "true"}, {"Symmetric", "true"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAReferenceType/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "HasChild_test"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "HasEventSource_test"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "DisplayName", "SomeReferences_test"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAReferenceType/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAReferenceType/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
#pragma endregion UAReferenceType

#pragma region UAObjectType
    xpath = "//xmlns:UANodeSet/xmlns:UAObjectType"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UAObjectType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=24"}, {"BrowseName", "2:FolderType_test"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message, xml_nodes[1], "UAObjectType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=23"}, {"BrowseName", "2:AggregateConfigurationType_test"}, {"IsAbstract", "true"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObjectType/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "FolderType_test"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "AggregateConfigurationType_test"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObjectType/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObjectType/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
#pragma endregion UAObjectType

#pragma region UADataType
    xpath = "//xmlns:UANodeSet/xmlns:UADataType"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UADataType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=21"}, {"BrowseName", "2:Union"}, {"IsAbstract", "true"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "UADataType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=31"}, {"BrowseName", "2:Union_concrete"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "UADataType", "", std::map<std::string, std::string>({{"NodeId", "ns=2;i=32"}, {"BrowseName", "KeyValuePair_test"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UADataType/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "Union"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "Union concrete"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "DisplayName", "KeyValuePair_test"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UADataType/xmlns:Description"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description Union"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UADataType/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 3);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UADataType/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 6);
#pragma endregion UADataType

#pragma region UAVariableType
    xpath = "//xmlns:UANodeSet/xmlns:UAVariableType"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[0],
            "UAVariableType",
            "",
            std::map<std::string, std::string>(
                {{"NodeId", "ns=2;i=27"},
                 {"BrowseName", "2:SamplingIntervalDiagnosticsArrayType_test"},
                 {"DataType", "SamplingIntervalDiagnosticsDataType"},
                 {"ValueRank", "1"},
                 {"ArrayDimensions", "0"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[1],
            "UAVariableType",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=2;i=26"}, {"BrowseName", "2:DataTypeDescriptionType_test"}, {"DataType", "String"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariableType/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "SamplingIntervalDiagnosticsArrayType_test"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "DataTypeDescriptionType_test"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariableType/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariableType/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 5);
#pragma endregion UAVariableType
}

/**
 * @brief Method for checking the main elements of the generated test suite with one starting node.
 * @param namespaces List of nodes to compare.
 * @param aliases List of type aliases and references to compare.
 * @param parser The parser object into which the XML elements should be loaded.
 */
void CheckElements2( // NOLINT(google-readability-function-size,readability-function-size)
    const std::vector<std::string>& namespaces,
    const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases,
    xmlpp::DomParser& parser)
{
    std::string xpath;
    std::string log_message;
    xmlpp::Node::NodeSet xml_nodes;


#pragma region UANodeSet
    xpath = "//xmlns:UANodeSet"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1); // UANodeSet is only one
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UANodeSet"));
        MESSAGE(log_message);
    }
#pragma endregion UANodeSet

#pragma region NamespaceUris
    // Item-by-Item Validation
    xpath = "//xmlns:UANodeSet/xmlns:NamespaceUris"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "NamespaceUris"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:NamespaceUris/xmlns:Uri"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), namespaces.size());
    if (!xml_nodes.empty())
    {
        for (size_t index = 0; index < namespaces.size(); index++)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Uri", namespaces[index]));
            MESSAGE(log_message);
            log_message.clear();
        }
    }
#pragma endregion NamespaceUris

#pragma region Aliases
    xpath = "//xmlns:UANodeSet/xmlns:Aliases"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:Aliases/xmlns:Alias"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 5);
    if (!xml_nodes.empty())
    {
        size_t index = 0;
        for (const auto& alias : aliases)
        {
            // Map sorts by key, the order will be saved in XML, and then it will also be transferred to std::vector when searching, based on this we can use the index
            // to synchronize the test and element under test.
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Alias", alias.second.ToString(), std::map<std::string, std::string>({{"Alias", alias.first}})));
            index++;
            MESSAGE(log_message);
            log_message.clear();
        }
    }
#pragma endregion Aliases

#pragma region UAObject
    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=3;i=100"}, {"BrowseName", "3:vPLC2"}, {"ParentNodeId", "i=85"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "vPLC2"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:Description"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description. Testing multiple start nodes."));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 1);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 4);
#pragma endregion UAObject

#pragma region UAVariable
    xpath = "//xmlns:UANodeSet/xmlns:UAVariable"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[0],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=3;i=102"}, {"BrowseName", "3:integer32"}, {"ParentNodeId", "ns=3;i=100"}, {"DataType", "Int32"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(
            log_message,
            xml_nodes[1],
            "UAVariable",
            "",
            std::map<std::string, std::string>({{"NodeId", "ns=3;i=101"}, {"BrowseName", "3:boolean"}, {"ParentNodeId", "ns=3;i=100"}, {"DataType", "Boolean"}, {"ValueRank", "-2"}})));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:DisplayName"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "integer32"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "DisplayName", "boolean"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:Description"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description integer32"));
        MESSAGE(log_message);
        log_message.clear();
        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Description", "Description boolean"));
        MESSAGE(log_message);
        log_message.clear();
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:References"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 2);
    if (!xml_nodes.empty())
    {
        for (const auto& xml_node : xml_nodes)
        {
            CHECK_NOTHROW(CheckXMLNode(log_message, xml_node, "References"));
            MESSAGE(log_message);
            log_message.clear();
        }
    }

    xpath = "//xmlns:UANodeSet/xmlns:UAVariable/xmlns:References/xmlns:Reference"; // Node to be checked
    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
    MESSAGE("Nodes size = ", xml_nodes.size());
    CHECK_EQ(xml_nodes.size(), 4);
#pragma endregion UAVariable
}

} // namespace


TEST_SUITE("nodesetexporter")
{
    TEST_CASE("nodesetexporter::NodesetExporter - one start node (short test)") // NOLINT
    {
        std::string log_message;
        xmlpp::XsdValidator valid("UANodeSet.xsd"); // Schema for XML validation
        xmlpp::DomParser parser;

        running = true;
        auto server_thread = OpcUaServerStart();
        sleep(1);
        auto* client = UA_Client_new();
        auto* cli_config = UA_Client_getConfig(client);
        Logger cli_logger("client-test");
        cli_config->logger = LoggerPlugin::Open62541LoggerCreator(cli_logger);
        UA_ClientConfig_setDefault(cli_config);
        REQUIRE(UA_StatusCode_isGood(UA_Client_connect(client, "opc.tcp://localhost:4840")));

        Logger nodesetexporter_logger("nodesetexporter-test");
        nodesetexporter_logger.SetLevel(LogLevel::Trace);
        std::vector<UATypesContainer<UA_ExpandedNodeId>> node_id_list;

        std::stringstream out_test_buffer;
        std::string xpath;
        xmlpp::Attribute::NodeSet xml_nodes;

        std::vector<std::string> namespaces{"urn:open62541.server.application", "http://test/nodes/1", "http://test/nodes/2"};
        std::map<std::string, UATypesContainer<UA_NodeId>> aliases(
            {{"Boolean", UATypesContainer<UA_NodeId>{UA_NODEID("i=1"), UA_TYPES_NODEID}},
             {"Int32", UATypesContainer<UA_NodeId>{UA_NODEID("i=6"), UA_TYPES_NODEID}},
             {"HasComponent", UATypesContainer<UA_NodeId>{UA_NODEID("i=47"), UA_TYPES_NODEID}},
             {"Organizes", UATypesContainer<UA_NodeId>{UA_NODEID("i=35"), UA_TYPES_NODEID}},
             {"HasTypeDefinition", UATypesContainer<UA_NodeId>{UA_NODEID("i=40"), UA_TYPES_NODEID}}});

        SUBCASE("Export test nodes via OPC UA Client to XML.")
        {
            constexpr auto number_of_max_nodes_to_request_data = 6;
            nodesetexporter::Options opt;
            opt.logger = nodesetexporter_logger;

            SUBCASE("Buffer output.")
            {
                opt.is_perf_timer_enable = true;
                const UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID("ns=3;i=100"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id, node_id_list);
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>> node_id_list_from_start_nodes{{node_id_list[0].ToString(), std::move(node_id_list)}};

                SUBCASE("number_of_max_nodes_to_request_data = 0. PerfTimer = ON")
                {
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    // At some point the error "Line 44, column 1 (fatal): Extra content at the end of the document" appeared.
                    // Swears at the presence of a line feed at the end.
                    // The libxml2 library may have been updated in libxmlpp. I haven't found another solution yet.
                    // Since it is used only for testing - due to its non-criticality - I will introduce a temporary solution.
                    // Similar solutions will exist wherever a buffer is used. There are no such problems when checking files.
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements2(namespaces, aliases, parser);
                }

                SUBCASE("number_of_max_nodes_to_request_data = 6. PerfTimer = ON")
                {
                    opt.number_of_max_nodes_to_request_data = number_of_max_nodes_to_request_data;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements2(namespaces, aliases, parser);
                }
            }

            SUBCASE("Output to a file.")
            {
                opt.is_perf_timer_enable = false;
                const UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID("ns=3;i=100"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id, node_id_list);
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>> node_id_list_from_start_nodes{{node_id_list[0].ToString(), std::move(node_id_list)}};

                constexpr auto filename = "nodeset_test.xml";
                std::string line;
                std::string lines;
                SUBCASE("number_of_max_nodes_to_request_data = 0. PerfTimer = OFF")
                {
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, filename, std::nullopt, opt));
                    std::ifstream in(filename, std::ios_base::in);
                    CHECK(in.is_open());
                    while (std::getline(in, line))
                    {
                        lines += line + "\n";
                    };
                    MESSAGE(lines);

                    CHECK_NOTHROW(parser.parse_file(filename));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    out_test_buffer = std::stringstream(lines);
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements2(namespaces, aliases, parser);
                }

                SUBCASE("number_of_max_nodes_to_request_data = 6. PerfTimer = OFF")
                {
                    opt.number_of_max_nodes_to_request_data = number_of_max_nodes_to_request_data;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, filename, std::nullopt, opt));
                    std::ifstream in(filename, std::ios_base::in);
                    CHECK(in.is_open());
                    while (std::getline(in, line))
                    {
                        lines += line + "\n";
                    };
                    MESSAGE(lines);

                    CHECK_NOTHROW(parser.parse_file(filename));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    out_test_buffer = std::stringstream(lines);
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements2(namespaces, aliases, parser);
                }
                if (std::filesystem::exists(filename))
                {
                    std::remove(filename);
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
    }


    TEST_CASE("nodesetexporter::NodesetExporter - multiple starting nodes") // NOLINT
    {
        std::string log_message;
        xmlpp::XsdValidator valid("UANodeSet.xsd"); // Schema for XML validation
        xmlpp::DomParser parser;

        running = true;
        auto server_thread = OpcUaServerStart();
        sleep(1);
        auto* client = UA_Client_new();
        auto* cli_config = UA_Client_getConfig(client);
        Logger cli_logger("client-test");
        cli_config->logger = LoggerPlugin::Open62541LoggerCreator(cli_logger);
        UA_ClientConfig_setDefault(cli_config);
        REQUIRE(UA_StatusCode_isGood(UA_Client_connect(client, "opc.tcp://localhost:4840")));

        Logger nodesetexporter_logger("nodesetexporter-test");
        nodesetexporter_logger.SetLevel(LogLevel::Trace);
        std::vector<UATypesContainer<UA_ExpandedNodeId>> node_id_list;
        std::vector<UATypesContainer<UA_ExpandedNodeId>> node_id_list2;

        std::stringstream out_test_buffer;
        std::string xpath;
        xmlpp::Attribute::NodeSet xml_nodes;


        std::vector<std::string> namespaces{"urn:open62541.server.application", "http://test/nodes/1", "http://test/nodes/2"};
        std::map<std::string, UATypesContainer<UA_NodeId>> aliases(
            {{"Boolean", UATypesContainer<UA_NodeId>{UA_NODEID("i=1"), UA_TYPES_NODEID}},
             {"Int32", UATypesContainer<UA_NodeId>{UA_NODEID("i=6"), UA_TYPES_NODEID}},
             {"Int64", UATypesContainer<UA_NodeId>{UA_NODEID("i=8"), UA_TYPES_NODEID}},
             {"Double", UATypesContainer<UA_NodeId>{UA_NODEID("i=11"), UA_TYPES_NODEID}},
             {"EnumValueType", UATypesContainer<UA_NodeId>{UA_NODEID("i=7594"), UA_TYPES_NODEID}},
             {"SamplingIntervalDiagnosticsDataType", UATypesContainer<UA_NodeId>{UA_NODEID("i=856"), UA_TYPES_NODEID}},
             {"String", UATypesContainer<UA_NodeId>{UA_NODEID("i=12"), UA_TYPES_NODEID}},
             {"HasProperty", UATypesContainer<UA_NodeId>{UA_NODEID("i=46"), UA_TYPES_NODEID}},
             {"HasComponent", UATypesContainer<UA_NodeId>{UA_NODEID("i=47"), UA_TYPES_NODEID}},
             {"Organizes", UATypesContainer<UA_NodeId>{UA_NODEID("i=35"), UA_TYPES_NODEID}},
             {"HasSubtype", UATypesContainer<UA_NodeId>{UA_NODEID("i=45"), UA_TYPES_NODEID}},
             {"HasTypeDefinition", UATypesContainer<UA_NodeId>{UA_NODEID("i=40"), UA_TYPES_NODEID}}});

        std::map<std::string, UATypesContainer<UA_NodeId>> aliases2(
            {{"Boolean", UATypesContainer<UA_NodeId>{UA_NODEID("i=1"), UA_TYPES_NODEID}},
             {"Int32", UATypesContainer<UA_NodeId>{UA_NODEID("i=6"), UA_TYPES_NODEID}},
             {"HasComponent", UATypesContainer<UA_NodeId>{UA_NODEID("i=47"), UA_TYPES_NODEID}},
             {"Organizes", UATypesContainer<UA_NodeId>{UA_NODEID("i=35"), UA_TYPES_NODEID}},
             {"HasTypeDefinition", UATypesContainer<UA_NodeId>{UA_NODEID("i=40"), UA_TYPES_NODEID}}});

        SUBCASE("Export test nodes via OPC UA Client to XML.")
        {
            constexpr auto number_of_max_nodes_to_request_data = 6;
            nodesetexporter::Options opt;
            opt.logger = nodesetexporter_logger;

            SUBCASE("Buffer output.")
            {
                opt.is_perf_timer_enable = true;
                const UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id, node_id_list);
                const UATypesContainer<UA_ExpandedNodeId> start_node_id2(UA_EXPANDEDNODEID("ns=3;i=100"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id2, node_id_list2);
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>
                    node_id_list_from_start_nodes{{node_id_list[0].ToString(), std::move(node_id_list)}, {node_id_list2[0].ToString(), std::move(node_id_list2)}};

                SUBCASE("number_of_max_nodes_to_request_data = 0. PerfTimer = ON")
                {
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements(namespaces, aliases, parser);
                }

                SUBCASE("number_of_max_nodes_to_request_data = 6. PerfTimer = ON")
                {
                    opt.number_of_max_nodes_to_request_data = number_of_max_nodes_to_request_data;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements(namespaces, aliases, parser);
                }
            }

            SUBCASE("Output to a file.")
            {
                opt.is_perf_timer_enable = false;
                const UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id, node_id_list);
                const UATypesContainer<UA_ExpandedNodeId> start_node_id2(UA_EXPANDEDNODEID("ns=3;i=100"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id2, node_id_list2);
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>
                    node_id_list_from_start_nodes{{node_id_list[0].ToString(), std::move(node_id_list)}, {node_id_list2[0].ToString(), std::move(node_id_list2)}};

                constexpr auto filename = "nodeset_test.xml";
                std::string line;
                std::string lines;
                SUBCASE("number_of_max_nodes_to_request_data = 0. PerfTimer = OFF")
                {
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, filename, std::nullopt, opt));
                    std::ifstream in(filename, std::ios_base::in);
                    CHECK(in.is_open());
                    while (std::getline(in, line))
                    {
                        lines += line + "\n";
                    };
                    MESSAGE(lines);

                    CHECK_NOTHROW(parser.parse_file(filename));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    out_test_buffer = std::stringstream(lines);
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements(namespaces, aliases, parser);
                }

                SUBCASE("number_of_max_nodes_to_request_data = 6. PerfTimer = OFF")
                {
                    opt.number_of_max_nodes_to_request_data = number_of_max_nodes_to_request_data;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, filename, std::nullopt, opt));
                    std::ifstream in(filename, std::ios_base::in);
                    CHECK(in.is_open());
                    while (std::getline(in, line))
                    {
                        lines += line + "\n";
                    };
                    MESSAGE(lines);

                    CHECK_NOTHROW(parser.parse_file(filename));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document
                    out_test_buffer = std::stringstream(lines);
                    // Since there are a lot of elements to compare, I use the function
                    CheckElements(namespaces, aliases, parser);
                }
                if (std::filesystem::exists(filename))
                {
                    std::remove(filename);
                }
            }

            SUBCASE("Checking the binding of the parent of the starting node")
            {
                const UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID("ns=2;i=17"), UA_TYPES_EXPANDEDNODEID);
                GrabChildNodeIdsFromStartNodeId(client, start_node_id, node_id_list);
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>> node_id_list_from_start_nodes{{node_id_list[0].ToString(), std::move(node_id_list)}};

                SUBCASE("Default binding check (i=85)")
                {
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document

                    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 1);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(
                            log_message,
                            xml_nodes[0],
                            "UAObject",
                            "",
                            std::map<std::string, std::string>({{"NodeId", "ns=2;i=17"}, {"BrowseName", "2:myNewStaticObject1_2"}, {"ParentNodeId", "i=85"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 5);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=85", std::map<std::string, std::string>({{"ReferenceType", "Organizes"}, {"IsForward", "false"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=2;i=20", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Reference", "ns=2;i=19", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Reference", "ns=2;i=18", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                    }
                }

                SUBCASE("Default binding check (i=85). number_of_max_nodes_to_request_data = 2")
                {
                    opt.number_of_max_nodes_to_request_data = 2;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document

                    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 1);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(
                            log_message,
                            xml_nodes[0],
                            "UAObject",
                            "",
                            std::map<std::string, std::string>({{"NodeId", "ns=2;i=17"}, {"BrowseName", "2:myNewStaticObject1_2"}, {"ParentNodeId", "i=85"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 5);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=85", std::map<std::string, std::string>({{"ReferenceType", "Organizes"}, {"IsForward", "false"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=2;i=20", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Reference", "ns=2;i=19", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Reference", "ns=2;i=18", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                    }
                }

                SUBCASE("Checking for binding to the specified node in the parameters (ns=2;i=3)")
                {
                    opt.parent_start_node_replacer = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(2, 3), UA_TYPES_EXPANDEDNODEID);
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document

                    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 1);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(
                            log_message,
                            xml_nodes[0],
                            "UAObject",
                            "",
                            std::map<std::string, std::string>({{"NodeId", "ns=2;i=17"}, {"BrowseName", "2:myNewStaticObject1_2"}, {"ParentNodeId", "ns=2;i=3"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 5);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "ns=2;i=3", std::map<std::string, std::string>({{"ReferenceType", "Organizes"}, {"IsForward", "false"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=2;i=20", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Reference", "ns=2;i=19", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Reference", "ns=2;i=18", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                    }
                }

                SUBCASE("Check for binding to the specified node in the parameters (ns=2;i=3). number_of_max_nodes_to_request_data = 2")
                {
                    opt.parent_start_node_replacer = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(2, 3), UA_TYPES_EXPANDEDNODEID);
                    opt.number_of_max_nodes_to_request_data = 2;
                    CHECK_NOTHROW(ExportNodesetFromClient(*client, node_id_list_from_start_nodes, "", out_test_buffer, opt));
                    std::string out_xml(out_test_buffer.str());
                    out_xml.erase(out_xml.rfind('\n'));
                    MESSAGE(out_xml);
                    CHECK_NOTHROW(parser.parse_memory(out_xml));
                    CHECK_NOTHROW(valid.validate(parser.get_document())); // Checking against the schema of the entire document

                    xpath = "//xmlns:UANodeSet/xmlns:UAObject"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 1);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(
                            log_message,
                            xml_nodes[0],
                            "UAObject",
                            "",
                            std::map<std::string, std::string>({{"NodeId", "ns=2;i=17"}, {"BrowseName", "2:myNewStaticObject1_2"}, {"ParentNodeId", "ns=2;i=3"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                    xpath = "//xmlns:UANodeSet/xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
                    CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                    MESSAGE("Nodes size = ", xml_nodes.size());
                    CHECK_EQ(xml_nodes.size(), 5);
                    if (!xml_nodes.empty())
                    {
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "ns=2;i=3", std::map<std::string, std::string>({{"ReferenceType", "Organizes"}, {"IsForward", "false"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=2;i=20", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Reference", "ns=2;i=19", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                        MESSAGE(log_message);
                        log_message.clear();
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[4], "Reference", "ns=2;i=18", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                    }
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
    }
}