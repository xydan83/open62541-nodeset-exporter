//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_CLIENTWRAPPERS_H
#define NODESETEXPORTER_OPEN62541_CLIENTWRAPPERS_H

#include "nodesetexporter/interfaces/IOpen62541.h"

#include <open62541/client_highlevel.h>

#include <functional>

namespace nodesetexporter::open62541
{

using nodesetexporter::common::LogLevel;
using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using StatusResults = nodesetexporter::common::statuses::StatusResults<int64_t>;
using nodesetexporter::interfaces::IOpen62541;
using nodesetexporter::open62541::typealiases::UAVariantToStdVariant;
using nodesetexporter::open62541::typealiases::VariantsOfAttr;


class Open62541ClientWrapper final : public IOpen62541
{
public:
    explicit Open62541ClientWrapper(UA_Client& ua_client, LoggerBase& logger)
        : IOpen62541(logger)
        , m_ua_client(ua_client)
    {
    }
    ~Open62541ClientWrapper() override = default;
    Open62541ClientWrapper(Open62541ClientWrapper&) = delete;
    Open62541ClientWrapper(Open62541ClientWrapper&&) = delete;
    Open62541ClientWrapper& operator=(const Open62541ClientWrapper& obj) = delete;
    Open62541ClientWrapper& operator=(Open62541ClientWrapper&& obj) = delete;

private:
    /**
     * @brief Method for querying a set of node links. Batch processing is supported.
     * @param node_references_structure_lists List of node reference request-response structures.
     * @param browse_offset The offset, for the internal request of the Browse function, at which the node_references_structure_lists structure is filled.
     * So, in order not to make intermediate elements and to avoid copying, we turn directly to the structure
     * node_references_structure_lists by index, to fill in the necessary elements received from the server.
     * @param b_req Structure for Browse requesting data in Open62541 library format.
     * @param total_read_ref Total number of nodes read per call to the Browse operation, including BrowseNext. The counter is added to the previous results.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults Browse(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists, size_t browse_offset, const UA_BrowseRequest& b_req, size_t& total_read_ref);
    /**
     * @brief Implementation of a technique for selecting node references in parts, in portions.
     * @warning It has a limitation; in one call it performs a complete selection of only one continuation_point (or one parent node).
     * @param continuation_point An object representing the description of the possibility of further reading data in portions.
     * @param result_nodes Array where the references of the node being retrieved will be written.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults BrowseNext(UA_ByteString* continuation_point, std::vector<UATypesContainer<UA_ReferenceDescription>>& result_nodes);

    /**
     * @brief A method for querying multiple attributes from multiple nodes.
     * @param nodes_and_attr An array of node value structures to the requested attribute.
     * @param set_data Callback function to return the finished result
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodesAttributes(std::vector<UA_ReadValueId>& read_value_ids, const std::function<void(size_t, UA_DataValue&, UA_NodeId&, UA_AttributeId)>& set_data);

public:
    /**
     * @brief The method calculates the limit on the number of concurrently requested links to child nodes from parent nodes by browsing by sending the NodeID to the OPC UA server (Browsing).
     * @tparam TCollection A collection of the NodeID list supporting the dimension function size_t size().
     * @param numbers_of_nodes Number of nodes to process.
     * @param max_nodes_per_browse Maximum number of parallel nodes per Browsing operation
     * @param max_browse_continuation_points The maximum number of points that are used to complete data reading through the BrowseNext operation
     * @param requested_max_references_per_node The maximum number of references that the client is willing to accept in a response from the server to a Browsing(Next) request
     * @return Returns the maximum number of NodeIDs that can be sent to the operation of obtaining references to child nodes in the form of NodeID
     */
    [[nodiscard]] static size_t CalculateBrowseLimit(
        size_t numbers_of_nodes,
        uint32_t max_nodes_per_browse,
        uint16_t max_browse_continuation_points,
        uint32_t requested_max_references_per_node) noexcept;

