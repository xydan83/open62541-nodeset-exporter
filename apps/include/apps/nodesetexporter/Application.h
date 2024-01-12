//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef APPS_NODESETEXPORTER_APPLICATION_H
#define APPS_NODESETEXPORTER_APPLICATION_H


#include "include/nodesetexporter/NodesetExporter.h"
#include "include/nodesetexporter/logger/StdLog.h"
#include "include/nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/client_config_default.h>

#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <future>
#include <span>
#include <thread>

namespace apps::nodesetexporter
{

using StatusResults = ::nodesetexporter::StatusResults;
using ::nodesetexporter::open62541::UATypesContainer;

class InterruptException : public std::runtime_error
{
public:
    InterruptException() = delete;
    explicit InterruptException(const std::string& arg)
        : runtime_error(arg)
    {
    }
};

class Application // NOLINT(cppcoreguidelines-special-member-functions)
{
    // OptionsCliPars method statuses
    constexpr static int info_print = 0;
    constexpr static int input_param = 1;

    // Return statuses from functions
    constexpr static int success = 0;
    constexpr static int fail = 1;

    constexpr static uint32_t client_timeout_default_ms = 5000;

    using Logger = ::nodesetexporter::logger::ConsoleLogger;
    using LogLevel = ::nodesetexporter::common::LogLevel;

public:
    Application() = delete;

    explicit Application(std::span<const char*> args)
        : m_args(args)
        , m_logger_main("logger main")
        , m_signal_set(m_io_context)
        , m_opc_nodesetexporter_logger("logger nodesetexporter")
        , m_opc_ua_client_logger("opc-ua-client")
        , m_client(nullptr)
    {
        m_logger_main.SetLevel(LogLevel::Info);
        m_opc_nodesetexporter_logger.SetLevel(LogLevel::Info);
        m_opc_ua_client_logger.SetLevel(LogLevel::Info);
    }

    ~Application()
    {
        if (m_client != nullptr)
        {
            UA_Client_delete(m_client);
        }
    }

#pragma region Helper_methods
public:
    /**
     * @brief Displays hints for working with application parameters on the command line.
     */
    void PrintHelp(std::ostream& out, boost::program_options::options_description const& options_description) const;

    /**
     * @brief Displays the application version and related information.
     */
    void PrintVersion(std::ostream& out) const;

    /**
     * @brief Method for parsing command line parameters.
     * @return INFO_PRINT - in the case of displaying help information, INPUT_PARAM - in the case of working with parameters.
     */
    int OptionsCliPars();

    /**
     * @brief Signal processing setup.
     */
    void SignalSet();

#pragma endregion Helper_methods

private:
    /**
     * @brief A method to run the main export task in a separate thread so that signals can be monitored on the main thread.
     */
    void StartExportInAnotherThread();

    /**
     * @brief Checking the list of starting nodes to see if they are in the hierarchy of the list of other starting nodes.
     * Filtering for NodeID intersection. Search for the presence of a starting node in any list of already collected NodeIDs (Except for the list of the searched node).
     * If such a NodeID is detected, this means that it is already part of the nodes of some previous list and does not need to be processed again.
     *  @param node_ids List of starting nodes at which intersections will be searched.
     * @return The result of the operation.
     */
    StatusResults CheckStartNodeCrossing(std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>& node_ids);

public:
    /**
     * @brief Initialization and startup process.
     * @return EXIT_SUCCESS in case of normal shutdown or EXIT_FAILURE.
     */
    int Run();

private:
    std::span<const char*> const m_args;

    std::thread m_export_thread;
    std::future<int> m_future_thread_result;

    // The context is needed to wait for the force shutdown signal in the main thread.
    // For this, the main export loop will be executed in a separate thread.
    boost::asio::io_context m_io_context;
    boost::asio::signal_set m_signal_set;

    // Logging objects
    Logger m_logger_main;
    Logger m_opc_nodesetexporter_logger;
    Logger m_opc_ua_client_logger;

    UA_Client* m_client;

    std::string m_client_endpointUrl{};
    std::vector<std::string> m_start_node_ids{};
    std::string m_user_name{};
    std::string m_password{};
    std::string m_export_filename{};
    std::string m_parent_start_node_replacer{};
    u_int32_t m_number_of_max_nodes_to_request_data{0};
    u_int32_t m_client_timeout{client_timeout_default_ms};
    bool m_perf_timer{false};
    ::nodesetexporter::Options m_opt{};
};

} // namespace apps::nodesetexporter


#endif // APPS_NODESETEXPORTER_APPLICATION_H