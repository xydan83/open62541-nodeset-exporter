//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/ClientWrappers.h"

#include "magic_enum/magic_enum.hpp"
#include <algorithm>

namespace nodesetexporter::open62541
{
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
StatusResults Open62541ClientWrapper::Browse(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists, size_t browse_offset, const UA_BrowseRequest& b_req, size_t& total_read_ref)
{
    UATypesContainer<UA_BrowseResponse> response(UA_TYPES_BROWSERESPONSE);
    response.ShallowCopyingAndOwnership(UA_Client_Service_browse(&m_ua_client, b_req)); //<-- BROWSE
    if (UA_StatusCode_isBad(response.GetRef().responseHeader.serviceResult))
    {
        m_logger.Error("Browse has error from Open62541: {}", UA_StatusCode_name(response.GetRef().responseHeader.serviceResult));
        return StatusResults::Fail;
    }
    if (UA_StatusCode_isUncertain(response.GetRef().responseHeader.serviceResult))
    {
        m_logger.Warning("Browse has uncertain value from Open62541: {}", UA_StatusCode_name(response.GetRef().responseHeader.serviceResult));
    }

    for (size_t node_index = 0; node_index < response.GetRef().resultsSize; ++node_index)
    {
        m_logger.Debug(
            "Total points: {}, Point number: {}, NodeID: {}, References received: {}, Presence of continuationPoint: {}",
            response.GetRef().resultsSize,
            node_index,
            node_references_structure_lists.at(node_index + browse_offset).exp_node_id.get().ToString(),
            response.GetRef().results[node_index].referencesSize,
            response.GetRef().results[node_index].continuationPoint.length != 0);
        if (UA_StatusCode_isBad(response.GetRef().results[node_index].statusCode))
        {
            m_logger.Warning(
                "UA_BrowseResult has bad status '{}' of node {} in response.",
                UA_StatusCode_name(response.GetRef().results[node_index].statusCode),
                node_references_structure_lists.at(node_index + browse_offset).exp_node_id.get().ToString());
        }
        if (UA_StatusCode_isUncertain(response.GetRef().results[node_index].statusCode))
        {
            m_logger.Warning(
                "UA_BrowseResult has uncertain status '{}' of node {} in response.",
                UA_StatusCode_name(response.GetRef().results[node_index].statusCode),
                node_references_structure_lists.at(node_index + browse_offset).exp_node_id.get().ToString());
        }

        // Point
        for (size_t ref_index = 0; ref_index < response.GetRef().results[node_index].referencesSize; ++ref_index)
        {
            // Node
            // Processing the browsing result
            node_references_structure_lists.at(node_index + browse_offset).references.emplace_back(response.GetRef().results[node_index].references[ref_index], UA_TYPES_REFERENCEDESCRIPTION);
        }

        // Call BrowseNext. The condition prevents an unnecessary function call when everything has been read
        if (response.GetRef().results[node_index].continuationPoint.length != 0
            && BrowseNext(&response.GetRef().results[node_index].continuationPoint, node_references_structure_lists.at(node_index + browse_offset).references) == StatusResults::Fail)
        {
            m_logger.Error("BrowseNext error with NodeID: {}", node_references_structure_lists.at(node_index + browse_offset).exp_node_id.get().ToString());
            return StatusResults::Fail;
        }
        total_read_ref += node_references_structure_lists.at(node_index + browse_offset).references.size();
    }
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::BrowseNext(UA_ByteString* const continuation_point, std::vector<UATypesContainer<UA_ReferenceDescription>>& result_nodes)
{
    m_logger.Trace("Method called: BrowseNext()");
    if (continuation_point == nullptr)
    {
        throw std::runtime_error("continuation_point is null");
    }
    UATypesContainer<UA_BrowseNextRequest> b_next_req(UA_TYPES_BROWSENEXTREQUEST);
    UA_BrowseNextRequest_init(&b_next_req.GetRef());

    UATypesContainer<UA_ByteString> i_continuation_point(UA_TYPES_BYTESTRING);
    UA_ByteString_copy(continuation_point, &i_continuation_point.GetRef());
    while (i_continuation_point.GetRef().length != 0)
    {
        b_next_req.GetRef().releaseContinuationPoints = UA_FALSE;
        b_next_req.GetRef().continuationPoints = &i_continuation_point.GetRef();
        b_next_req.GetRef().continuationPointsSize = 1;

        // Create a structure to ensure that UA_BrowseNextResponse is removed when exiting the processing function.
        struct UaBrowseNextResponseWithAutoClear
        {
            UaBrowseNextResponseWithAutoClear() = delete;
            UaBrowseNextResponseWithAutoClear(UA_Client& client, const UA_BrowseNextRequest& request)
                : value(UA_Client_Service_browseNext(&client, request))
            {
            }
            ~UaBrowseNextResponseWithAutoClear()
            {
                UA_BrowseNextResponse_clear(&value);
            }
            UaBrowseNextResponseWithAutoClear(UaBrowseNextResponseWithAutoClear const&) noexcept = delete;
            UaBrowseNextResponseWithAutoClear& operator=(UaBrowseNextResponseWithAutoClear const&) = delete;
            UaBrowseNextResponseWithAutoClear(UaBrowseNextResponseWithAutoClear&&) noexcept = delete;
            UaBrowseNextResponseWithAutoClear& operator=(UaBrowseNextResponseWithAutoClear&&) = delete;


            UA_BrowseNextResponse value;
        };

        const UaBrowseNextResponseWithAutoClear response(m_ua_client, b_next_req.GetRef()); //<-- BROWSE NEXT
        UA_BrowseNextRequest_init(&b_next_req.GetRef()); // cleaning the structure before filling it again

        if (UA_StatusCode_isBad(response.value.responseHeader.serviceResult))
        {
            m_logger.Error("Browse Next has bad status '{}' in response.", UA_StatusCode_name(response.value.responseHeader.serviceResult));
            return StatusResults::Fail;
        }
        if (UA_StatusCode_isUncertain(response.value.responseHeader.serviceResult))
        {
            m_logger.Warning("Browse Next has uncertain status '{}' in response.", UA_StatusCode_name(response.value.responseHeader.serviceResult));
        }

        if (response.value.results == nullptr)
        {
            throw std::runtime_error("response.value.results == nullptr");
        }
        if (response.value.resultsSize != 1)
        {
            throw std::runtime_error("response.value.resultsSize != 1");
        }
        m_logger.Debug("{} references received", response.value.results[0].referencesSize);
        for (size_t j = 0; j < response.value.results[0].referencesSize; ++j)
        {
            if (UA_StatusCode_isBad(response.value.results[0].statusCode))
            {
                m_logger.Warning("UA_BrowseResult has bad status '{}' in response.", UA_StatusCode_name(response.value.results[0].statusCode));
            }
            if (UA_StatusCode_isUncertain(response.value.results[0].statusCode))
            {
                m_logger.Warning("UA_BrowseResult has uncertain status '{}' in response.", UA_StatusCode_name(response.value.results[0].statusCode));
            }
            // Processing the browsing result in parts
            result_nodes.emplace_back(response.value.results[0].references[j], UA_TYPES_REFERENCEDESCRIPTION);
        }
        UA_ByteString_clear(&i_continuation_point.GetRef());
        UA_ByteString_copy(&response.value.results[0].continuationPoint, &i_continuation_point.GetRef());
    }
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::ReadNodesAttributes(std::vector<UA_ReadValueId>& read_value_ids, const std::function<void(size_t, UA_DataValue&, UA_NodeId&, UA_AttributeId)>& set_data)
{
    m_logger.Trace("Method called: ReadNodesAttributes()");
    UA_ReadRequest read_request = {
        0}; // Create on the stack without a class wrapper, otherwise the delimiter in the wrapper will delete everything, including what is passed to read_request.nodesToRead
    // by pointer and when std::vector<UA_ReadValueId> is already deleted and the contents of read_value_ids are deleted, there will be an attempt to double delete.

    size_t step_size = read_value_ids.size(); // Processing the full amount of data
    if (m_max_nodes_per_read != 0)
    {
        // If a limit is set, we will check whether it exceeds the volume that we want to count, and if not, then we will take the size according to the limit.
        step_size = m_max_nodes_per_read > read_value_ids.size() ? read_value_ids.size() : m_max_nodes_per_read;
    }

    size_t begin_offset = 0;
    while (begin_offset < read_value_ids.size())
    {
        if (m_logger.GetLevel() <= LogLevel::Debug)
        {
            std::string nodes_txt_buf;
            std::for_each_n(
                read_value_ids.begin() + static_cast<int64_t>(begin_offset),
                step_size,
                [&nodes_txt_buf](const UA_ReadValueId& read_val)
                {
                    fmt::format_to(std::back_inserter(nodes_txt_buf), "{}, ", UATypesContainer<UA_NodeId>(read_val.nodeId, UA_TYPES_NODEID).ToString());
                });
            m_logger.Info("Preparing to read attribute nodes ({}) (NodeID == Attribute). Read attribute: {}, Attribute to read size: {}", nodes_txt_buf, begin_offset, step_size);
        }
        else
        {
            m_logger.Info("Preparing to read attribute nodes (NodeID == Attribute). Read attribute: {}, Attribute to read size: {}", begin_offset, step_size);
        }

        read_request.nodesToRead = &read_value_ids.at(begin_offset);
        read_request.nodesToReadSize = step_size;


        // I will create a structure to ensure that UA_ReadResponse is deleted when exiting the processing function.
        struct ReadResponseWithAutoClear // NOLINT(cppcoreguidelines-special-member-functions)
        {
            ReadResponseWithAutoClear() = delete;
            ReadResponseWithAutoClear(UA_Client& client, const UA_ReadRequest& read_request)
                : value(UA_Client_Service_read(&client, read_request))
            {
            }
            ~ReadResponseWithAutoClear()
            {
                UA_ReadResponse_clear(&value);
            }
            ReadResponseWithAutoClear(ReadResponseWithAutoClear const&) noexcept = delete;
            ReadResponseWithAutoClear& operator=(ReadResponseWithAutoClear const&) = delete;
            ReadResponseWithAutoClear(ReadResponseWithAutoClear&&) noexcept = delete;
            ReadResponseWithAutoClear& operator=(ReadResponseWithAutoClear&&) = delete;

            UA_ReadResponse value;
        };

        // To automatically fire the structure destructor whenever the function exits, I create a structure on the stack.
        ReadResponseWithAutoClear response_wrap(m_ua_client, read_request); // <-- request data via open62541
        if (UA_StatusCode_isBad(response_wrap.value.responseHeader.serviceResult))
        {
            m_logger.Error("ReadNodesAttributes has error from Open62541: {}", UA_StatusCode_name(response_wrap.value.responseHeader.serviceResult));
            // Will UA_ReadRequest also delete vector objects by pointer?
            return StatusResults::Fail;
        }
        if (UA_StatusCode_isUncertain(response_wrap.value.responseHeader.serviceResult))
        {
            m_logger.Warning("ReadNodesAttributes has uncertain value from Open62541: {}", UA_StatusCode_name(response_wrap.value.responseHeader.serviceResult));
        }

        if (response_wrap.value.resultsSize != step_size)
        {
            m_logger.Error("ReadNodesAttributes has error: response results size not equal to requested. {} != {}", response_wrap.value.resultsSize, step_size);
            return StatusResults::Fail;
        }

        // Cycle for issuing requested data by attributes
        for (size_t index = 0; index < response_wrap.value.resultsSize; index++)
        {
            if (read_value_ids.at(index).attributeId == UA_ATTRIBUTEID_NODECLASS)
            {
                // Correction. When querying the NodeClass attribute, the type returned is Int32.
                // This behavior is described here: https://github.com/open62541/open62541/commit/6ae9c485e126c62254f14f641900706ede072d45
                // The library level fix was made only in the __UA_Client_readAttribute function.
                // Redefining the type.
                response_wrap.value.results[index].value.type = &UA_TYPES[UA_TYPES_NODECLASS];
            }
            set_data(index + begin_offset, response_wrap.value.results[index], read_value_ids.at(index).nodeId, static_cast<UA_AttributeId>(read_value_ids.at(index).attributeId));
        }

        begin_offset += step_size; // I shift the index of the start of the next stage of data reading by the size of the completed step.
        step_size = read_value_ids.size() - begin_offset > step_size ? step_size : read_value_ids.size() - begin_offset; // Calculation of a new step taking into account the data remaining to be read.
    }
    return StatusResults::Good;
}

size_t Open62541ClientWrapper::CalculateBrowseLimit(
    const size_t numbers_of_nodes,
    const uint32_t max_nodes_per_browse,
    const uint16_t max_browse_continuation_points,
    const uint32_t requested_max_references_per_node) noexcept
{
    // Calculating which of the two constraint parameters is the most minimal
    size_t pre_param_limit = max_nodes_per_browse;

    if (max_nodes_per_browse != 0 && max_browse_continuation_points != 0 && requested_max_references_per_node != 0)
    {
        pre_param_limit = max_nodes_per_browse <= max_browse_continuation_points ? max_nodes_per_browse : max_browse_continuation_points;
    }
    else if (max_nodes_per_browse == 0 && requested_max_references_per_node != 0)
    {
        pre_param_limit = max_browse_continuation_points;
    }
    // I compare it with the number of nodes for browsing and calculate the limit, or the limit == the total number of nodes.
    return (pre_param_limit != 0 && pre_param_limit < numbers_of_nodes) ? pre_param_limit : numbers_of_nodes;
}

StatusResults Open62541ClientWrapper::ReadNodeClasses(std::vector<NodeClassesRequestResponse>& node_class_structure_lists)
{
    m_logger.Trace("Method called: ReadNodeClasses()");
    std::unique_ptr<std::vector<UA_ReadValueId>, void (*)(std::vector<UA_ReadValueId>* const)> read_value_ids(
        new std::vector<UA_ReadValueId>(node_class_structure_lists.size()),
        [](std::vector<UA_ReadValueId>* const vec)
        {
            // I clear all the contents of UA_ReadValueId structures by pointers
            for (auto& read_value_id : *vec)
            {
                UA_ReadValueId_clear(&read_value_id);
            }
            // I delete Vector along with the contents of UA_ReadValueId
            delete vec; // NOLINT(cppcoreguidelines-owning-memory)
        });
    for (size_t index = 0; index < node_class_structure_lists.size(); index++)
    {
        UA_NodeId_copy(&node_class_structure_lists.at(index).exp_node_id.get().GetRef().nodeId, &read_value_ids->at(index).nodeId);
        read_value_ids->at(index).attributeId = UA_ATTRIBUTEID_NODECLASS;
    }

    size_t good_attr_read = 0;
    auto status = ReadNodesAttributes(
        *read_value_ids,
        [&node_class_structure_lists, &logger = m_logger, &good_attr_read](size_t array_index, const UA_DataValue& data_value, const UA_NodeId& /*not_need*/, UA_AttributeId attr_id)
        {
            if (!UA_StatusCode_isBad(data_value.status) && data_value.hasValue)
            {
                // The basic rule of a request-response using the OPC UA protocol is that you can be sure that the response will arrive in the same order in which the request was made.
                // Based on this and knowing the sequence of composing the request, you can also sequentially link the response to the request, for example, the requested data in a certain order
                // by NodeID will be returned as a response in the same order and they can be linked in the same order to the constructed NodeId sequence in the request.
                // https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2.2
                // memcpy(&node_class_structure_lists[array_index].node_class, static_cast<UA_NodeClass*>(data_value.value.data), sizeof(UA_NodeClass));
                node_class_structure_lists.at(array_index).node_class = *static_cast<UA_NodeClass*>(data_value.value.data);
                good_attr_read++;
            }
            else
            {
                node_class_structure_lists.at(array_index).node_class = UA_NodeClass::UA_NODECLASS_UNSPECIFIED;
                logger.Warning(
                    "ReadNodeClasses (atrId={}) has bad status '{}' of node {} in response",
                    magic_enum::enum_name(attr_id),
                    UA_StatusCode_name(data_value.status),
                    node_class_structure_lists.at(array_index).exp_node_id.get().ToString());
                node_class_structure_lists.at(array_index).result_code = data_value.status;
            }
        });
    m_logger.Info("Total read node classes good status attributes: {} from Nodes: {}", good_attr_read, node_class_structure_lists.size());
    return status;
}

StatusResults Open62541ClientWrapper::ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists)
{
    m_logger.Trace("Method called: ReadNodeReferences()");

    // Calculation of the limit on the number of nodes per request to the server.
    auto limit = CalculateBrowseLimit(node_references_structure_lists.size(), m_max_nodes_per_browse, m_max_browse_continuation_points, m_requested_max_references_per_node);
    m_logger.Info("Browse limit: {}. Nodes size list: {}", limit, node_references_structure_lists.size());
    size_t all_of_nodes_counter = 0;
    size_t browse_offset = 0;
    size_t total_read_ref = 0;
    UA_BrowseRequest b_req; // The structure on the stack will be deleted upon exit, except for the structures at the pointer "UA_BrowseDescription *nodesToBrowse".

    // The cycle of sending requests for all NodeIDs. Within the cycle, depending on the limit, requests will be sent in portions.
    while (all_of_nodes_counter < node_references_structure_lists.size())
    {
        UA_BrowseRequest_init(&b_req);
        // I create structures nodesToBrowse. I am calculating the step with which it is necessary to measure the size of the structure for sending.
        auto step_size = std::min(limit, node_references_structure_lists.size() - all_of_nodes_counter);
        std::unique_ptr<std::vector<UA_BrowseDescription>, void (*)(std::vector<UA_BrowseDescription>* const)> b_req_vector(
            new std::vector<UA_BrowseDescription>(step_size),
            [](std::vector<UA_BrowseDescription>* const vec)
            {
                // Happy content of the structure of UA_BrowseDecription on the index
                for (auto& read_value_id : *vec)
                {
                    UA_BrowseDescription_clear(&read_value_id);
                }
                // Я удаляю Vector вместе с содержимым UA_BrowseDescription
                delete vec; // NOLINT(cppcoreguidelines-owning-memory)
            });
        // I will fill the contents of the created objects of the UA_BrowseDescription structures
        m_logger.Info("Preparing to read reference from nodes. Number of NodesID to read reference: {}", step_size);
        for (auto& br_descr : *b_req_vector)
        {
            if (m_logger.GetLevel() <= LogLevel::Debug) // To avoid running ToString() once again
            {
                m_logger.Debug("Name of sent nodes:\nNodeID: '{}'", node_references_structure_lists.at(all_of_nodes_counter).exp_node_id.get().ToString());
            }
            br_descr.includeSubtypes = UA_TRUE;
            br_descr.browseDirection = UA_BROWSEDIRECTION_BOTH;
            br_descr.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
            UA_NodeId_copy(&node_references_structure_lists.at(all_of_nodes_counter).exp_node_id.get().GetRef().nodeId, &br_descr.nodeId);
            br_descr.resultMask = UA_BROWSERESULTMASK_ALL;
            all_of_nodes_counter++;
        }

        m_logger.Debug("--------------------------------------");
        // I assign a pointer to the first element of the array
        b_req.nodesToBrowse = b_req_vector->data();
        b_req.nodesToBrowseSize = b_req_vector->size();
        b_req.requestedMaxReferencesPerNode = m_requested_max_references_per_node;

        // Sending a request to the Server and receiving the result.
        if (Browse(node_references_structure_lists, browse_offset, b_req, total_read_ref) == StatusResults::Fail) // Все результаты пишутся в конец node_references_structure_lists.
        {
            return StatusResults::Fail;
        }
        // Shift for the internal request of the Browse function, during which the node_references_structure_lists structure is filled.
        //  So, in order not to make intermediate elements and to avoid copying, we go directly to the node_references_structure_lists structure
        //  by index, to fill in the necessary elements received from the server.
        browse_offset += step_size;
    }
    m_logger.Info("Total read node reference: {} from Nodes: {}", total_read_ref, node_references_structure_lists.size());
    return StatusResults::Good;
}


StatusResults Open62541ClientWrapper::ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& node_attr_structure_lists, size_t attr_sum)
{
    m_logger.Trace("Method called: ReadNodesAtrrubutes()");
    std::unique_ptr<std::vector<UA_ReadValueId>, void (*)(std::vector<UA_ReadValueId>* const)> read_value_ids(
        new std::vector<UA_ReadValueId>,
        [](std::vector<UA_ReadValueId>* const vec)
        {
            // I clear all the contents of UA_ReadValueId structures by pointers
            for (auto& read_value_id : *vec)
            {
                UA_ReadValueId_clear(&read_value_id);
            }
            // I delete Vector along with the contents of UA_ReadValueId
            delete vec; // NOLINT(cppcoreguidelines-owning-memory)
        });
    read_value_ids->reserve(attr_sum);
    size_t attr_index = 0;
    for (const auto& node_attr_structure_list : node_attr_structure_lists)
    {
        for (const auto& attr : node_attr_structure_list.attrs)
        {
            read_value_ids->emplace_back();
            UA_NodeId_copy(&node_attr_structure_list.exp_node_id.get().GetRef().nodeId, &read_value_ids->at(attr_index).nodeId);
            read_value_ids->at(attr_index).attributeId = attr.first;
            attr_index++;
        }
    }
    size_t const& flat_attr_numbers = attr_index;
    if (read_value_ids->size() != flat_attr_numbers)
    {
        throw std::runtime_error("read_value_ids.size() != flat_attr_numbers");
    }

    std::vector<std::optional<VariantsOfAttr>> variants(attr_index);
    size_t good_attr_read = 0;
    if (auto result = ReadNodesAttributes(
            *read_value_ids,
            [&variants, &logger = m_logger, &good_attr_read](size_t array_index, const UA_DataValue& data_value, const UA_NodeId& node_id, UA_AttributeId attr_id) // attr_index == array_index
            {
                if (!UA_StatusCode_isBad(data_value.status) && data_value.hasValue && !UA_Variant_isEmpty(&data_value.value))
                {
                    variants.at(array_index) = UAVariantToStdVariant(data_value.value);
                    // We ignore the ArrayDimensions type, since it can have the value null, and this will not be considered any kind of error,
                    //  therefore, the typeName field will be present in UA_Variant despite value == null.
                    //  https://reference.opcfoundation.org/Core/Part5/v104/docs/3.3
                    if (variants.at(array_index) == std::nullopt && attr_id != UA_ATTRIBUTEID_ARRAYDIMENSIONS)
                    {
                        logger.Warning(
                            "ReadNodesAtrrubutes. NodeID:{}. Data type '{}' of attr_id '{}' is not supported.",
                            UATypesContainer<UA_NodeId>(node_id, UA_TYPES_NODEID).ToString(),
                            data_value.value.type->typeName,
                            magic_enum::enum_name(attr_id));
                    }
                    good_attr_read++;
                }
                else
                {
                    variants.at(array_index) = std::nullopt;
                    if (UA_Variant_isEmpty(&data_value.value))
                    {
                        logger.Warning(
                            "ReadNodesAtrrubutes ({}) has status '{}' of node '{}' in response. Data value is empty.",
                            magic_enum::enum_name(attr_id),
                            UA_StatusCode_name(data_value.status),
                            UATypesContainer<UA_NodeId>(node_id, UA_TYPES_NODEID).ToString());
                    }
                    else
                    {
                        logger.Warning(
                            "ReadNodesAtrrubutes ({}) has status '{}' of node '{}' in response.",
                            magic_enum::enum_name(attr_id),
                            UA_StatusCode_name(data_value.status),
                            UATypesContainer<UA_NodeId>(node_id, UA_TYPES_NODEID).ToString());
                    }
                }
            });
        result != StatusResults::Good)
    {
        return result;
    }

    // The basic rule of a request-response using the OPC UA protocol is that you can be sure that the response will arrive in the same order in which the request was made.
    // Based on this and knowing the sequence of composing the request, you can also sequentially link the response to the request, for example, the requested data in a certain order by NodeID
    // will be returned as a response in the same order, and they can be linked in the same order to the constructed NodeId sequence in the request.
    // https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2.2
    attr_index = 0;
    for (auto& node_attr_structure_list : node_attr_structure_lists)
    {
        for (auto& attr : node_attr_structure_list.attrs)
        {
            attr.second = variants.at(attr_index);
            attr_index++;
        }
    }
    m_logger.Info("Total read good status attributes: {} from Nodes: {}", good_attr_read, node_attr_structure_lists.size());
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::ReadNodeDataValue(const UATypesContainer<UA_ExpandedNodeId>& node_id, UATypesContainer<UA_Variant>& data_value)
{
    m_logger.Trace("Method called: ReadNodeDataValue()");
    auto status = UA_Client_readValueAttribute(&m_ua_client, node_id.GetRef().nodeId, &data_value.GetRef());
    if (UA_StatusCode_isBad(status))
    {
        m_logger.Error("ReadNodeDataValue has error from Open62541: {}", UA_StatusCode_name(status));
        return StatusResults::Fail;
    }
    if (UA_StatusCode_isUncertain(status))
    {
        m_logger.Warning("ReadNodeDataValue has uncertain value from Open62541: {}", UA_StatusCode_name(status));
    }
    return StatusResults::Good;
}
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
} // namespace nodesetexporter::open62541