//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "NodesetExporter.h"
#include "ClientWrappers.h"
#include "NodesetExporterLoop.h"
#include "PerformanceTimer.h"
#include "ServerWrappers.h"
#include "encoders/XMLEncoder.h"
#include "logger/StdLog.h"


namespace nodesetexporter
{
using Open62541ServerWrapper = nodesetexporter::open62541::Open62541ServerWrapper;
using Open62541ClientWrapper = nodesetexporter::open62541::Open62541ClientWrapper;
using XMLEncoder = nodesetexporter::encoders::XMLEncoder;
using ConsoleLogger = nodesetexporter::logger::ConsoleLogger;
using PerformanceTimer = nodesetexporter::common::PerformanceTimer;

template <typename TOpen62541ServerOrClient>
StatusResults ExportNodeset(
    TOpen62541ServerOrClient& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept
{
    auto logger = opt.logger;
    std::unique_ptr<LoggerBase> default_logger;
    try
    {
        // Select the logging method. If an external object is not provided, the internal implementation will be used.
        if (!logger)
        {
            default_logger = std::make_unique<ConsoleLogger>("nodesetexporter");
            logger = std::reference_wrapper<LoggerBase>(*default_logger);
            logger.value().get().SetLevel(opt.internal_log_level);
        }
        logger.value().get().Trace("Method called: ExportNodeset()");

        if (opt.is_perf_timer_enable)
        {
            logger.value().get().Info("Perf-monitoring mode is enabled...");
        }
    }
    catch (...)
    {
        return StatusResults::Fail;
    }

    if (node_ids.empty())
    {
        logger.value().get().Error("The list of node IDs is empty.");
        return {StatusResults::Fail, StatusResults::SubStatus::EmptyNodeIdList};
    }

    try
    {
        // I check that either the client or server type is passed to open62541_object, otherwise there is a static build error.
        static_assert(std::is_same_v<TOpen62541ServerOrClient, UA_Server> || std::is_same_v<TOpen62541ServerOrClient, UA_Client>, "Wrong type of TOpen62541ServerOrClient");
        // Select the service provider implementation of the Open62541 library
        std::unique_ptr<IOpen62541> uniq_open625411_obj = nullptr;
        if constexpr (std::is_same_v<TOpen62541ServerOrClient, UA_Server>)
        {
            uniq_open625411_obj = std::make_unique<Open62541ServerWrapper>(open62541_object, logger.value().get());
        }
        else if constexpr (std::is_same_v<TOpen62541ServerOrClient, UA_Client>)
        {
            uniq_open625411_obj = std::make_unique<Open62541ClientWrapper>(open62541_object, logger.value().get());
        }
        else
        {
            static_assert("You need to choose between UA_Server or UA_Client....");
        }
        // Set the limits on requests to the OPCUA Server.
        uniq_open625411_obj->SetRequestedMaxReferencesPerNode(opt.max_references_per_node);
        uniq_open625411_obj->SetMaxBrowseContinuationPoints(opt.max_browse_continuation_points);
        uniq_open625411_obj->SetMaxNodesPerBrowse(opt.max_nodes_per_browse);
        uniq_open625411_obj->SetMaxNodesPerRead(opt.max_nodes_per_read);

        // Selects the exporter encoder implementation.
        std::unique_ptr<IEncoder> uniq_encoder;
        switch (opt.encoder_types)
        {
        // So far only one implementation in XML.
        default:
            if (out_buffer)
            {
                uniq_encoder = std::make_unique<XMLEncoder>(logger.value().get(), *out_buffer);
            }
            else
            {
                uniq_encoder = std::make_unique<XMLEncoder>(logger.value().get(), std::move(filename));
            }
        }

        NodesetExporterLoop export_core(
            node_ids,
            *uniq_open625411_obj,
            *uniq_encoder,
            logger.value().get(),
            {.is_perf_timer_enable = opt.is_perf_timer_enable,
             .ns0_custom_nodes_ready_to_work = opt.ns0_custom_nodes_ready_to_work,
             .flat_list_of_nodes =
                 {.is_enable = opt.flat_list_of_nodes.is_enable,
                  .create_missing_start_node = opt.flat_list_of_nodes.create_missing_start_node,
                  .allow_abstract_variable = opt.flat_list_of_nodes.allow_abstract_variable},
             .parent_start_node_replacer = opt.parent_start_node_replacer});

        auto timer = PREPARE_TIMER(opt.is_perf_timer_enable);
        auto status = export_core.StartExport();
        GET_TIME_ELAPSED_FMT_FORMAT(timer, logger.value().get().Info, "Total time to export: ", "");

        return status;
    }
    catch (std::exception& exc)
    {
        logger.value().get().Error("An exception was caught. {}", exc.what());
        return StatusResults::Fail;
    }
}

template StatusResults DLL_PUBLIC ExportNodeset<UA_Server>(
    UA_Server& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept;

template StatusResults DLL_PUBLIC ExportNodeset<UA_Client>(
    UA_Client& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept;

StatusResults DLL_PUBLIC ExportNodesetFromServer(
    UA_Server& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept
{
    return ExportNodeset<UA_Server>(open62541_object, node_ids, std::move(filename), out_buffer, opt);
}

StatusResults DLL_PUBLIC ExportNodesetFromClient(
    UA_Client& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept
{
    return ExportNodeset<UA_Client>(open62541_object, node_ids, std::move(filename), out_buffer, opt);
}
} // namespace nodesetexporter