//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_ENCODERS_XMLENCODER_H
#define NODESETEXPORTER_ENCODERS_XMLENCODER_H

#include "encoders/UANodesetTypesToXMLText.h"
#include "encoders/UAValueTypesToXMLText.h"
#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/interfaces/IEncoder.h"

#include <tinyxml2.h>

#include <variant>

namespace nodesetexporter::encoders
{
namespace ua_to_text = uanodesettypestoxmltext;
using LogLevel = nodesetexporter::common::LogLevel;
using nodesetexporter::common::UaStringToStdString;
using nodesetexporter::interfaces::IEncoder;
using nodesetexporter::interfaces::LoggerBase;
using nodesetexporter::interfaces::NodeIntermediateModel;
using nodesetexporter::interfaces::StatusResults;
using nodesetexporter::interfaces::UATypesContainer;
using nodesetexporter::open62541::VariantsOfAttr;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using nodesetexporter::open62541::typealiases::UAVariantToStdVariant;
using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLError;
using tinyxml2::XMLPrinter;
using tinyxml2::XMLUtil;

/**
 * @brief Implementation class for the OPC UA node space structure encoder in XML.
 * The implementation is based on the OPC UA protocol standard version 1.04.
 * The XML information model description schema can be found here: https://reference.opcfoundation.org/Core/Part6/v104/docs/F
 * When writing the logic for filling in attributes and unloading elements in XML, their sequence and mandatory nature were taken from the scheme described above.
 */
class XMLEncoder final : public IEncoder
{
private:
    enum class ParentNodeId : uint8_t
    {
        Used,
        NotUsed
    };

    enum class Required : uint8_t
    {
        Required,
        NotRequired
    };

    /**
     * @brief A set of default values. Such values may not be output in XML.
     */
    class DefaultValueAttributes
    {
    public:
        /**
         * @brief Method for checking the default value of a particular attribute.
         * @param var The object being tested, stored in the std::variant container
         * @param attr Attribute associated with the check object
         * @warning If the attribute is of an incorrect type, a std::bad_variant_access exception may occur.
         * @return True - the value stored in the attr attribute var is the default value.
         *         False - if the value is not the default or the attribute being checked is missing.
         */
        static bool IsDefault(const VariantsOfAttr& var, UA_AttributeId attr);

    private:
        static constexpr UA_UInt32 write_mask = 0;
        static constexpr UA_UInt32 user_write_mask = 0;
        static constexpr UA_Byte event_notifier = 0;
        static constexpr UA_NodeId data_type{0, UA_NODEIDTYPE_NUMERIC, 24};
        static constexpr UA_Int32 value_rank = UA_VALUERANK_SCALAR;
        static constexpr UA_Byte access_level = UA_ACCESSLEVELMASK_READ;
        static constexpr UA_Byte user_access_level = UA_ACCESSLEVELMASK_READ;
        static constexpr UA_Double minimum_sampling_interval = 0;
        static constexpr UA_Boolean historizing = UA_FALSE;
        static constexpr size_t array_dimension = 0;
        static constexpr UA_Boolean symmetric = UA_FALSE;
        static constexpr UA_Boolean is_abstract = UA_FALSE;
    };

public:
    XMLEncoder(LoggerBase& logger, std::string filename)
        : IEncoder(logger, std::move(filename))
    {
    }

    XMLEncoder(LoggerBase& logger, std::iostream& out_buffer)
        : IEncoder(logger, out_buffer)
    {
    }
    ~XMLEncoder() override = default;
    XMLEncoder(XMLEncoder&) = delete;
    XMLEncoder(XMLEncoder&&) = delete;
    XMLEncoder& operator=(const XMLEncoder& obj) = delete;
    XMLEncoder& operator=(XMLEncoder&& obj) = delete;

    /**
     * @brief Method to initialize the XML tree assembly UANODESET. Adds a UANodeSet root node with static defining nodes that relate only to the underlying information model.
     * @warning Any initial unloading construct must begin with a global declaration - UANodeSet.
     * @return Function execution status.
     */
    StatusResults Begin() override;

    /**
     * @brief Method for dumping an XML tree into a file or specified buffer.
     *        After the call, all XML tree resources are released.
     * @return Function execution status.
     */
    StatusResults End() override;

    /**
     * @brief Метод добавления в XML дерево узла NamespaceUris.
     * @warning Метод вызывается только один раз до окончания построения дерева (до вызова End()).
     * @param namespaces Список пространств имен OPC UA.
     * @return Статус выполнения функции.
     */
    StatusResults AddNamespaces(const std::vector<std::string>& namespaces) override;

