//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "apps/nodesetexporter/Application.h"

#include "include/nodesetexporter/Build.h"
#include "include/nodesetexporter/common/PerformanceTimer.h"
#include "include/nodesetexporter/logger/LogPlugin.h"
#include "include/nodesetexporter/open62541/BrowseOperations.h"

#include <open62541/client.h>

#include <boost/bind/bind.hpp>

#include <iostream>

namespace apps::nodesetexporter
{
namespace prog_opt = boost::program_options;
namespace build = build;
namespace browseoperations = ::nodesetexporter::open62541::browseoperations;

using EncoderTypes = ::nodesetexporter::common::EncoderTypes;
using ::nodesetexporter::ExportNodesetFromClient;
using ::nodesetexporter::common::PerformanceTimer;

#pragma region Helper_methods

void Application::PrintHelp(std::ostream& out, const boost::program_options::options_description& options_description) const
{
    out << "Usage: " << m_args[0] << " [options]" << std::endl;
    out << options_description;
}

void Application::PrintVersion(std::ostream& out) const
{
    out << "Application version: " << build::version << std::endl;
    out << "Git hash: " << build::git_revision << std::endl;
    out << "Compiler: " << build::compiler << std::endl;
    out << "Build type: " << build::build_type << std::endl;
}

int Application::OptionsCliPars()
{
    prog_opt::options_description cli_options("Options");
    cli_options.add_options()("help,h", "Show hints");
    cli_options.add_options()("version,v", "Show version");
    cli_options.add_options()("endpoint,e", boost::program_options::value<>(&m_client_endpointUrl)->default_value("opc.tcp://localhost:4840"), "Endpoint to OPC UA Server");
    cli_options.add_options()(
        "nodeids,n",
        boost::program_options::value<>(&m_start_node_ids)->required()->multitoken(),
        R"(The IDs of the nodes from which the export will be started. For example: "ns=2;i=1" "ns=2;s=test")");
    cli_options.add_options()("file,f", boost::program_options::value<>(&m_export_filename)->default_value("nodeset_export.xml")->required(), "Path with filename to export");
    cli_options.add_options()("username,u", boost::program_options::value<>(&m_user_name), "Authentication username");
    cli_options.add_options()("password,p", boost::program_options::value<>(&m_password), "Authentication password");
    cli_options.add_options()("maxnrd,m", boost::program_options::value<>(&m_number_of_max_nodes_to_request_data)->default_value(0), "Number of max nodes to request data");
    cli_options.add_options()("timeout,t", boost::program_options::value<>(&m_client_timeout)->default_value(client_timeout_default_ms), "Response timeout in ms");
    cli_options.add_options()("perftimer", boost::program_options::value<>(&m_perf_timer)->default_value(false), "Enable the performance timer (true/false)");
    cli_options.add_options()(
        "parent",
        boost::program_options::value<>(&m_parent_start_node_replacer),
        "The parent node ID of all of the start nodes, which is replaced by the custom one for the binding. default: \"i=85\"");

    prog_opt::variables_map var_map;
    try
    {
        prog_opt::store(prog_opt::command_line_parser(static_cast<int>(m_args.size()), m_args.data()).options(cli_options).run(), var_map);
        prog_opt::notify(var_map);
    }
    catch (boost::program_options::unknown_option& e) // Error reading command line parameters.
    {
        std::cout << e.what() << std::endl;
        PrintHelp(std::cerr, cli_options);
        return info_print;
    }
    catch (boost::program_options::required_option& e) // If a required value is not entered
    {
        if (var_map.count("version") != 0U)
        {
            PrintVersion(std::cout);
        }
        else if (var_map.count("help") != 0U)
        {
            PrintHelp(std::cerr, cli_options);
        }
        else
        {
            std::cout << e.what() << std::endl;
            PrintHelp(std::cerr, cli_options);
        }
        return info_print;
    }
    // Display the application version.
    if (var_map.contains("version"))
    {
        PrintVersion(std::cout);
        return info_print;
    }

    // Display information on setting the application mode.
    if (var_map.contains("help"))
    {
        PrintHelp(std::cout, cli_options);
        return info_print;
    }
    return input_param;
}


void Application::SignalSet()
{
    // Setting up signal processing to stop the connector
    m_signal_set.clear();
    m_signal_set.add(SIGINT);
    m_signal_set.add(SIGTERM);

    m_signal_set.async_wait(
        [&](boost::system::error_code const& error_code, int signal_number)
        {
            if (error_code.value() == boost::system::errc::operation_canceled)
            {
                return;
            }
            if (error_code.failed())
            {
                m_logger_main.Error("Unexpected error while processing signals: {}", error_code.message());
                return;
            }
            switch (signal_number)
            {
            case SIGINT:
            case SIGTERM:
                m_logger_main.Warning("Stop signal received.");
                if (m_client != nullptr)
                {
                    UA_Client_disconnect(m_client);
                }
                m_io_context.stop();
                break;

            default:
                m_logger_main.Warning("An unexpected signal was received: {}", signal_number);
                break;
            }
        });
}

#pragma endregion Helper_methods

void Application::StartExportInAnotherThread()
{
    std::promise<int> promise;
    m_future_thread_result = promise.get_future();
    m_export_thread = std::thread(
        [this](std::promise<int>&& return_res)
        {
            try
            {
                // The first main operation is collecting units for export. Can take a long time.
                m_logger_main.Info("Browse node lists for export");
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>> node_ids_export;

                for (const auto& start_node_id_s : m_start_node_ids)
                {
                    std::vector<UATypesContainer<UA_ExpandedNodeId>> export_node_id_list;
                    UATypesContainer<UA_ExpandedNodeId> start_node_id(UA_EXPANDEDNODEID(start_node_id_s.data()), UA_TYPES_EXPANDEDNODEID);
                    auto perf_timer = PerformanceTimer();
                    auto client_result = browseoperations::GrabChildNodeIdsFromStartNodeId(m_client, start_node_id, export_node_id_list);
                    m_logger_main.Info("Browsing operation from starting NodeID '{}': {}", start_node_id_s, PerformanceTimer::TimeToString(perf_timer.GetTimeElapsed()));
                    if (!UA_StatusCode_isGood(client_result))
                    {
                        throw std::runtime_error("Browsing error: " + std::string(UA_StatusCode_name(client_result)));
                    }

                    // If work is interrupted while browsing, I do not start the export and exit
                    UA_SessionState session_state = UA_SessionState::UA_SESSIONSTATE_CLOSED;
                    UA_Client_getState(m_client, nullptr, &session_state, nullptr);
                    if (session_state == UA_SessionState::UA_SESSIONSTATE_CLOSED || session_state == UA_SessionState::UA_SESSIONSTATE_CLOSING)
                    {
                        throw InterruptException("Interrupt detected.");
                    }
                    node_ids_export.emplace(start_node_id_s, std::move(export_node_id_list));
                }

                // Search for starting nodes in the list of nodes for export.
                auto perf_timer = PerformanceTimer();
                if (CheckStartNodeCrossing(node_ids_export) != StatusResults::Good)
                {
                    throw std::runtime_error("Export error");
                }
                m_logger_main.Info("Check start nodes crossing operation: {}", PerformanceTimer::TimeToString(perf_timer.GetTimeElapsed()));

                // The second main operation is export. Nodesetexporter library function. Can take a long time.
                m_logger_main.Info("Launch export");
                auto nodeexporter_status = ExportNodesetFromClient(*m_client, node_ids_export, std::move(m_export_filename), std::nullopt, m_opt);
                if (nodeexporter_status != StatusResults::Good)
                {
                    throw std::runtime_error("Export error");
                }
            }
            catch (InterruptException& e)
            {
                m_logger_main.Warning("{}", e.what());
                // success
            }
            catch (std::exception& e)
            {
                m_logger_main.Critical("{}", e.what());
                m_io_context.stop();
                return_res.set_value(fail);
                return; // fail
            }
            m_io_context.stop();
            return_res.set_value(success);
            // success
        },
        std::move(promise));
}

StatusResults Application::CheckStartNodeCrossing(std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>& node_ids)
{
    for (const auto& start_nodeids : node_ids)
    {
        for (const auto& lists_of_nodes_to_filtering : node_ids)
        {
            if (start_nodeids.first == lists_of_nodes_to_filtering.first) // The search occurs only in lists where the starting node is not equal to itself.
            {
                continue;
            }
            for (const auto& nodes_to_filtering : lists_of_nodes_to_filtering.second)
            {
                if (UA_ExpandedNodeId_equal(&nodes_to_filtering.GetRef(), &start_nodeids.second.at(0).GetRef()))
                {
                    m_logger_main.Error(
                        "Start NodeID '{}' was found in other node list where Start NodeID is '{}'. Please remove one of the specified starting nodes from the configuration parameters.",
                        start_nodeids.first,
                        lists_of_nodes_to_filtering.first);
                    return StatusResults::Fail;
                }
            }
        }
    }
    return StatusResults::Good;
}

int Application::Run()
{
    try
    {
        UA_StatusCode client_result = UA_STATUSCODE_GOOD;

        // Command Line Options
        if (OptionsCliPars() == info_print)
        {
            return EXIT_SUCCESS;
        }

        // Preparing auxiliary export options
        m_opt.logger = m_opc_nodesetexporter_logger;
        m_opt.number_of_max_nodes_to_request_data = m_number_of_max_nodes_to_request_data;
        m_opt.internal_log_level = LogLevel::Off; // Internal logger is not used
        m_opt.is_perf_timer_enable = m_perf_timer;
        if (!m_parent_start_node_replacer.empty())
        {
            m_opt.parent_start_node_replacer = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID(m_parent_start_node_replacer.c_str()), UA_TYPES_EXPANDEDNODEID);
            if (UA_NodeId_isNull(&m_opt.parent_start_node_replacer.GetRef().nodeId))
            {
                m_logger_main.Error("Invalid parameter \"--parent\".  Check it and try again.");
                return EXIT_FAILURE;
            }
        }

        m_logger_main.Info("Installing a signal handler");
        SignalSet();

        m_client = UA_Client_new();
        m_logger_main.Info("Configuration the Open62541 Client");
        auto* cli_config = UA_Client_getConfig(m_client);
        cli_config->logger = ::nodesetexporter::logger::Open62541LogPlugin::Open62541LoggerCreator(m_opc_ua_client_logger);
        UA_ClientConfig_setDefault(cli_config);
        cli_config->timeout = m_client_timeout;

        m_logger_main.Info("Connecting a Client to a Server");
        if (m_user_name.empty())
        {
            client_result = UA_Client_connect(m_client, m_client_endpointUrl.data());
        }
        else
        {
            client_result = UA_Client_connectUsername(m_client, m_client_endpointUrl.data(), m_user_name.data(), m_password.data());
        }
        if (!UA_StatusCode_isGood(client_result))
        {
            m_logger_main.Error("OPC UA Client error: {}", UA_StatusCode_name(client_result));
            return EXIT_FAILURE;
        }

        // Sending a task for execution to the thread queue context
        m_io_context.post(
            [this]
            {
                StartExportInAnotherThread();
            });

        m_logger_main.Info("Entering a processing loop");
        m_io_context.run();

        if (m_export_thread.joinable())
        {
            m_export_thread.join();
        }

        if (m_client != nullptr)
        {
            UA_Client_delete(m_client);
            m_client = nullptr;
        }
        m_logger_main.Info("I`m leaving...");


        if (m_future_thread_result.valid() && m_future_thread_result.get() == fail)
        {
            return EXIT_FAILURE;
        }
    }
    catch (std::exception& e)
    {
        m_logger_main.Critical("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace apps::nodesetexporter