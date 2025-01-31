//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

// Entry point, functions for export, creation of an exporter chain

#ifndef NODESETEXPORTER_NODESETEXPORTER_H
#define NODESETEXPORTER_NODESETEXPORTER_H

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllexport))
#else
#define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((dllimport))
#else
#define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define DLL_LOCAL
#else
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__((visibility("default")))
#define DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define DLL_PUBLIC
#define DLL_LOCAL
#endif
#endif

#include "Encoder_types.h"
#include "LoggerBase.h"
#include "Statuses.h"
#include "UATypesContainer.h"

#include <open62541/client.h>
#include <open62541/server.h>

#include <map>
#include <optional>
#include <vector>

namespace nodesetexporter
{
using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using EncoderTypes = nodesetexporter::common::EncoderTypes;
using ExpandedNodeId = nodesetexporter::open62541::UATypesContainer<UA_ExpandedNodeId>;
using LogLevel = nodesetexporter::common::LogLevel;
using StatusResults = nodesetexporter::common::statuses::StatusResults<int64_t>;

/**
 * @brief Additional export options
 * @param logger External logging object. If absent, the internal standard output mechanism will be used. [optional]
 * @param number_of_max_nodes_to_request_data The maximum number of nodes for which data must be received from the server in one request. Default - no limit [optional]
 * @param encoder_types The upload encoding type. By default, XML encoding is used. [optional]
 * @param internal_log_level The logging level of the internal logging engine. The default is Info level. [optional]
 * @param parent_start_node_replacer ID of the node that will be substituted as the main parent for the start node if the parent of the start node is not ns=0;i=85.
 *                                   By default, the parent of the starting node will be the node with NodeId i=85.
 *                                   For encoder_types XML, both the ParentNodeID and the reference to the main parent are changed. [optional]
 * @param perf_counter_enable Enable performance counters and output the results to the log. The result is displayed in the Info level mode. [optional]
 * @param ns0_custom_nodes_ready_to_work Export user nodes located in the standard OPC UA space (ns=0). [optionally] [experimental]
 * @param flat_list_of_nodes__is_enable Generate a flat list of nodes linked to a single start node. All related links between nodes are deleted. The Objects [i=85] node from the
 *                                      standard OPC UA space can be specified as a start node. All other nodes of the standard are prohibited.
 *                                      If the specified start node is missing on the server and the "flat_list_of_nodes__create_missing_start_node" parameter is disabled,
 *                                      an export error will be returned.
 *                                      [optionally] [experimental]
 * @param flat_list_of_nodes__create_missing_start_node Works in conjunction only with the activated parameter "flat_list_of_nodes__is_enable". In case of activation, if the starting node does
 *                                                      not exist (an error will occur when trying to collect data from it), then such a node will be created as the "Object" class and bound to
 *                                                      the parent node specified in the "parent_start_node_replacer" parameter.
 *                                                      If all other node lists have one non-existent node, then such lists will all be bound to one created node. [optionally] [experimental]
 * @param flat_list_of_nodes__allow_abstract_variable Works in conjunction with "flat_list_of_nodes__create_missing_start_node" and "flat_list_of_nodes__is_enable".
 *                                                    When enabled, adding two backlinks of type "HasComponent" to nodes 'i=63" and "i=58" thus allows using nodes of class
 *                                                    "Variable" of abstract type. [optionally] [experimental]
 */
struct Options
{
    std::optional<std::reference_wrapper<LoggerBase>> logger = std::nullopt;
    u_int32_t number_of_max_nodes_to_request_data = 0;
    EncoderTypes encoder_types = EncoderTypes::XML;
    LogLevel internal_log_level = LogLevel::Info;
    ExpandedNodeId parent_start_node_replacer = ExpandedNodeId(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_TYPES_EXPANDEDNODEID);
    bool is_perf_timer_enable = false;
    bool ns0_custom_nodes_ready_to_work = false;
    struct
    {
        bool is_enable;
        bool create_missing_start_node;
        bool allow_abstract_variable;
    } flat_list_of_nodes{};
};

/**
 * @brief The function of export of the list of nodes with the necessary environment to the file or buffer with the indicated type of encoding.
 * @param open62541_objects Created object of the UA_Server or UA_Client type, which will be used as a data source.
 * @param node_ids Multiple node list (map), where <text name of starting node ID, list of node IDs including starting>.
 *                 For example: {"ns=2;i=5" : [ns=2;i=5, ns=2;i=6, ..., ns=2; i=999], "ns=3;s=start_node_1 : [ns=3;i=222, ns=3;s=strew, ....]}.
 * @param filiname Full path and file name where the unloading will be formed.
 * @param out_buffer Output buffer, where unloading will be formed instead of a file. When specifying this parameter, the file will not be formed.
 * @param opt. Additional export mode parameters.
 * @return Status of the function.
 */
template <typename TOpen62541ServerOrClient>
StatusResults DLL_PUBLIC ExportNodeset(
    TOpen62541ServerOrClient& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer,
    const Options& opt) noexcept;

/**
 * @brief Function for exporting a list of nodes with the required environment to a file or buffer with the specified encoding type where the data source is OPC UA Server.
 * @warning !!! ExportNodesetFromServer is not implemented !!!
 * @param open62541_objects Object of the created UA_Server.
 * @param node_ids List of nodes to export.
 * @param filename Full path and name of the file where the upload will be generated.
 * @param out_buffer Output buffer where the upload will be generated instead of the file. When this parameter is specified, the file will not be generated. [optional]
 * @param opt Additional export mode options. [optional]
 * @return Function execution status.
 */
static StatusResults DLL_PUBLIC ExportNodesetFromServer(
    UA_Server& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer = std::nullopt,
    const Options& opt = Options()) noexcept
{
    return ExportNodeset<UA_Server>(open62541_object, node_ids, std::move(filename), out_buffer, opt);
}


/**
 * @brief Function for exporting a list of nodes with the required environment to a file or buffer with the specified encoding type where the data source is OPC UA Client.
 * @param open62541_objects The object of the created Client.
 * @param node_ids List of nodes to export.
 * @param filename Full path and name of the file where the upload will be generated.
 * @param out_buffer Output buffer where the upload will be generated instead of the file. When this parameter is specified, the file will not be generated. [optional]
 * @param opt Additional export mode options. [optional]
 * @return Function execution status.
 */
static StatusResults DLL_PUBLIC ExportNodesetFromClient( // NOLINT(misc-definitions-in-headers)
    UA_Client& open62541_object,
    const std::map<std::string, std::vector<ExpandedNodeId>>& node_ids,
    std::string&& filename,
    std::optional<std::reference_wrapper<std::iostream>> out_buffer = std::nullopt,
    const Options& opt = Options()) noexcept
{
    return ExportNodeset<UA_Client>(open62541_object, node_ids, std::move(filename), out_buffer, opt);
}

} // namespace nodesetexporter

#endif // NODESETEXPORTER_NODESETEXPORTER_H
