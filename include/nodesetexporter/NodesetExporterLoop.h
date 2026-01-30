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

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <variant>

namespace nodesetexporter
{

#pragma region Using_declarations_to_some_types

using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LogLevel = nodesetexporter::common::LogLevel;
using IEncoder = ::nodesetexporter::interfaces::IEncoder;
using IOpen62541 = ::nodesetexporter::interfaces::IOpen62541;
using NodeIntermediateModel = ::nodesetexporter::open62541::NodeIntermediateModel;
using StatusResults = nodesetexporter::common::statuses::StatusResults<>;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::typealiases::VariantsOfAttr;
using ExpandedNodeId = nodesetexporter::open62541::UATypesContainer<UA_ExpandedNodeId>;
using HasStartNodeSubtypeDetected = bool;
using StartNodeReverseReferenceCounter = uint64_t;

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
     * @param node_range The range of operation within the list of nodes node_ids and node_classes_req_res. Used for batch requests.
     * @param node_classes_req_res List of structures containing the node class.
     * @param nodes_attr_req_res [out] List of attributes bound to their NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeAttributes(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
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
#pragma endregion Получение ID атрибут

#pragma region Получение и обработка ссылок

    /**
     * @brief Prepare a query and get a list of links for each node.
     * @param node_ids List of NodeIds of nodes that participate in the export.
     * @param node_range The range of operation within the list of nodes node_ids and node_classes_req_res. Used for batch requests.
     * @param node_references_req_res List of references associated with NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeReferences(const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids, std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Method for adjusting links of type HasTypeDefinition. Forces direct directionality (in general, links have no direction
     * but the basic direction can be represented as a straight line by default) if the opposite direction is encountered, and also removes unnecessary links in case
     * if the principle of one link of this type per node is violated.
     * @param ref_desc_arr
     */
    void CorrectionUnnecessaryHasTypeDefinitionReferences(std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr);

    /**
     * @brief Checks for the presence of the first backlink found in the node and returns the result of the presence of such a link.
     * @param ref_desc_arr ref data description object
     * @return true - if at least one link is found, otherwise false.
     */
    [[nodiscard]] bool HasReverseReference(const std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr);

    /**
     * @brief For unknown reasons, KepServer sets Variable class nodes to HasTypeDefinition = BaseVariableType(62).
     * This abstract type cannot be used directly on nodes of this class. When importing nodesetloader we get
     * error. In this case, the easiest option is to change HasTypeDefinition to a more specific one, although still generic, but
     * non-abstract type BaseDataVariableType(63).
     * @param ref_desc_arr ref data description object
     * @return true - if HasTypeDefinition = BaseVariableType(62) was found and replaced with BaseDataVariableType(63) otherwise false.
     */
    [[maybe_unused]] bool ReplaceBaseVariableType(const UATypesContainer<UA_ExpandedNodeId>& node_id, std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr);

    /**
     * @brief Algorithm for adding backlinks from text node identifiers.
     * The algorithm does not use deep analysis to identify link types. All ReferenceTypes will be of type HasComponent.
     * There is also no solution for analyzing the namespace in case the parent and child may have different namespaces.
     * @param node_id The node in whose links backlinks are supposed to be added.
     * @param ref_desc_arr ref data description object
     * @return true if links have been added, false if the node id type is not string.
     */
    [[maybe_unused]] bool AddNodeReverseReference(const UATypesContainer<UA_ExpandedNodeId>& node_id, std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_desc_arr);

    /**
     * @brief Method for processing links for working with the KepServer server (and similar ones with similar features)
     * @param node_reference_req_res References bound to NodeID.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults KepServerRefFix(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res);

    /**
     * @brief Удалить ссылки на игнорируемые известные узлы.
     * @param index Индекс узла, связанного со ссылками.
     * @param node_references_req_res Список ссылок, связанных с NodeID.
     */
    void DeleteFailedReferences(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res);

    /**
     * @brief Remove all hierarchical links in a node.
     * @param node_references_req_res References bound to NodeID.
     */
    void DeleteAllHierarchicalReferences(IOpen62541::NodeReferencesRequestResponse& node_reference_req_res);

    /**
     * @brief Removing back references in nodes of classes of types ReferenceTypes, DataTypes, ObjectTypes, VariableTypes other than HasSubtype.
     * @param node_class The class of the node associated with the links.
     * @param node_references_req_res References bound to NodeID.
     */
    void DeleteNotHasSubtypeReference(UA_NodeClass node_class, IOpen62541::NodeReferencesRequestResponse& node_reference_req_res);

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
     * @param list_of_nodes_from_one_start_node An array of NodeIDs from one start node.
     * @param node_classes_req_res [out] An array of NodeClassesRequestResponse structures filled by the function containing references to NodeID and node classes.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodeClasses(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& list_of_nodes_from_one_start_node,
        std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res);

    /**
     * @brief Receive the necessary node data for further processing.
     * @param node_ids List of NodeIds of nodes that participate in the export.
     * @param node_classes_req_res List of structures containing the node class.
     * @param nodes_attr_req_res List of attributes bound to their NodeID.
     * @param node_references_req_res List of references associated with NodeID.
     * @return Request execution status.
     */
    StatusResults GetNodesDataPreparation(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
        const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
        std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief A method that combines various node filtering methods for access to further processing.
     * @param node_class The node class.
     * @param node_id The node to be checked by the filters.
     * @return The result of the filter. In case of Fail, the node is not suitable for further processing.
     */
    StatusResults GetNodesDataFiltering(UA_NodeClass node_class, const UATypesContainer<UA_ExpandedNodeId>& node_id);

    /**
     * @brief Method for updating links of the node being processed.
     * @param node_class The node class.
     * @param node_reference_req_res References bound to NodeID.
     * @return Processing status.
     */
    StatusResults GetNodesDataReferenceCorrection(UA_NodeClass node_class, IOpen62541::NodeReferencesRequestResponse& node_reference_req_res);

    /**
     * @brief Method for creating an artificial start node and its minimum required links.
     * @param index Index of the node associated with the links (with offset)
     * @param index_from_zero The index of the node associated with the links. (from scratch)
     * @param node_class The node class.
     * @param node_ids Start node ID.
     * @param nodes_attr_req_res List of attributes bound to their NodeID.
     * @param node_references_req_res List of references associated with NodeID.
     * @return Returns a tuple of two variables (START NODE ONLY). In the case of a non-start node, the return data should be ignored.
     * 1.HasStartNodeSubtypeDetected - If the starting node is the TYPES node class, it is marked as true. Otherwise false.
     * 2.StartNodeReverseReferenceCounter - counters whether a node has backlinks and their number.
     */
    auto GetNodesDataStartNodeCreation(
        size_t index,
        size_t index_from_zero,
        UA_NodeClass node_class,
        const UATypesContainer<UA_ExpandedNodeId>& node_ids,
        std::vector<IOpen62541::NodeAttributesRequestResponse>& nodes_attr_req_res,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief Method for getting the ID of the parent node.
     * @param index The index of the node from the list of export nodes whose parent node should be retrieved.
     * @param node_class The node class.
     * @param start_node_prop Tuple of two variables {HasStartNodeSubtypeDetected, StartNodeReverseReferenceCounter} (FOR START NODE ONLY).
     * see return from GetNodesDataStartNodeCreation method.
     * @param node_references_req_res List of references associated with NodeID.
     * @return A NodeID object with the primary parent node of the node specified by index.
     */
    auto GetNodesDataParentId(
        size_t index,
        UA_NodeClass node_class,
        const std::pair<HasStartNodeSubtypeDetected, StartNodeReverseReferenceCounter>& start_node_prop,
        std::vector<IOpen62541::NodeReferencesRequestResponse>& node_references_req_res);

    /**
     * @brief The main method for obtaining the necessary data to generate a list of intermediate structures
     * describing the main parameters of the node and its attributes.
     * @remark node_ids is a synchronizer for all arrays of structures based on NodeID as a sequence of elements.
     * The basic rule is that any request for nodes in a certain sequence and number of nodes must be equal to the number and the same sequence of responses.
     * This rule is used in the OPC UA standard when receiving a response from a server where the main parameter is a set of nodes.
     * For example: "The size and order of the list matches the size and order of the nodesToBrowsespecified in the request (Browsing)",
     * https://reference.opcfoundation.org/Core/Part4/v104/docs/5.8.2.2
     * or: "The size and order of this list matches the size and order of the nodesToReadrequest parameter (Read)".
     * https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2.2
     * @param node_ids A list of NodeIds of nodes that participate in the export, where the first parameter is the starting or root node of the list in text form, the second parameter is a list of
     * nodes including the start or root node in the form of an object that is the first node in the list.
     * @param node_classes_req_res List of structures containing the node class.
     * @param node_models [out] List of intermediate structures describing the main parameters of nodes and their attributes.
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults GetNodesData(
        const std::vector<UATypesContainer<UA_ExpandedNodeId>>& node_ids,
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

    /**
     * @brief Retrieve all other data in batches and export
     * @param list_of_nodes_from_one_start_node An array of NodeIDs from one start node.
     * @param node_range The range of batch data processing per function call.
     * @param node_classes_req_res An array of NodeClassesRequestResponse structures containing references to NodeID and node classes.
     * @param aliases Return associative array of text aliases relative to their NodeID.
     * @return The execution status of the method.
     */
    [[nodiscard]] StatusResults GetNodeDataAndExport(
        const std::vector<ExpandedNodeId>& list_of_nodes_from_one_start_node,
        const std::vector<IOpen62541::NodeClassesRequestResponse>& node_classes_req_res,
        std::map<std::string, UATypesContainer<UA_NodeId>>& aliases);

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
    ~NodesetExporterLoop() = default;
    NodesetExporterLoop(NodesetExporterLoop const&) noexcept = delete;
    NodesetExporterLoop& operator=(NodesetExporterLoop const&) = delete;
    NodesetExporterLoop(NodesetExporterLoop&&) noexcept = delete;
    NodesetExporterLoop& operator=(NodesetExporterLoop&&) = delete;

    /**
     * @brief Method to start a chain by exporting nodes of their accompanying data.
     * The export scheme is based on the description of the node structure of the 1.04 standard
     * https://files.opcfoundation.org/schemas/UA/1.04/UANodeSet.xsd
     * @return Request execution status.
     */
    [[nodiscard]] StatusResults StartExport();

private:
    std::map<std::string, std::vector<ExpandedNodeId>> m_node_ids;
    std::vector<UATypesContainer<UA_ReferenceDescription>> m_filtered_references_tmp; // Массив для фильтрации ссылок
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
    } m_exported_nodes = {.object_nodes = 0};
};

} // namespace nodesetexporter

#endif // NODESETEXPORTER_NODESETEXPORTERLOOP_H
