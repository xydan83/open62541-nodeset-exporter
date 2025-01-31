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
using StatusResults = nodesetexporter::common::statuses::StatusResults<int64_t>;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttr;
using ExpandedNodeId = nodesetexporter::open62541::UATypesContainer<UA_ExpandedNodeId>;

#pragma endregion Using_declarations_to_some_types

/**
 * @brief The main core of the algorithm for exporting OPC UA node structure to a specific format
 */
class NodesetExporterLoop final
{
public:
    /**
     * @brief structure with additional parameters.
     * @param is_perf_timer_enable Inclusion of performance meters and output of the results in the log. The result is displayed in the Info level mode.
     * @param ns0_custom_nodes_read_to_work Take user nodes located in the OPC UA (ns = 0) namespace.
     * @param flat_list_of_nodes__is_enable form a flat list of nodes tied to one starting. In this case, all family ties between the nodes are removed.
     * It is possible to specify ns = 85 from the standard OPC UA unit as a starting node. All other nodes of the standard are prohibited.
     * In the absence of the specified starting assembly on the server and the deactivated Create_Missing_start_node parameter, an export error will be issued.
     * @param flat_list_of_nodes__create_missing_start_node works in conjunction only with an activated parameter flat_list_of_nodes__is_enable.
     * In the case of activation, if the start node does not exist (when trying to collect data from it there will be an error), then such a node will be created in the form of Object class,
     * tied to the Parent_node_replacer specified in the parameter to the parent node. If all other lists of nodes will have one
     * The existing node, then such lists will be all tied to one created node.
     * @param flat_list_of_nodes__allow_abstract_variable works in conjunction with flat_list_of_nodes__create_missing_start_node and flat_list_of_nodes__is_enable.
     * In case of activation, adds two reverse links such as HasComponent to nodes i=63 and i=58 Thus allows the use of class nodes
     * Variable abstract types.
     * @param parent_start_node_replacer the node, which will be substituted as the main parent to the launch unit if the parent of the start node is not ns=0;i=85.
     * By default, a node with NodeId i=85 will be installed as a parental node parent.
     * For encoder_types XML, both ParentNodeID and the link to the main parent are changing.
     * @param ignored_nodeclasses User list of ignored classes of export units. In the case of an indication of any class of the node, all nodes of this class are ignored
     * from lists of nodes. Behind the nodes of ignored classes, all subsidiaries of other classes will be removed, as a chain of connections will be destroyed.
     * By default, Method classes are always considered ignored, View - regardless of the content of this list.
     */
    struct Options
    {
        bool is_perf_timer_enable;
        bool ns0_custom_nodes_ready_to_work;
        struct
        {
            bool is_enable;
            bool create_missing_start_node;
            bool allow_abstract_variable;
        } flat_list_of_nodes{};
        UATypesContainer<UA_ExpandedNodeId> parent_start_node_replacer;
        //        std::vector<UA_NodeClass> ignored_nodeclasses;
    };

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
        const std::pair<size_t, size_t>& node_range,
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
        const std::pair<size_t, size_t>& node_range,
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
    void DeleteFailedReferences(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Removing all hierarchical links in the node.
     * @param index Index associated with the links of the node.
     * @param node_references_req_res List of links tied to nodeid.
     */
    void DeleteAllHierarhicalReferences(size_t node_index, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

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
     * @brief The method of adding attributes to an artificially created start node.
     * @param node_attr_res_req List of attributes tied to NodeId.
     * @param node_references_req_res List of links tied to NodeId.
     */
    void CreateAttributesForStartNode(std::vector<IOpen62541::NodeAttributesRequestResponse>& node_attr_res_req, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief The method of adding a link of a certain type of specified node of the PA index to another node.
     * @param ref_pointing_to_node_id The node to which the link will indicate.
     * @param add_ref_to_node_by_index The node index in which the link will be placed.
     * @param reference_type Type of reference.
     * @param is_forward Direct or reverse reference.
     * @param node_references_req_res List of links tied to NodeId.
     */
    void AddCustomRefferenceToNodeID( // NOLINT(readability-inconsistent-declaration-parameter-name)
        const UATypesContainer<UA_ExpandedNodeId>& ref_pointing_to_node_id,
        size_t add_ref_to_node_by_index,
        uint32_t reference_type_id,
        bool is_forward,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

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
        const std::pair<size_t, size_t>& node_range,
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
        const std::pair<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>& node_ids,
        const std::pair<size_t, size_t>& node_range,
        const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
        std::vector<NodeIntermediateModel>& node_models);

    /**
     * @brief The method removes duplicate nodes from the list.
     * @param node_ids The list of components that need to remove Nodeid duplicates.
     * @return Returns an associative container of links to nodes in a filtered list for a faster search.
     */
    std::set<std::reference_wrapper<ExpandedNodeId>, std::less<ExpandedNodeId>> Distinct(std::vector<ExpandedNodeId>& node_ids); // NOLINT(modernize-use-transparent-functors)

    /**
     * @brief The method of checking the starting nodes in the lists for belonging to the basic space of names (ns=0) depending on the working mode.
     * In the case of the flat_list_of_nodes mode on and the determination of the starting unit of one list as i=85 - the check is always positive.
     * In another case, depending on the flag of work with custom ns=0, one of two checks occurs:
     * 1) If the mode of operation with custom nodes is included in the ns=0 space, then the belonging of such a node to the basic nodes of the OPC UA standard is checked.
     * Only custom nodes are allowed outside the standard otherwise error.
     * 2) If the work with custom nodes in the ns=0 space is not allowed, then an error is given when any such starting node is.
     * @return Verification status. StatusRessults :: good in case of successful check of starting nodes from the list and StatusRessults :: Fail in case of non -compliance with the conditions
     * described above.
     */
    [[nodiscard]] StatusResults CheckStartNodesOnNS0();

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
     * @param options Structure with additional parameters.
     */
    NodesetExporterLoop(std::map<std::string, std::vector<ExpandedNodeId>> node_ids, IOpen62541& open62541_lib, IEncoder& export_encoder, LoggerBase& logger, Options&& options)
        : m_node_ids(std::move(node_ids))
        , m_logger(logger)
        , m_open62541_lib(open62541_lib)
        , m_export_encoder(export_encoder)
        , m_external_options(std::move(options))
    {
        m_logger.Trace("Constructor called: NodesetExporterLoop()");

        // Create_Missing_start_node mode only works together with activated flat_list_of_nodes.
        if (m_external_options.flat_list_of_nodes.create_missing_start_node && !m_external_options.flat_list_of_nodes.is_enable)
        {
            throw std::runtime_error("The 'create_missing_start_node' parameter was enabled without 'flat_list_of_nodes'.");
        }

        // Allow_Abstract_variable mode only works with the activated Create_Missing_start_Node.
        if (m_external_options.flat_list_of_nodes.allow_abstract_variable && !m_external_options.flat_list_of_nodes.create_missing_start_node)
        {
            throw std::runtime_error("The 'allow_abstract_variable' parameter was enabled without 'create_missing_start_node'.");
        }

        // In flat mode, we work only with Object and Variable Node Class.
        if (m_external_options.flat_list_of_nodes.is_enable)
        {
            m_ignored_nodeclasses.insert(std::pair(UA_NODECLASS_OBJECTTYPE, "UA_NODECLASS_OBJECTTYPE"));
            m_ignored_nodeclasses.insert(std::pair(UA_NODECLASS_VARIABLETYPE, "UA_NODECLASS_VARIABLETYPE"));
            m_ignored_nodeclasses.insert(std::pair(UA_NODECLASS_REFERENCETYPE, "UA_NODECLASS_REFERENCETYPE"));
            m_ignored_nodeclasses.insert(std::pair(UA_NODECLASS_DATATYPE, "UA_NODECLASS_DATATYPE"));
        }
    }

    /**
     * @brief Sets the maximum number of nodes for which you need to get data from the server for one request.
     *        Also affects how much data on nodes will be stored in memory (except XML nodes).
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
    std::map<std::string, std::vector<ExpandedNodeId>> m_node_ids;
    LoggerBase& m_logger;
    IOpen62541& m_open62541_lib;
    IEncoder& m_export_encoder;
    Options m_external_options;

#pragma region Nodes from the namespace of the OPC UA standard

    const UA_NodeId m_ns0id_objectfolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    const UA_NodeId m_ns0id_hascomponent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    const UA_NodeId m_ns0id_hastypedefenition_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    const UA_NodeId m_ns0id_basevariabletype_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE);
    const UA_NodeId m_ns0id_hassubtype_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    const UATypesContainer<UA_ExpandedNodeId> m_ns0id_baseobjecttype_node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_TYPES_EXPANDEDNODEID);
    const UATypesContainer<UA_ExpandedNodeId>
        m_ns0id_basedatavariabletype_node_id = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), UA_TYPES_EXPANDEDNODEID);

#pragma endregion Nodes from the namespace of the OPC UA standard

    u_int32_t m_number_of_max_nodes_to_request_data = default_number_of_max_nodes_to_request_data;
    // A list of basic hierarchical types of links in the form of an associative container, consisting of "nodeid type of link: string name type of link".
    static const std::map<UATypesContainer<UA_NodeId>, std::string> m_hierarhical_references;
    // Список классов узлов представляющий ТИПЫ. Представляет собой ассоциативный контейнер из - "значение типа: строковое название типа".
    static const std::map<std::uint32_t, std::string> m_types_nodeclasses;
    // The list of nodes that refers to ns=0 and are the OPC UA standard.
    static const std::set<UATypesContainer<UA_ExpandedNodeId>> m_ns0_opcua_standard_node_ids;
    // A list of classes of components that should be ignored during processing.
    static std::map<UA_NodeClass, std::string> m_ignored_nodeclasses; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    // A list of ignored nodes according to ignored classes that should not be added to export unloading.
    std::set<UATypesContainer<UA_ExpandedNodeId>> m_ignored_node_ids_by_classes;
    // Copies of all nodeid in SET to quickly search for the desired node, for filter of link correction.
    // In the global version, it is especially needed when the processing of the nodes goes "packs"
    // if m_number_of_max_nodes_to_request_data > 0.
    // As-how such a list copies all the processing nodes, made through Reference_wrapper to store not objects of containers with nodeid,
    // and links to them (signs)
    std::set<std::reference_wrapper<UATypesContainer<UA_ExpandedNodeId>>, std::less<UATypesContainer<UA_ExpandedNodeId>>> m_node_ids_set_copy; // NOLINT

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
