//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_INTERFACES_IENCODER_H
#define NODESETEXPORTER_INTERFACES_IENCODER_H

#include "nodesetexporter/common/LoggerBase.h"
#include "nodesetexporter/common/Statuses.h"
#include "nodesetexporter/open62541/NodeIntermediateModel.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/types_generated_handling.h>

#include <optional>
#include <set>
#include <vector>

namespace nodesetexporter::interfaces
{

using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using NodeIntermediateModel = ::nodesetexporter::open62541::NodeIntermediateModel;
using StatusResults = ::nodesetexporter::common::statuses::StatusResults;
using ::nodesetexporter::open62541::UATypesContainer;

/**
 * @brief An abstract class of methods for interacting with export encoders.
 * @warning The error status is displayed as a result of each function (except setters or getters),
 *          but the error description must be logged in interface implementations.
 *          If it is possible to correct the error locally or it does not have a critical consequence, then the fact itself should be displayed in the log,
 *          but the method should continue to work and, in the absence of critical errors in other places, issue a success status.
 */
class IEncoder
{
public:
    /**
     * Building an exporter encoder where export is done to a file.
     * @param logger The logging class object.
     * @param filename The full path and name of the file.
     */
    IEncoder(LoggerBase& logger, std::string&& filename)
        : m_logger(logger)
        , m_filename(std::move(filename))
        , m_out_buffer(std::nullopt)
    {
    }

    /**
     * Building an exporter encoder where export is done to a dedicated buffer object.
     * @param logger The logging class object.
     * @param out_buffer The buffer into which the content will be generated. When specifying a buffer, the save to file method will be ignored.
     */
    IEncoder(LoggerBase& logger, std::iostream& out_buffer)
        : m_logger(logger)
        , m_out_buffer(out_buffer)
    {
    }
    virtual ~IEncoder() = default;
    IEncoder(IEncoder&) = delete;
    IEncoder(IEncoder&&) = delete;
    IEncoder& operator=(const IEncoder& obj) = delete;
    IEncoder& operator=(IEncoder&& obj) = delete;

    /**
     * @brief A method that executes a set of possible instructions before exporting begins.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults Begin() = 0;

    /**
     * @brief A method that executes a set of possible instructions at the end of the export.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults End() = 0;

    /**
     * @brief Method for adding node spaces to the export.
     * @param namespaces list of server nodespaces.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNamespaces(const std::vector<std::string>& namespaces) = 0;

    /**
     * @brief Method for adding aliases for node types participating in the export.
     * @param aliases unique NodeID objects that represent type aliases.
     *                Consists of the BrowseName attribute and the NodeID value.
     *                BrowseName is built on QualifiedName, but according to the XSD schema it is a regular string.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddAliases(const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases) = 0;

    /**
     * @brief Method for adding a node of type Object to the export.
     * @param node_model model of the required data for node export
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeObject(const NodeIntermediateModel& node_model) = 0;

    /**
     * @brief Method for adding an ObjectType node to the export.
     * @param node_model model of the required data for node export
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeObjectType(const NodeIntermediateModel& node_model) = 0;

    /**
     * @brief Method for adding a node of type Variable to the export.
     * @param node_model model of the required data for node export
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeVariable(const NodeIntermediateModel& node_model) = 0;

    /**
     * @brief Method for adding a VariableType node to the export.
     * @param node_model model of the required data for node export
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeVariableType(const NodeIntermediateModel& node_model) = 0;

    /**
     * @brief Method for adding a ReferenceType node to the export.
     * @param node_model model of the required data for node export.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeReferenceType(const NodeIntermediateModel& node_model) = 0;

    /**
     * @brief Method for adding a DataType node to the export.
     * @param node_model model of the required data for node export.
     * @return Return the error status.
     */
    [[nodiscard]] virtual StatusResults AddNodeDataType(const NodeIntermediateModel& node_model) = 0;

protected:
    LoggerBase& m_logger; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::string m_filename; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::optional<std::reference_wrapper<std::iostream>> m_out_buffer; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

} // namespace nodesetexporter::interfaces

#endif // NODESETEXPORTER_INTERFACES_IENCODER_H
