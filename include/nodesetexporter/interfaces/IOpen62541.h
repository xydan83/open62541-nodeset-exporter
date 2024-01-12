//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_INTERFACES_IOPEN62541_H
#define NODESETEXPORTER_INTERFACES_IOPEN62541_H

#include "nodesetexporter/common/LoggerBase.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/open62541/TypeAliases.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/types_generated_handling.h>

#include <map>
#include <optional>
#include <vector>

namespace nodesetexporter::interfaces
{
using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using StatusResults = ::nodesetexporter::common::statuses::StatusResults;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttr;

/**
 * @brief An abstract class of methods for interacting with the Open62541 library client/server to access attributes of nodes and other entities.
 * @warning The error status is displayed as a result of each function (except setters or getters),
 *          but the error description must be logged in interface implementations.
 *          If it is possible to correct the error locally or it does not have a critical consequence, then the fact itself should be displayed in the log,
 *          but the method should continue to work and, in the absence of critical errors in other places, issue a success status.
 */

class IOpen62541
{
public:
    explicit IOpen62541(LoggerBase& logger)
        : m_logger(logger)
    {
    }
    virtual ~IOpen62541() = default;
    IOpen62541(IOpen62541&) = delete;
    IOpen62541(IOpen62541&&) = delete;
    IOpen62541& operator=(const IOpen62541& obj) = delete;
    IOpen62541& operator=(IOpen62541&& obj) = delete;


    /**
     * @brief Data request structure by node class.
     * @warning You cannot create a structure and simultaneously initialize exp_node_id at the same time as creating UATypesContainer through an initializer or passing directly to the constructor,
     *          since exp_node_id does not store an object, and UATypesContainer will be created directly in the NodeClassesRequestResponse constructor and will be immediately deleted after
     *          the constructor works.
     *          For example: "NodeClassesRequestResponse{UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=500"), UA_TYPES_EXPANDEDNODEID)};" such a creation is not valid.
     */
    struct NodeClassesRequestResponse
    {
        NodeClassesRequestResponse(const UATypesContainer<UA_ExpandedNodeId>& exp_node_id, UA_NodeClass node_cl = UA_NodeClass::UA_NODECLASS_UNSPECIFIED) // NOLINT(google-explicit-constructor)
            : exp_node_id(exp_node_id)
            , node_class(node_cl)
        {
        }
        // Request
        const UATypesContainer<UA_ExpandedNodeId>& exp_node_id;
        // Response [out]
        UA_NodeClass node_class;
    };

    /**
     * @brief Request structure for node reference data.
     * @warning You cannot create a structure and simultaneously initialize exp_node_id at the same time as creating UATypesContainer through an initializer or passing directly to the constructor,
     *          since exp_node_id does not store an object, and UATypesContainer will be created directly in the NodeClassesRequestResponse constructor and will be immediately deleted after
     *          the constructor works.
     *          For example: "NodeClassesRequestResponse{UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID("ns=2;i=500"), UA_TYPES_EXPANDEDNODEID)};" such a creation is not valid.
     */
    struct NodeReferencesRequestResponse
    {
        NodeReferencesRequestResponse(const UATypesContainer<UA_ExpandedNodeId>& exp_node_id, std::vector<UATypesContainer<UA_ReferenceDescription>>&& ref = {}) // NOLINT(google-explicit-constructor)
            : exp_node_id(exp_node_id)
            , references(std::move(ref))
        {
        }
        // Request
        const UATypesContainer<UA_ExpandedNodeId>& exp_node_id;
        // Response [out]
        std::vector<UATypesContainer<UA_ReferenceDescription>> references;
    };

    /**
     * @brief Structure of a data request based on node attributes.
     */
    struct NodeAttributesRequestResponse
    {
        // Request
        const UATypesContainer<UA_ExpandedNodeId>& exp_node_id;
        // {Request, Response [out]}
        // todo Replace std::optional with variant::monostate
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs;
    };

    /**
     * @brief Method for querying class attributes of a set of nodes.
     * @param node_class_lists List of node class request-response structures.
     * @remark Attribute Service Set.
     * @return Request execution status.
     */
    [[nodiscard]] virtual StatusResults ReadNodeClasses(std::vector<NodeClassesRequestResponse>& node_class_structure_lists) = 0;
    /**
     * @brief Method for querying references of multiple nodes.
     * @param node_references_structure_lists List of node reference request-response structures.
     * @remark View Service Set - Browse.
     * @return Request execution status.
     */
    [[nodiscard]] virtual StatusResults ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& node_references_structure_lists) = 0;
    /**
     * @brief Method for querying multiple attributes of multiple nodes.
     * @param node_attr_structure_lists List of node attribute request-response structures.
     * @remark Attribute Service Set
     * @return Request execution status.
     */
    [[nodiscard]] virtual StatusResults ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& node_attr_structure_lists) = 0;
    /**
     * @brief Method for querying the value of a single node.
     * @param node_id The node for which the value is requested.
     * @param data_value [out] The value of the node.
     * @return Request execution status.
     */
    [[nodiscard]] virtual StatusResults ReadNodeDataValue(const UATypesContainer<UA_ExpandedNodeId>& node_id, UATypesContainer<UA_Variant>& data_value) = 0;

protected:
    LoggerBase& m_logger; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

} // namespace nodesetexporter::interfaces

#endif // NODESETEXPORTER_INTERFACES_IOPEN62541_H
