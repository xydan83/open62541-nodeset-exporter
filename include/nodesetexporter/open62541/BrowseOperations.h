//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//
#ifndef NODESETEXPORTER_OPEN62541_BROWSEOPERATIONS_H
#define NODESETEXPORTER_OPEN62541_BROWSEOPERATIONS_H

#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/client_highlevel.h>

#include <set>
#include <vector>

// todo Add the ability to choose which node classes to ignore and not add to the list, as well as the ability to ignore nodes with ns=0.

/**
 * An auxiliary function for collecting the NodeId chain from the starting node in depth to prepare for export.
 * The function is based on another Open62541 library function UA_Client_forEachChildNodeCall.
 * This function is suitable for collecting a chain of a small number of nodes (up to 10 thousand), since UA_Client_forEachChildNodeCall allows you to receive parent node references
 * for only one such node in one call, which in a chain with a large number of nodes, in the case of a request over the network, will load the network and take a lot of time to receive data.
 *
 * If it is necessary to receive chains of a node hierarchy of more than 10 thousand from the starting node, it is necessary to use lower-level functions of the Open62541 library
 * (__UA_Client_Service or __UA_Client_AsyncService), which allow you to receive references to multiple parent nodes at once in one function call (one request over the network).
 */
namespace nodesetexporter::open62541::browseoperations
{

using nodesetexporter::common::statuses::StatusResults;

static std::vector<UATypesContainer<UA_ExpandedNodeId>> one_iteration_nodes; // Temporary Nodes for a Single Processing Iteration //NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * @brief Call-back function for filtering and adding nodes to the array for processing after receiving nodes from the server.
 */
UA_StatusCode NodeIteratorCallback(UA_NodeId child_id, UA_Boolean is_inverse, UA_NodeId reference_type_id, void* /*handle*/);

// todo Remove the UA_BrowseOptions structure and the UA_Client_forEachChildNodeCall_Ex function after accepting the MR https://github.com/open62541/open62541/pull/5846
// NOLINTBEGIN
struct UA_BrowseOptions
{
    UA_BrowseDirection direction = UA_BROWSEDIRECTION_BOTH;
    bool includeSubtypes = false;
    UA_NodeId* refType = NULL;
};

/**
 * @brief Get a list of references for the specified node.
 */
UA_StatusCode UA_Client_forEachChildNodeCall_Ex(UA_Client* client, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback, void* handle, const UA_BrowseOptions* options = NULL);
// NOLINTEND


/**
 * @brief Function of iterative passage through all nodes starting from the starting one and collecting a list of nodes for export.
 * @warning The function does not ignore nodes with namespace=0 included in hierarchical reference.
 * @param client - Pointer to the Open62541 client object.
 * @param start_node_id - Link to the starting node from which the list of nodes for export will be built.
 * @param out - Link to the list where the list of nodes for export will be built.
 * @return Request execution status.
 */
[[maybe_unused]] StatusResults GrabChildNodeIdsFromStartNodeId(UA_Client* client, const UATypesContainer<UA_ExpandedNodeId>& start_node_id, std::vector<UATypesContainer<UA_ExpandedNodeId>>& out);

} // namespace nodesetexporter::open62541::browseoperations

#endif // NODESETEXPORTER_OPEN62541_BROWSEOPERATIONS_H
