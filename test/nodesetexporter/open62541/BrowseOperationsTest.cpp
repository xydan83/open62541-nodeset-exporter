//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/BrowseOperations.h"
#include "LogMacro.h"
#include "nodesetexporter/common/LoggerBase.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/logger/LogPlugin.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include "ex_nodeset.h"

#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>

#include <doctest/doctest.h>

#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
TEST_LOGGER_INIT

using LoggerPlugin = nodesetexporter::logger::Open62541LogPlugin;
using StatusResults = ::nodesetexporter::common::statuses::StatusResults<>;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::browseoperations::GrabChildNodeIdsFromStartNodeId;
using LogLevel = nodesetexporter::common::LogLevel;
using namespace std::literals;

constexpr auto SERVER_START_TIMEOUT = 10s;
volatile std::atomic_bool running = true; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::condition_variable cv_server_started; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex cv_mutex; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Preparing a test server to which the client will connect
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
            // When updating the Open62541 library to commit 6287f35545e397b1a7384906859f5b504db6dc25, changes were made in which the creation of a server via UA_Server_new starts writing logs
            // to std::out, which negatively affects the display of logs in CLION. Therefore, changes have been made - first I create the config, and only then the Server itself.
            // Moreover, when creating a Client there are no such changes.
            UA_ServerConfig config = {0};
            Logger logger("server-test");
#ifdef OPEN62541_VER_1_3
            config.logger = LoggerPlugin::Open62541LoggerCreator(logger);
#elif defined(OPEN62541_VER_1_4)
            logger.SetLevel(LogLevel::Info);
            auto logging = LoggerPlugin::Open62541LoggerCreator(logger);
            config.logging = &logging;
#endif
            CHECK_ERR(UA_ServerConfig_setDefault(&config));
            auto* server = UA_Server_newWithConfig(&config);
            REQUIRE_NE(server, nullptr);
            CHECK_ERR(ex_nodeset(server)); // TEST NODESET LOADER (HARDCODE)
            uint64_t callback_id = 0;
            CHECK_ERR(UA_Server_addTimedCallback(
                server,
                [](UA_Server* /*server*/, void*)
                {
                    const std::lock_guard locker(cv_mutex);
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

} // namespace

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::browseoperations") // NOLINT
    {
        std::unique_lock<std::mutex> locker(cv_mutex);
        running = true;
        auto server_thread = OpcUaServerStart();
        const std::chrono::time_point<std::chrono::system_clock> server_start_timeout = std::chrono::system_clock::now() + SERVER_START_TIMEOUT;
        REQUIRE_EQ(cv_server_started.wait_until(locker, server_start_timeout), std::cv_status::no_timeout); // Ожидаю старта Сервера
        locker.unlock();
        auto* client = UA_Client_new();
        auto* cli_config = UA_Client_getConfig(client);
        Logger cli_logger("client-test");
#ifdef OPEN62541_VER_1_3
        cli_config->logger = LoggerPlugin::Open62541LoggerCreator(cli_logger);
#elif defined(OPEN62541_VER_1_4)
        auto logging = LoggerPlugin::Open62541LoggerCreator(cli_logger);
        cli_config->logging = &logging;
        cli_config->eventLoop->logger = &logging;
#endif
        UA_ClientConfig_setDefault(cli_config);
        REQUIRE(UA_StatusCode_isGood(UA_Client_connect(client, "opc.tcp://localhost:4840")));

        SUBCASE("GrabChildNodeIdsFromStartNodeId")
        {
            SUBCASE("Getting a list of nodes starting from the start node")
            {
                auto startNodeId = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=1"), UA_TYPES_EXPANDEDNODEID);
                std::vector<UATypesContainer<UA_ExpandedNodeId>> out;
                CHECK_EQ(GrabChildNodeIdsFromStartNodeId(client, startNodeId, out).GetStatus(), StatusResults::Good);
                CHECK_NE(out.size(), 0);
                CHECK_EQ(out.size(), 46);

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
        sleep(1); // NOLINT
    }
}