//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/BrowseOperations.h"
#include "nodesetexporter/common/Strings.h"

namespace nodesetexporter::open62541::browseoperations
{

inline UA_StatusCode NodeIteratorCallback(UA_NodeId child_id, UA_Boolean /*is_inverse*/, UA_NodeId /*reference_type_id*/, void* /*unused*/)
{
    one_iteration_nodes.emplace_back(UA_EXPANDEDNODEID_NODEID(child_id), UA_TYPES_EXPANDEDNODEID);

    return UA_STATUSCODE_GOOD;
}

// NOLINTBEGIN
UA_StatusCode UA_Client_forEachChildNodeCall_Ex(UA_Client* client, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback, void* handle, const UA_BrowseOptions* const options)
{
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    UA_NodeId_copy(&parentNodeId, &bReq.nodesToBrowse[0].nodeId); // Deep copy
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; // return everything
    if (options != NULL)
    {
        bReq.nodesToBrowse[0].browseDirection = options->direction;
        bReq.nodesToBrowse[0].includeSubtypes = options->includeSubtypes;
        if (options->refType != NULL)
        {
            UA_NodeId_copy(options->refType, &bReq.nodesToBrowse[0].referenceTypeId); // Deep copy
        }
    }
    else
    {
        bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;
    }

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

    UA_StatusCode retval = bResp.responseHeader.serviceResult;
    if (retval == UA_STATUSCODE_GOOD)
    {
        for (size_t i = 0; i < bResp.resultsSize; ++i)
        {
            for (size_t j = 0; j < bResp.results[i].referencesSize; ++j)
            {
                UA_ReferenceDescription* ref = &bResp.results[i].references[j];
                retval |= callback(ref->nodeId.nodeId, !ref->isForward, ref->referenceTypeId, handle);
            }
        }
    }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    return retval;
}
// NOLINTEND

StatusResults GrabChildNodeIdsFromStartNodeId(UA_Client* client, const UATypesContainer<UA_ExpandedNodeId>& start_node_id, std::vector<UATypesContainer<UA_ExpandedNodeId>>& out)
{
    out.push_back(start_node_id); // Start node in test
    int counter = 0;
    auto ref_type = UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), UA_TYPES_NODEID);
    UA_BrowseOptions browse_opt;
    browse_opt.includeSubtypes = true;
    browse_opt.direction = UA_BROWSEDIRECTION_FORWARD;
    browse_opt.refType = &ref_type.GetRef();
    // Perform a more primitive analogue of the Browsing operation of the entire structure of nodes starting from the starting one
    do
    {
        one_iteration_nodes.clear();
        for (size_t index = counter; index < out.size(); index++)
        {
            // Using the ready-made browsing function. The disadvantage is that only one node accepts input, but for small volumes you can get by with this.
            if (UA_StatusCode_isBad(UA_Client_forEachChildNodeCall_Ex(client, out[index].GetRef().nodeId, &NodeIteratorCallback, nullptr, &browse_opt)))
            {
                return StatusResults::Fail;
            }
            counter++;
        }
        if (!one_iteration_nodes.empty())
        {
            std::ranges::copy(one_iteration_nodes, back_inserter(out));
        }
    } while (!one_iteration_nodes.empty());

    return StatusResults::Good;
}

} // namespace nodesetexporter::open62541::browseoperations