//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_NODESETEXPORTERLOOP_H
#define NODESETEXPORTER_NODESETEXPORTERLOOP_H

#include "nodesetexporter/common/LoggerBase.h"
#include "nodesetexporter/common/Open62541CompatibilityCheck.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/interfaces/IEncoder.h"
#include "nodesetexporter/interfaces/IOpen62541.h"
#include "nodesetexporter/open62541/NodeIntermediateModel.h"
#include "nodesetexporter/open62541/TypeAliases.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/nodeids.h>
#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include <map>
#include <optional>
#include <set>
#include <variant>

namespace nodesetexporter
{

#pragma region Using_declarations_to_some_types

using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LogLevel = nodesetexporter::common::LogLevel;
using IEncoder = ::nodesetexporter::interfaces::IEncoder;
using IOpen62541 = ::nodesetexporter::interfaces::IOpen62541;
using NodeIntermediateModel = ::nodesetexporter::open62541::NodeIntermediateModel;
using StatusResults = ::nodesetexporter::common::statuses::StatusResults;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttr;
using ExpandedNodeId = nodesetexporter::open62541::UATypesContainer<UA_ExpandedNodeId>;

#pragma endregion Using_declarations_to_some_types

/**
 * @brief The main core of the algorithm for exporting OPC UA node structure to a specific format
 */
class NodesetExporterLoop final
{

#pragma region Default parameter constants
private:
    static constexpr auto default_number_of_max_nodes_to_request_data = 50000;
#pragma endregion Default parameter constants

private:
#pragma region Methods for obtaining and generating data

#pragma region Getting ID attribute
    /**
     * @brief Prepare a request and get all the necessary node attributes.
     * @param range_for_nodes The range of operation within the list of nodes node_ids and node_classes_req_res. Used for batch requests.
     * @param node_classes_req_res List of structures containing the node class.
     * @param nodes_attr_req_res [out] List of attributes bound to their NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeAttributes(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
        const std::pair<int64_t, int64_t>& range_for_nodes,
        const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
        std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res);

    /**
     * @brief Get the underlying node attribute IDs
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetCommonNodeAttributes() const
    {
        m_logger.Trace("Method called: GetCommonNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{
            {UA_ATTRIBUTEID_BROWSENAME, std::nullopt},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::nullopt},
            {UA_ATTRIBUTEID_DESCRIPTION, std::nullopt},
            {UA_ATTRIBUTEID_WRITEMASK, std::nullopt},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::nullopt}};
    }

    /**
     * @brief Get attribute IDs of a node of type Object
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetObjectNodeAttributes() const
    {
        m_logger.Trace("Method called: GetObjectNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_ATTRIBUTEID_EVENTNOTIFIER, std::nullopt}};
    }

    /**
     * @brief Get attribute identifiers of a node of type Object Type
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetObjectTypeNodeAttributes() const
    {
        m_logger.Trace("Method called: GetObjectTypeNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_ATTRIBUTEID_ISABSTRACT, std::nullopt}};
    }

    /**
     * @brief Get attribute identifiers of a node of type Variable
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetVariableNodeAttributes() const
    {
        m_logger.Trace("Method called: GetVariableNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{
            {UA_ATTRIBUTEID_DATATYPE, std::nullopt},
            {UA_ATTRIBUTEID_VALUERANK, std::nullopt},
            {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::nullopt},
            {UA_ATTRIBUTEID_VALUE, std::nullopt},
            {UA_ATTRIBUTEID_ACCESSLEVEL, std::nullopt},
            {UA_ATTRIBUTEID_USERACCESSLEVEL, std::nullopt},
            {UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, std::nullopt},
            {UA_ATTRIBUTEID_HISTORIZING, std::nullopt}};
    }

    /**
     * @brief Get node attribute identifiers of type Variable Type
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetVariableTypeNodeAttributes() const
    {
        m_logger.Trace("Method called: GetVariableTypeNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{
            {UA_ATTRIBUTEID_ISABSTRACT, std::nullopt},
            {UA_ATTRIBUTEID_DATATYPE, std::nullopt},
            {UA_ATTRIBUTEID_VALUERANK, std::nullopt},
            {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::nullopt},
            {UA_ATTRIBUTEID_VALUE, std::nullopt}};
    }

    /**
     * @brief Get attribute identifiers of a node of type Reference Type
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetReferenceTypeNodeAttributes() const
    {
        m_logger.Trace("Method called: GetReferenceTypeNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_ATTRIBUTEID_INVERSENAME, std::nullopt}, {UA_ATTRIBUTEID_ISABSTRACT, std::nullopt}, {UA_ATTRIBUTEID_SYMMETRIC, std::nullopt}};
    }

    /**
     * @brief Get attribute identifiers of a node of type DataType
     * @return A dictionary with key-filled identifiers. Values are empty.
     */
    [[nodiscard]] std::map<UA_AttributeId, std::optional<VariantsOfAttr>> GetDataTypeNodeAttributes() const
    {
        m_logger.Trace("Method called: GetDataTypeNodeAttributes()");
        return std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_ATTRIBUTEID_DATATYPEDEFINITION, std::nullopt}, {UA_ATTRIBUTEID_ISABSTRACT, std::nullopt}};
    }

    // todo Do I need to add support for View attribute query?
