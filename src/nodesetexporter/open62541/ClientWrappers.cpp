//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/ClientWrappers.h"

namespace nodesetexporter::open62541
{

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
    while (i_continuation_point.GetRef().length != 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    {
        b_next_req.GetRef().releaseContinuationPoints = UA_FALSE;
        b_next_req.GetRef().continuationPoints = &i_continuation_point.GetRef();
        b_next_req.GetRef().continuationPointsSize = 1;

        // Create a structure to ensure that UA_BrowseNextResponse is removed when exiting the processing function.
        struct UaBrowseNextResponseWithAutoClear // NOLINT(cppcoreguidelines-special-member-functions)
        {
            ~UaBrowseNextResponseWithAutoClear()
            {
                UA_BrowseNextResponse_clear(&value);
            }
            UA_BrowseNextResponse value;
        };

        UaBrowseNextResponseWithAutoClear response{UA_Client_Service_browseNext(&m_ua_client, b_next_req.GetRef())}; //<-- BROWSE NEXT
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
        m_logger.Debug("{} references received", response.value.results[0].referencesSize); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (size_t j = 0; j < response.value.results[0].referencesSize; ++j) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            if (UA_StatusCode_isBad(response.value.results[0].statusCode)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            {
                m_logger.Warning(
                    "UA_BrowseResult has bad status '{}' in response.",
                    UA_StatusCode_name(response.value.results[0].statusCode)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (UA_StatusCode_isUncertain(response.value.results[0].statusCode)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            {
                m_logger.Warning(
                    "UA_BrowseResult has uncertain status '{}' in response.",
                    UA_StatusCode_name(response.value.results[0].statusCode)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            // Processing the browsing result in parts
            result_nodes.emplace_back(response.value.results[0].references[j], UA_TYPES_REFERENCEDESCRIPTION); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        UA_ByteString_clear(&i_continuation_point.GetRef());
        UA_ByteString_copy(&response.value.results[0].continuationPoint, &i_continuation_point.GetRef()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::ReadNodesAttributes(std::vector<UA_ReadValueId>& read_value_ids, const std::function<void(size_t, UA_DataValue&, UA_NodeId&, UA_UInt32)>& set_data)
{
    m_logger.Trace("Method called: ReadNodesAttributes()");
    UA_ReadRequest read_request = {0}; // Create on the stack without a class wrapper, otherwise the deliter in the wrapper will delete everything,
    // including what is passed to read_request.nodesToRead by pointer and when std::vector<UA_ReadValueId> is deleted and deletes the contents of read_value_ids,
    // there will be an attempt at double deletion.
    read_request.nodesToRead = read_value_ids.data();
    read_request.nodesToReadSize = read_value_ids.size();

    // Create a structure to ensure that UA_ReadResponse is deleted when exiting the processing function.
    struct ReadResponseWithAutoClear // NOLINT(cppcoreguidelines-special-member-functions)
    {
        ~ReadResponseWithAutoClear()
        {
            UA_ReadResponse_clear(&value);
        }
        UA_ReadResponse value;
    };

    // To automatically fire the structure destructor whenever the function exits, I create a structure on the stack.
    ReadResponseWithAutoClear response_wrap{UA_Client_Service_read(&m_ua_client, read_request)}; // <-- REQUEST DATA VIA Open62541
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

    if (response_wrap.value.resultsSize != read_value_ids.size())
    {
        m_logger.Error("ReadNodesAttributes has error: response results size not equal to requested. {} != {}", response_wrap.value.resultsSize, read_value_ids.size());
        return StatusResults::Fail;
    }

    // Cycle of issuing requested data by attributes
    for (size_t index = 0; index < response_wrap.value.resultsSize; index++)
    {
        if (read_value_ids.at(index).attributeId == UA_ATTRIBUTEID_NODECLASS)
        {
            // Correction. When querying the NodeClass attribute, the type returned is Int32.
            // This behavior is described here: https://github.com/open62541/open62541/commit/6ae9c485e126c62254f14f641900706ede072d45
            // The library level fix was made only in the __UA_Client_readAttribute function.
            // Redefining the type.
            response_wrap.value.results[index].value.type = &UA_TYPES[UA_TYPES_NODECLASS]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        set_data(index, response_wrap.value.results[index], read_value_ids.at(index).nodeId, read_value_ids.at(index).attributeId); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::ReadNodeClasses(std::vector<NodeClassesRequestResponse>& node_class_structure_lists)
{
    m_logger.Trace("Method called: ReadNodeClasses()");
    std::unique_ptr<std::vector<UA_ReadValueId>, void (*)(std::vector<UA_ReadValueId>* const)> read_value_ids(
        new std::vector<UA_ReadValueId>(node_class_structure_lists.size()),
        [](std::vector<UA_ReadValueId>* const vec)
        {
            // Remove all the contents of UA_READVALUEID structures according to signs
            for (auto& read_value_id : *vec)
            {
                UA_ReadValueId_clear(&read_value_id);
            }
            // Удаляю Vector вместе с содержимым UA_ReadValueId
            delete vec; // NOLINT(cppcoreguidelines-owning-memory)
        });
    for (size_t index = 0; index < node_class_structure_lists.size(); index++)
    {
        UA_NodeId_copy(&node_class_structure_lists.at(index).exp_node_id.GetRef().nodeId, &read_value_ids->at(index).nodeId);
        read_value_ids->at(index).attributeId = UA_ATTRIBUTEID_NODECLASS;
    }

    return ReadNodesAttributes(
        *read_value_ids,
        [&](size_t array_index, UA_DataValue& data_value, UA_NodeId& /*not_need*/, UA_UInt32 attr_id)
        {
            if (!UA_StatusCode_isBad(data_value.status) && data_value.hasValue) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            {
                // The basic rule of a request-response using the OPC UA protocol is that you can be sure that the response will arrive in the same order in which the request was made.
                // Based on this and knowing the sequence of composing the request, you can also sequentially link the response to the request, for example, the requested data in a certain order
                // by NodeID will be returned as a response in the same order and they can be linked in the same order to the constructed NodeId sequence in the request.
                // https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2.2
                // memcpy(&node_class_structure_lists[array_index].node_class, static_cast<UA_NodeClass*>(data_value.value.data), sizeof(UA_NodeClass));
                node_class_structure_lists.at(array_index).node_class = *static_cast<UA_NodeClass*>(data_value.value.data); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            else
            {
                node_class_structure_lists.at(array_index).node_class = UA_NodeClass::UA_NODECLASS_UNSPECIFIED;
                m_logger.Warning(
                    "ReadNodeClasses (atrId={}) has bad status '{}' of node {} in response",
                    attr_id,
                    UA_StatusCode_name(data_value.status), // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    node_class_structure_lists.at(array_index).exp_node_id.ToString());
                node_class_structure_lists.at(array_index).result_code = data_value.status;
            }
        });
}

// todo Add work with a limit of max_nodes_per_browse and max_browse_continuation_points (low priority).
//  The limit itself can be taken from the client; in this method, stage-by-stage reading relative to the limit can be organized.
StatusResults Open62541ClientWrapper::ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists)
{
    m_logger.Trace("Method called: ReadNodeReferences()");

    UA_BrowseRequest b_req; // The structure on the stack will be deleted upon exit, except for the structures at the pointer "UA_BrowseDescription *nodesToBrowse".
    UA_BrowseRequest_init(&b_req);
    // Creating nodesToBrowse structures
    std::unique_ptr<std::vector<UA_BrowseDescription>, void (*)(std::vector<UA_BrowseDescription>* const)> b_req_vector(
        new std::vector<UA_BrowseDescription>(node_references_structure_lists.size()),
        [](std::vector<UA_BrowseDescription>* const vec)
        {
            // Remove everything from the content of the structure of UA_BrowseDescription on the index
            for (auto& read_value_id : *vec)
            {
                UA_BrowseDescription_clear(&read_value_id);
            }
            // Delete Vector along with the content of UA_BrowseDescription
            delete vec; // NOLINT(cppcoreguidelines-owning-memory)
        });
    // заполню содержимое созданных объектов структур UA_BrowseDescription
    m_logger.Debug("--------------------------------------");
    m_logger.Debug("Prepare query parent NodeID[{}] --> references NodeIDs. Name of sent nodes:", node_references_structure_lists.size());

    int count = 0;
    for (const auto& node_ref_request_response_struct : node_references_structure_lists)
    {
        if (m_logger.GetLevel() <= LogLevel::Debug) // To avoid running ToString() once again
        {
            m_logger.Debug("NodeID: '{}'", node_ref_request_response_struct.exp_node_id.ToString());
        }
        b_req_vector->at(count).includeSubtypes = UA_TRUE;
        b_req_vector->at(count).browseDirection = UA_BROWSEDIRECTION_BOTH;
        b_req_vector->at(count).referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
        UA_NodeId_copy(&node_ref_request_response_struct.exp_node_id.GetRef().nodeId, &b_req_vector->at(count).nodeId);
        b_req_vector->at(count).resultMask = UA_BROWSERESULTMASK_ALL;
        count++;
    }
    m_logger.Debug("--------------------------------------");
    // Assign a pointer to the first element of the array
    b_req.nodesToBrowse = b_req_vector->data();
    b_req.nodesToBrowseSize = b_req_vector->size();
    b_req.requestedMaxReferencesPerNode = m_requested_max_references_per_node;

    // Create a structure to ensure that UA_BrowseResponse is removed when exiting the processing function.
    struct UaBrowseResponseWithAutoClear // NOLINT(cppcoreguidelines-special-member-functions)
    {
        explicit UaBrowseResponseWithAutoClear(UA_BrowseResponse&& br_response)
            : value(br_response){};
        ~UaBrowseResponseWithAutoClear()
        {
            UA_BrowseResponse_clear(&value);
        }
        UA_BrowseResponse& value;
    };

    UaBrowseResponseWithAutoClear response(UA_Client_Service_browse(&m_ua_client, b_req)); //<-- BROWSE
    if (UA_StatusCode_isBad(response.value.responseHeader.serviceResult))
    {
        m_logger.Error("Browse has error from Open62541: {}", UA_StatusCode_name(response.value.responseHeader.serviceResult));
        return StatusResults::Fail;
    }
    if (UA_StatusCode_isUncertain(response.value.responseHeader.serviceResult))
    {
        m_logger.Warning("Browse has uncertain value from Open62541: {}", UA_StatusCode_name(response.value.responseHeader.serviceResult));
    }

    if (response.value.results == nullptr)
    {
        throw std::runtime_error("response.value.results == nullptr");
    }

    for (size_t node_index = 0; node_index < response.value.resultsSize; ++node_index)
    {
        m_logger.Debug(
            "Total points: {}, Point number: {}, Presence of continuationPoint: {}",
            response.value.resultsSize,
            node_index,
            response.value.results[node_index].continuationPoint.length != 0); // NOLINT
        if (UA_StatusCode_isBad(response.value.results[node_index].statusCode)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            m_logger.Warning(
                "UA_BrowseResult has bad status '{}' of node {} in response.",
                UA_StatusCode_name(response.value.results[node_index].statusCode), // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                node_references_structure_lists.at(node_index).exp_node_id.ToString());
        }
        if (UA_StatusCode_isUncertain(response.value.results[node_index].statusCode)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            m_logger.Warning(
                "UA_BrowseResult has uncertain status '{}' of node {} in response.",
                UA_StatusCode_name(response.value.results[node_index].statusCode), // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                node_references_structure_lists.at(node_index).exp_node_id.ToString());
        }

        // continuationPoint
        for (size_t ref_index = 0; ref_index < response.value.results[node_index].referencesSize; ++ref_index) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        {
            // Node
            // Processing the browsing result
            node_references_structure_lists.at(node_index)
                .references.emplace_back(response.value.results[node_index].references[ref_index], UA_TYPES_REFERENCEDESCRIPTION); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        // Call BrowseNext. The condition prevents an unnecessary function call when everything has been read
        if (response.value.results[node_index].continuationPoint.length != 0) // NOLINT
        {
            if (BrowseNext(&response.value.results[node_index].continuationPoint, node_references_structure_lists.at(node_index).references) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                == StatusResults::Fail)
            {
                m_logger.Error("BrowseNext error with NodeID: {}", node_references_structure_lists.at(node_index).exp_node_id.ToString());
                return StatusResults::Fail;
            }
        }
    }
    return StatusResults::Good;
}

StatusResults Open62541ClientWrapper::ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& node_attr_structure_lists)
{
    m_logger.Trace("Method called: ReadNodesAtrrubutes()");
    std::unique_ptr<std::vector<UA_ReadValueId>, void (*)(std::vector<UA_ReadValueId>* const)> read_value_ids(
        new std::vector<UA_ReadValueId>,
        [](std::vector<UA_ReadValueId>* const vec)
        {
            // Delete all the contents of UA_READVALUEID structures by signs
            for (auto& read_value_id : *vec)
            {
                UA_ReadValueId_clear(&read_value_id);
            }
            // Delete Vector along with the content of UA_READVALUEID
            delete vec; // NOLINT(cppcoreguidelines-owning-memory)
        });
    read_value_ids->reserve(node_attr_structure_lists.size());
    size_t attr_index = 0;
    for (const auto& node_attr_structure_list : node_attr_structure_lists)
    {
        for (const auto& attr : node_attr_structure_list.attrs)
        {
            read_value_ids->emplace_back();
            UA_NodeId_copy(&node_attr_structure_list.exp_node_id.GetRef().nodeId, &read_value_ids->at(attr_index).nodeId);
            read_value_ids->at(attr_index).attributeId = attr.first;
            attr_index++;
        }
    }
    size_t const& flat_attr_numbers = attr_index;
    if (read_value_ids->size() != flat_attr_numbers)
    {
        throw std::runtime_error("read_value_ids.size() != flat_attr_numbers");
    }

    std::vector<std::optional<VariantsOfAttr>> variants(flat_attr_numbers);
    StatusResults result = ReadNodesAttributes(
        *read_value_ids,
        [&](size_t array_index, UA_DataValue& data_value, UA_NodeId& node_id, UA_UInt32 attr_id) // attr_index == array_index
        {
            if (!UA_StatusCode_isBad(data_value.status) && data_value.hasValue) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            {
                if (attr_id == UA_ATTRIBUTEID_VALUE)
                {
                    variants.at(array_index) = std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_Variant>(data_value.value, UA_TYPES_VARIANT))};
                }
                else
                {
                    variants.at(array_index) = UAVariantToStdVariant(data_value.value);
                }
            }
            else
            {
                variants.at(array_index) = std::nullopt;
                m_logger.Warning(
                    "ReadNodesAtrrubutes (atrID={}) has bad status '{}' of node {} in response",
                    attr_id,
                    UA_StatusCode_name(data_value.status), // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    UATypesContainer<UA_NodeId>(node_id, UA_TYPES_NODEID).ToString());
            }
        });

    if (result != StatusResults::Good)
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

} // namespace nodesetexporter::open62541