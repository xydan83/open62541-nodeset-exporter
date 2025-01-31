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
using ::nodesetexporter::open62541::UATypesContainer;
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
     * @param set_data Callback function to return the finished result.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodesAttributes(std::vector<UA_ReadValueId>& read_value_ids, const std::function<void(size_t, UA_DataValue&, UA_NodeId&, UA_UInt32)>& set_data);

public:
    /**
     * @brief Method for querying class attributes of a set of nodes.
     * @remark Attribute Service Set, Sync LowLevel - UA_Client_Service_read, async - __UA_Client_AsyncService
     * @param node_class_lists List of node class request-response structures.
     * @remark Attribute Service Set.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeClasses(std::vector<NodeClassesRequestResponse>& node_class_structure_lists) override;

    /**
     * @brief Method for querying references of multiple nodes.
     * @remark View Service Set - Browsing-Browsnext, Sync __UA_Client_Service, Async - __UA_Client_AsyncService
     * @param node_references_structure_lists List of node reference request-response structures.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists) override;

    /**
     * @brief Method for querying multiple attributes of multiple nodes.
     * @warning The value of the UA_ATTRIBUTEID_VALUE attribute is returned as a UA_Variant wrapped in std::optional<VariantsOfAttr>>.
     * This is done to make it easier to transfer values whose type is determined at runtime and work with them, since std::variant requires a static description of all possible types,
     * including arrays of different nesting, which makes it very difficult to convert possible types without knowing in advance what may come (unlike other attributes).
     * A table of the relationship between attributes and data types can be seen at the following reference:
     * https://www.open62541.org/doc/master/core_concepts.html#information-modelling
     * @remark Attribute Service Set, Sync LowLevel - UA_Client_Service_read, async - __UA_Client_AsyncService
     * @param node_attr_structure_lists List of node attribute request-response structures.
     * @remark Attribute Service Set
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& node_attr_structure_lists) override;

    /**
     * @brief Method for querying the value of a single node.
     * @remark Attribute Service Set, Sync - UA_Client_readValueAttribute, async - UA_Client_readValueAttribute_async
     * @param node_id The node for which the value is requested. * @param data_value [out] The value of the node.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults ReadNodeDataValue(const UATypesContainer<UA_ExpandedNodeId>& node_id, UATypesContainer<UA_Variant>& data_value) override;

    /**
     * @brief The method specifies the maximum number of references to return for each starting node specified in the request.
     *        If specified during the Browsing request, no more than the specified number of links is returned.
     *        To obtain the remaining links, the BrowseNext operation will be used.
     *        Used in the ReadNodeReferences request.
     */
    [[maybe_unused]] void SetRequestedMaxReferencesPerNode(std::uint32_t requested_max_references_per_node)
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

private:
    UA_Client& m_ua_client;
    std::uint32_t m_requested_max_references_per_node = 0;
};

} // namespace nodesetexporter::open62541

#endif // NODESETEXPORTER_OPEN62541_CLIENTWRAPPERS_H