#pragma endregion Retrieving the ID attribute

#pragma region Retrieving and Processing Links

    /**
     * @brief Prepare a query and get a list of references for each node.
     * @param node_ids List of NodeIds of nodes that participate in the export.
     * @param range_for_nodes The range of operation within the list of nodes node_ids and node_classes_req_res. Used for batch requests.
     * @param node_references_req_res List of references associated with NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeReferences(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
        const std::pair<int64_t, int64_t>& range_for_nodes,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Method for processing references for working with the KepServer server (and similar ones with similar features)
     * @param node_references_req_res List of references associated with NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults KepServerRefFix(std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Remove references to ignored, known nodes.
     * @param index The index of the node associated with the references.
     * @param node_references_req_res List of references associated with NodeID.
     */
    void DeleteFailedReference(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Removing back references in nodes of classes of types ReferenceTypes, DataTypes, ObjectTypes, VariableTypes other than HasSubtype.
     * @param node_index The index of the node associated with the references.
     * @param node_class The class of the node associated with the references.
     * @param node_references_req_res List of references associated with NodeID.
     */
    void DeleteNotHasSubtypeReference(size_t node_index, UA_NodeClass node_class, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Method for setting the start node.
     * @param node_index The index of the node associated with the references.
     * @param node_class The class of the node associated with the references.
     * @param node_references_req_res List of references associated with NodeID.
     * @param has_start_node_subtype_detected If a start node with a TYPE class is detected, the value of the variable is set to true.
     * @param start_node_reverse_reference_counter Returns the number of reverse references found in the start node.
     */
    void AddStartNodeIfNotFound(
        size_t node_index,
        UA_NodeClass node_class,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res,
        bool& has_start_node_subtype_detected,
        uint64_t& start_node_reverse_reference_counter);

    /**
     * @brief Get the ParentID from the back references. The first reverse reference found is taken. Does not return the parent if the node is of class TYPE and has no backreference type
     * UA_NS0ID_HASSUBTYPE.
     * @param node_index The index of the node associated with the references.
     * @param node_class The class of the node associated with the references.
     * @param node_references_req_res List of references associated with NodeID.
     * @return Returns the parent NodeID from the back references for the node at node_index.
     */
    [[nodiscard]] std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> GetParentNodeId(
        size_t node_index,
        UA_NodeClass node_class,
        const std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Returns the NodeID of the underlying node types.
     * @param node_class The node class.
     * @return NodeID of the node's base type depending on the node class passed in.
     */
    std::unique_ptr<UATypesContainer<UA_ExpandedNodeId>> GetBaseObjectType(UA_NodeClass node_class);

#pragma endregion Retrieving and Processing Links

    /**
     * @brief The method returns all namespaces available on the OPC UA server for export, with the exception of the standard OPC UA space.
     * @param namespaces [out] List of strings from the available non-standard Server namespaces.
     * // todo make a filter so that not all spaces in a row are included in the list, but only those associated with the exported nodes.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNamespaces(std::vector<std::string>& namespaces);

    /**
     * @brief The method generates a set of unique alias identifiers (Aliases)
     * @param node_intermediate_objs A set of generated data on nodes required for export.
     * @param aliases [out] A unique set of NodeID aliases. The method uses insert without deleting content.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetAliases(const std::vector<NodeIntermediateModel>& node_intermediate_objs, std::map<std::string, UATypesContainer<UA_NodeId>>& aliases);

    /**
     * @brief Method for getting node classes by NodeID.
     * @param node_ids List of NodeIds of nodes that participate in the export.
     * @param range_for_nodes The range of operation within the list of nodes node_ids. Used for batch requests.
     * @param node_classes_req_res [out] List of structures containing the node class.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeClasses(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
        const std::pair<int64_t, int64_t>& range_for_nodes,
        std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res);

    /**
     * @brief The main method for obtaining the necessary data to generate a list of intermediate structures describing the main parameters of a node and its attributes.
     * @remark node_ids is a synchronizer for all arrays of structures based on NodeID as a sequence of elements.
     *         The basic rule is that any request for nodes in a certain sequence and number of nodes must be equal to the number and the same sequence of responses.
     *         This rule is used in the OPC UA standard when receiving a response from a server where the main parameter is a set of nodes.
     *         For example: "The size and order of the list matches the size and order of the nodesToBrowsespecified in the request (Browsing)",
     *         https://reference.opcfoundation.org/Core/Part4/v104/docs/5.8.2.2
     *         or: "The size and order of this list matches the size and order of the nodesToReadrequest parameter (Read)".
     *         https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2.2
     * @param node_ids List of NodeIds of nodes that participate in the export.
     * @param range_for_nodes The range of operation within the list of nodes node_ids and node_classes_req_res. Used for batch requests.
     * @param node_classes_req_res List of structures containing the node class.
     * @param node_models [out] List of intermediate structures describing the main parameters of nodes and their attributes.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodesData(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
        const std::pair<int64_t, int64_t>& range_for_nodes,
        const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
        std::vector<NodeIntermediateModel>& node_models);

#pragma endregion Methods for obtaining and generating data


#pragma region Data Export Methods

    /**
     * @brief A method that executes a set of possible instructions before exporting begins.
     * @return The execution status of the method.
     */
    [[nodiscard]] StatusResults Begin()
    {
        m_logger.Trace("Method called: Begin()");
        m_logger.Info("Start of export...");
        memset(&m_exported_nodes, 0, sizeof(ExportedNodes));
        return m_export_encoder.Begin();
    };

    /**
     * @brief A method that executes a set of possible instructions at the end of the export.
     * @return The execution status of the method.
     */
    [[nodiscard]] StatusResults End() const
    {
        m_logger.Trace("Method called: End()");
        m_logger.Info("End of export");
        return m_export_encoder.End();
    };

    /**
     * @brief Method that adds node spaces to the export.
     * @param namespaces List of server namespaces.
     * @return The execution status of the method.
     */
    [[nodiscard]] StatusResults ExportNamespaces(const std::vector<std::string>& namespaces) const
    {
        m_logger.Trace("Method called: ExportNamespaces()");
        m_logger.Info("Export namespaces:");
        if (m_logger.IsEnable(common::LogLevel::Debug))
        {
            for (const auto& nmpspc : namespaces)
            {
                m_logger.Debug("  {}", nmpspc);
            }
        }
        return m_export_encoder.AddNamespaces(namespaces);
    }

    /**
     * @brief Method that adds aliases for node types involved in the export.
     * @param aliases Unique NodeID objects that represent type aliases.
     * @return The execution status of the method.
     */
    [[nodiscard]] StatusResults ExportAliases(const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases) const
    {
        m_logger.Trace("Method called: ExportAliases()");
        m_logger.Info("Export aliases:");
        if (m_logger.IsEnable(common::LogLevel::Debug))
        {
            for (const auto& alias : aliases)
            {
                m_logger.Debug("  Alias: {}, nodeId: {}", alias.first, alias.second.ToString());
            }
        }
        return m_export_encoder.AddAliases(aliases);
    }

    /**
     * @brief Method that exports the data of each node depending on its class.
     * @param list_of_nodes_data A list of intermediate structures describing the main parameters of nodes and their attributes.
     * @return The execution status of the method.
     */
    // todo Add a text description of the DataType and ReferenceType to the NodeIntermediateModel.
    [[nodiscard]] StatusResults ExportNodes(const std::vector<NodeIntermediateModel>& list_of_nodes_data);

#pragma endregion Data Export Methods
public:
    /**
     * @brief Constructor for the node export object.
     * @param node_ids List of nodes to export.
     * @param open62541_lib Implementation of the IOpen62541 interface.
     * @param export_encoder Implementation of the IEncoder interface.
     * @param logger Logging methods.
     * @param parent_start_node_replacer ID of the node that will be substituted as the main parent for the start node if the parent of the start node is not ns=0;i=85.
     * @param perf_counter_enable Enable performance counters and output the results to the log. The result is displayed in the Info level mode.
     */
    NodesetExporterLoop(
        std::map<std::string, std::vector<ExpandedNodeId>> node_ids,
        IOpen62541& open62541_lib,
        IEncoder& export_encoder,
        LoggerBase& logger,
        const UATypesContainer<UA_ExpandedNodeId>& parent_start_node_replacer,
        bool perf_counter_enable)
        : m_node_ids(std::move(node_ids))
        , m_logger(logger)
        , m_open62541_lib(open62541_lib)
        , m_export_encoder(export_encoder)
        , m_parent_start_node_replacer(parent_start_node_replacer)
        , m_is_perf_timer_enable(perf_counter_enable)
    {
        m_logger.Trace("Constructor called: NodesetExporterLoop()");
    }

    /**
     * @brief Sets the maximum number of nodes for which data must be received from the server in one request.
     * @param number
     */
    void SetNumberOfMaxNodesToRequestData(u_int32_t number)
    {
        m_logger.Trace("Method called: SetNumberOfMaxNodesToRequestData()");
        m_number_of_max_nodes_to_request_data = number;
    }

    /**
     * @brief Method to start a chain by exporting nodes of their accompanying data.
     * The export scheme is based on the description of the node structure of the 1.04 standard
     * https://files.opcfoundation.org/schemas/UA/1.04/UANodeSet.xsd
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults StartExport();

private:
    // todo Consider switching from std::variant to a custom std::set that does not sort (guarantees insertion order)
    std::map<std::string, std::vector<ExpandedNodeId>> m_node_ids;
    LoggerBase& m_logger;
    IOpen62541& m_open62541_lib;
    IEncoder& m_export_encoder;
    const UATypesContainer<UA_ExpandedNodeId>& m_parent_start_node_replacer;

    const UA_NodeId m_ns0id_objectfolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    const UA_NodeId m_hascomponent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    const UA_NodeId m_hastypedefenition_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    const UA_NodeId m_basevariabletype_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE);
    const UA_NodeId m_hassubtype_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);

    u_int32_t m_number_of_max_nodes_to_request_data = default_number_of_max_nodes_to_request_data;
    // A list of basic hierarchical link types in the form of an associative container, consisting of - "link type value: string name of the link type."
    static const std::map<std::uint32_t, std::string> m_hierarhical_references;
    static const std::map<std::uint32_t, std::string> m_ignored_nodeclasses;
    // List of node classes representing TYPES. Represents an associative container of - "type value: string type name".
    static const std::map<std::uint32_t, std::string> m_types_nodeclasses;
    // List of ignored nodes that should not be added to the export dump.
    std::set<UATypesContainer<UA_ExpandedNodeId>> m_ignored_node_ids;
    // Copies of all NodeIDs in the Set to quickly search for the desired node for the reference correction filter.
    // In the global version, it is especially necessary when nodes are processed in batches
    // if m_number_of_max_nodes_to_request_data > 0.
    // Since such a list copies all nodes for processing, it is made through a reference_wrapper in order to store not container objects with NodeID, but references to them (pointers)
    std::set<std::reference_wrapper<UATypesContainer<UA_ExpandedNodeId>>, std::less<UATypesContainer<UA_ExpandedNodeId>>> m_node_ids_set_copy; // NOLINT
    bool m_is_perf_timer_enable;
    struct ExportedNodes
    {
        size_t object_nodes;
        size_t variable_nodes;
        size_t objecttype_nodes;
        size_t variabletype_nodes;
        size_t referencetype_nodes;
        size_t datatype_nodes;
        size_t method_nodes;
        size_t view_nodes;
        size_t unspecified_nodes;

        /**
         * @brief Outputs all fields as a string for statistics.
         */
        [[nodiscard]] std::string ToString() const
        {
            return "NODECLASS OBJECT: " + std::to_string(object_nodes) + "\nNODECLASS VARIABLE: " + std::to_string(variable_nodes) + "\nNODECLASS OBJECTTYPE: " + std::to_string(objecttype_nodes)
                   + "\nNODECLASS VARIABLETYPE: " + std::to_string(variabletype_nodes) + "\nNODECLASS REFERENCETYPE: " + std::to_string(referencetype_nodes)
                   + "\nNODECLASS DATATYPE: " + std::to_string(datatype_nodes) + "\nNODECLASS METHOD: " + std::to_string(method_nodes) + "\nNODECLASS VIEW: " + std::to_string(view_nodes)
                   + "\nNODECLASS UNSPECIFIED: " + std::to_string(unspecified_nodes);
        }

        /**
         * @brief Get the sum of all nodes except undefined ones.
         */
        [[nodiscard]] size_t GetSumm() const
        {
            return object_nodes + variable_nodes + objecttype_nodes + variabletype_nodes + referencetype_nodes + datatype_nodes + method_nodes + view_nodes;
        }
    } m_exported_nodes = {0};
};

} // namespace nodesetexporter

#endif // NODESETEXPORTER_NODESETEXPORTERLOOP_H