    /**
     * @brief Method for adding an Aliases node to the XML tree.
     * @warning The method is called only once before the tree is built (before End() is called).
     * @param aliases unique NodeID objects that represent type aliases.
     *        Consists of the BrowseName attribute and the NodeID value.
     *        BrowseName is built on QualifiedName, but according to the XSD schema it is a regular string.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddAliases(const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases) override;

    /**
     * @brief Method for adding a UAObject node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeObject(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Method for adding a UAObjectType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeObjectType(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Method for adding a UAVariable node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeVariable(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Method for adding a UAVariableType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeVariableType(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Method for adding a UAReferenceType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeReferenceType(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Method for adding a UADataType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeDataType(const NodeIntermediateModel& node_model) override;

    /**
     * @brief Remove the XML tree and other supporting resources.
     */
    void Reset();

private:
    /**
     * @brief Basic checks for main actions performed or internal variables populated
     * @param method_name The name of the method that will appear in the error in case of a validation error
     * @return false - check failed.
     */
    [[nodiscard]] bool BasicCheck(const std::string& method_name) const;

    /**
     * @brief Prints messages if the searched attribute has an empty value.
     * @param func_name The name of the function in which the attribute was requested.
     * @param attr_name The name of the attribute.
     * @param is_required Is the attribute required?
     */
    void MessageEmptyAttribute(const std::string& func_name, const std::string& node_id, const std::string& attr_name, Required is_required) const;

    /**
     * @brief Looks up the specified attribute in the NodeIntermediateModel's attribute/value container.
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @param attr_id The attribute to look for in the node model.
     * @param attr_name The text name of the attribute.
     * @param is_required Is the attribute required?
     * @return Returns std::optional<std::variant> with attribute values. If the attribute is missing or the attribute value is missing, std::nullopt is returned.
     */
    [[nodiscard]] std::optional<VariantsOfAttr> GetAndCheckUaAttribute(const NodeIntermediateModel& node_model, UA_AttributeId attr_id, const std::string& attr_name, Required is_required) const;

    /**
     * @brief Adds an object describing UAINstance (UANode + parentNodeId) to the XML tree. If the ParentNodeID output is not required, then the object describes the UANode.
     * @param xml_node An XML element that is based on a UAINstance or UANode (in case the ParentNodeId attribute is set to an empty object).
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @param is_parent_nodeid Sets whether the ParentNodeID attribute is output or not.
     * @return True - if successful, otherwise false.
     */
    [[nodiscard]] bool AddNodeUAInstance(XMLElement* xml_node, const NodeIntermediateModel& node_model, ParentNodeId is_parent_nodeid) const;

    /**
     * @brief Adds an object describing UAType ((UANode + IsAbstract) to the XML tree
     * @param xml_node An XML element that is based on UAType.
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @return True - if successful, otherwise false.
     */
    [[nodiscard]] bool AddNodeUAType(XMLElement* xml_node, const NodeIntermediateModel& node_model) const;

    // Методы добавления атрибут
    bool AddReqAttrNodeId(XMLElement* xml_node, const NodeIntermediateModel& node_model) const;
    bool AddReqAttrBrowseName(XMLElement* xml_node, const NodeIntermediateModel& node_model) const;
    void AddAttrDataType(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrValueRank(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrArrayDimensions(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrAccessLevel(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrUserAccessLevel(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrMinimumSamplingInterval(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrHistorizing(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    void AddAttrEventNotifier(const NodeIntermediateModel& node_model, XMLElement* xml_object);
    void AddAttrSymmetric(const NodeIntermediateModel& node_model, XMLElement* xml_reference_type);
    void AddAttrIsAbstract(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;
    void AddAttrWriteMask(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;
    void AddAttrUserWriteMask(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;

    // Методы добавления элементов
    StatusResults AddElementValue(const NodeIntermediateModel& node_model, XMLElement* xml_variable);
    StatusResults AddElementInverseName(const NodeIntermediateModel& node_model, XMLElement* xml_reference_type);
    bool AddElementDisplayName(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;
    bool AddElementDescription(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;

    // Метод добавления ссылок в узел
    bool AddNodeReferences(const NodeIntermediateModel& node_model, XMLElement* xml_node) const;

    // Метод добавления атрибута родителя.
    void AddNodeParent(const NodeIntermediateModel& node_model, const ParentNodeId& is_parent_nodeid, XMLElement* xml_node) const;

private:
    XMLDocument m_xml_tree; // Main XML tree
    XMLElement* m_xml_ua_nodeset = nullptr; // The main parent node of the structure within which the upload will be formed
    XMLElement* m_xml_ua_namespace_uris = nullptr; // Must always go first in the sequence inside m_ua_nodeset
    XMLElement* m_xml_ua_aliases = nullptr; // Must always come after m_ua_namespace_uris in sequence.

    static constexpr auto m_required_attr = "[Required]"; // Attributes that, according to the UANodeSet.xsd scheme, are marked as mandatory and do not have default values.
    static constexpr auto m_n_required_attr = "[Optional]";
    bool m_begin_first = false;
};

} // namespace nodesetexporter::encoders

#endif // NODESETEXPORTER_ENCODERS_XMLENCODER_H