    /**
     * @brief Method for querying class attributes of a set of nodes.
     * @remark Attribute Service Set, Sync LowLevel - UA_Client_Service_read, async - __UA_Client_AsyncService
     * @param node_class_lists List of node class request-response structures.
     * @remark Attribute Service Set.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeClasses(std::vector<NodeClassesRequestResponse>& node_class_structure_lists) override;

    /**
     * @brief Method for querying links of multiple nodes. This function reads links taking into account the set limits, such as:
     * max_references_per_node, max_nodes_per_browse, max_browse_continuation_points.
     * If a limit is detected, the function splits the request packet into the calculated limit.
     * The function supports operations: Browse, BrowseNext.
     * @remark View Service Set - Browsing-Browsnext, Sync __UA_Client_Service, Async - __UA_Client_AsyncService
     * @param node_references_structure_lists List of node reference request-response structures.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists) override;

    /**
     * @brief Method for querying multiple attributes of multiple nodes.
     * @warning The value of the UA_ATTRIBUTEID_VALUE attribute is returned as a UA_Variant wrapped in std::optional<VariantsOfAttr>>.
     * This is done to make it easier to pass values whose type is determined at runtime and work with them, as
     * std::variant requires a static description of all possible types, including arrays of different nesting, which makes it very difficult
     * conversion of possible types, without knowing in advance what might come (unlike other attributes).
     * A table of the relationship between attributes and data types can be seen at the following link:
     * https://www.open62541.org/doc/master/core_concepts.html#information-modelling
     * @remark Attribute Service Set, Sync LowLevel - UA_Client_Service_read, async - __UA_Client_AsyncService
     * @param node_attr_structure_lists List of node attribute request-response structures.
     * @param attr_sum The total sum of attributes across all links. Each node can have an n-number of attributes.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& node_attr_structure_lists, size_t attr_sum) override;

    /**
     * @brief Method for querying the value of a single node.
     * @remark Attribute Service Set, Sync - UA_Client_readValueAttribute, async - UA_Client_readValueAttribute_async
     * @param node_id The node for which the value is requested.
     * @param data_value [out] The value of the node.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeDataValue(const UATypesContainer<UA_ExpandedNodeId>& node_id, UATypesContainer<UA_Variant>& data_value) override;

    /**
     * @brief The method specifies the maximum number of references to return for each starting node specified in the request.
     *        If specified during the Browsing request, no more than the specified number of links is returned.
     *        To obtain the remaining links, the BrowseNext operation will be used.
     *        Used in the ReadNodeReferences request.
     */
    [[maybe_unused]] void SetRequestedMaxReferencesPerNode(std::uint32_t requested_max_references_per_node) override
    {
        m_requested_max_references_per_node = requested_max_references_per_node;
    }

    /**
     * @brief The method returns the maximum number of references returned for each starting node specified in the request.
     *        The use of the parameter is described in the SetRequestedMaxReferencesPerNode method.
     */
    [[nodiscard]] std::uint32_t GetRequestedMaxReferencesPerNode() const
    {
        return m_requested_max_references_per_node;
    }

    /**
     * @brief The method specifies the maximum number of nodes that can be transferred to the server in one Browse operation request.
     * If the specified limit is exceeded, the request is divided into the required number of Browse operations.
     */
    [[maybe_unused]] void SetMaxNodesPerBrowse(std::uint32_t max_nodes_per_browse) override
    {
        m_max_nodes_per_browse = max_nodes_per_browse;
    }

    /**
     * @brief The method returns the specified maximum number of nodes that can be transferred to the server in one Browse operation request.
     */
    [[nodiscard]] std::uint32_t GetMaxNodesPerBrowse() const
    {
        return m_max_nodes_per_browse;
    }

    /**
     * @brief The method specifies the maximum number of possible continuation_points during the Browse and BrowseNext operations.
     * Each link request from one NodeID is equal to one continuation_points. The set of NodeIDs must be limited by this parameter per request.
     */
    [[maybe_unused]] void SetMaxBrowseContinuationPoints(std::uint16_t max_browse_continuation_points) override
    {
        m_max_browse_continuation_points = max_browse_continuation_points;
    }

    /**
     * @brief The method returns the specified maximum number of possible continuation_points for the Browse and BrowseNext operations.
     */
    [[nodiscard]] std::uint32_t GetMaxBrowseContinuationPoints() const
    {
        return m_max_browse_continuation_points;
    }

    /**
     * @brief Method specifies the maximum number of node attribute requests per read operation during the Client_Service_read operation.
     * It is important to note that one attribute request is equal to one request from the node.
     * For example, it is necessary to request three attributes in one request for one NodeID. In this case you need
     * take into account that this will be equivalent to three nodes, although the NodeID will be the same, i.e.
     * ns=2;i=44 - UA_ATTRIBUTEID_BROWSENAME
     * ns=2;i=44 - UA_ATTRIBUTEID_DISPLAYNAME
     * ns=2;i=44 - UA_ATTRIBUTEID_DESCRIPTION
     * As a result, there are already three nodes_per_read.
     */
    [[maybe_unused]] void SetMaxNodesPerRead(std::uint32_t max_nodes_per_read) override
    {
        m_max_nodes_per_read = max_nodes_per_read;
    }

    /**
     * @brief The method returns the configured maximum number of node attribute requests per read operation during the Client_Service_read operation.
     */
    [[nodiscard]] std::uint32_t GetMaxNodesPerRead() const
    {
        return m_max_nodes_per_read;
    }

private:
    UA_Client& m_ua_client;
    std::uint32_t m_requested_max_references_per_node = 0;
    std::uint32_t m_max_nodes_per_browse = 0;
    std::uint16_t m_max_browse_continuation_points = 0;
    std::uint32_t m_max_nodes_per_read = 0;
};

} // namespace nodesetexporter::open62541

#endif // NODESETEXPORTER_OPEN62541_CLIENTWRAPPERS_H