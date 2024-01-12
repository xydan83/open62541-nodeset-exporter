//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/BrowseOperations.h"
#include "nodesetexporter/common/LoggerBase.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/logger/LogPlugin.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include "ex_nodeset.h"

#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>

#include <doctest/doctest.h>

#include <iostream>
#include <thread>
#include <vector>

namespace
{
using LogerBase = nodesetexporter::common::LoggerBase<std::string>;
using LoggerPlugin = nodesetexporter::logger::Open62541LogPlugin;
using nodesetexporter::common::statuses::StatusResults;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::browseoperations::GrabChildNodeIdsFromStartNodeId;

std::atomic_bool running = true; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class Logger final : public LogerBase
{
public:
    explicit Logger(std::string&& logger_name)
        : LoggerBase<std::string>(std::move(logger_name))
    {
    }

private:
    void VTrace(std::string&& message) override
    {
        INFO(message);
    }
    void VDebug(std::string&& message) override
    {
        INFO(message);
    }
    void VInfo(std::string&& message) override
    {
        INFO(message);
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
            // When updating the Open62541 library to commit 6287f35545e397b1a7384906859f5b504db6dc25, changes were made in which the creation of a server via UA_Server_new starts writing logs
            // to std::out, which negatively affects the display of logs in CLION. Therefore, changes have been made - first I create the config, and only then the Server itself.
            // Moreover, when creating a Client there are no such changes.
            UA_ServerConfig config = {0};
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

} // namespace

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::browseoperations") // NOLINT
    {
        running = true;
        auto server_thread = OpcUaServerStart();
        sleep(1);
        auto* client = UA_Client_new();
        auto* cli_config = UA_Client_getConfig(client);
        Logger cli_logger("client-test");
        cli_config->logger = LoggerPlugin::Open62541LoggerCreator(cli_logger);
        UA_ClientConfig_setDefault(cli_config);
        REQUIRE(UA_StatusCode_isGood(UA_Client_connect(client, "opc.tcp://localhost:4840")));

        SUBCASE("GrabChildNodeIdsFromStartNodeId")
        {
            SUBCASE("Getting a list of nodes starting from the start node")
            {
                auto startNodeId = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                std::vector<UATypesContainer<UA_ExpandedNodeId>> out;
                CHECK_EQ(GrabChildNodeIdsFromStartNodeId(client, startNodeId, out), StatusResults::Good);
                CHECK_NE(out.size(), 0);
                CHECK_EQ(out.size(), 43);

                MESSAGE("Node Id's:");
                for (const auto& node_id : out)
                {
                    MESSAGE(node_id.ToString());
                }
            }

            SUBCASE("Error when the parent node does not exist")
            {
                auto startNodeId = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=3;i=1000"), UA_TYPES_EXPANDEDNODEID);
                std::vector<UATypesContainer<UA_ExpandedNodeId>> out;
                CHECK_EQ(GrabChildNodeIdsFromStartNodeId(client, startNodeId, out), StatusResults::Good);
                // If you send a non-existent starting node to the function, the UA_Client_forEachChildNodeCall function called internally will still return a “good” status,
                // since according to the code it returns it from the UA_BrowseResponse::responseHeader::serviceResult structure, and the status of the Browse operation itself
                // (UA_BrowseResponse::results[i]::statusCode) - ignores.
                // As a result, the GrabAllExportNodes function will return only the starting node.
                CHECK_EQ(out.size(), 1);
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
}