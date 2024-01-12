//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

// todo Move implementation to .cpp

#ifndef NODESETEXPORTER_ENCODERS_XMLENCODER_H
#define NODESETEXPORTER_ENCODERS_XMLENCODER_H

#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/encoders/GetAttributeToXMLText.h"
#include "nodesetexporter/interfaces/IEncoder.h"

#include <tinyxml2.h>

#include <variant>

namespace nodesetexporter::encoders
{
namespace ua_to_text = getattributetoxmltext;
using LogLevel = nodesetexporter::common::LogLevel;
using nodesetexporter::common::UaStringToStdString;
using nodesetexporter::interfaces::IEncoder;
using nodesetexporter::interfaces::LoggerBase;
using nodesetexporter::interfaces::NodeIntermediateModel;
using nodesetexporter::interfaces::StatusResults;
using nodesetexporter::interfaces::UATypesContainer;
using nodesetexporter::open62541::VariantsOfAttr;
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
    enum class ParentNodeId
    {
        Used,
        NotUsed
    };

    enum class Required
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
        static bool IsDefault(const VariantsOfAttr& var, UA_AttributeId attr)
        {
            switch (attr)
            {
            case UA_AttributeId::UA_ATTRIBUTEID_WRITEMASK:
                return std::get<UA_UInt32>(var) == write_mask;
            case UA_AttributeId::UA_ATTRIBUTEID_USERWRITEMASK:
                return std::get<UA_UInt32>(var) == user_write_mask;
            case UA_AttributeId::UA_ATTRIBUTEID_EVENTNOTIFIER:
                return std::get<UA_Byte>(var) == event_notifier;
            case UA_AttributeId::UA_ATTRIBUTEID_DATATYPE:
            {
                auto pval = std::get<UATypesContainer<UA_NodeId>>(var);
                return UA_NodeId_equal(&pval.GetRef(), &data_type);
            }
            case UA_AttributeId::UA_ATTRIBUTEID_VALUERANK:
                return std::get<UA_Int32>(var) == value_rank;
            case UA_AttributeId::UA_ATTRIBUTEID_ACCESSLEVEL:
                return std::get<UA_Byte>(var) == access_level;
            case UA_AttributeId::UA_ATTRIBUTEID_USERACCESSLEVEL:
                return std::get<UA_Byte>(var) == user_access_level;
            case UA_AttributeId::UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
                return std::get<UA_Double>(var) == minimum_sampling_interval;
            case UA_AttributeId::UA_ATTRIBUTEID_HISTORIZING:
                return std::get<UA_Boolean>(var) == historizing;
            case UA_AttributeId::UA_ATTRIBUTEID_ARRAYDIMENSIONS:
                return std::get<std::vector<UA_UInt32>>(var).size() == array_dimension;
            case UA_AttributeId::UA_ATTRIBUTEID_SYMMETRIC:
                return std::get<UA_Boolean>(var) == symmetric;
            case UA_AttributeId::UA_ATTRIBUTEID_ISABSTRACT:
                return std::get<UA_Boolean>(var) == is_abstract;
            default:
                break;
            }
            return false;
        }

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
    StatusResults Begin() override
    {
        m_logger.Trace("Method called: Begin()");
        auto* decl = m_xml_tree.NewDeclaration();
        if (decl == nullptr)
        {
            m_logger.Error("XMLEncoder::Begin(). NewDeclaration fail.");
            return StatusResults::Fail;
        }

        if (m_xml_tree.InsertFirstChild(decl) == nullptr)
        {
            m_logger.Error("XMLEncoder::Begin(). XML declaration insert error.");
            return StatusResults::Fail;
        }

        auto* const element = m_xml_tree.NewElement("UANodeSet");
        if (element == nullptr)
        {
            m_logger.Error("XMLEncoder::Begin(). Error setting UANodeSet.");
            return StatusResults::Fail;
        }
        element->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
        element->SetAttribute("xmlns:uax", "http://opcfoundation.org/UA/2008/02/Types.xsd");
        element->SetAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
        element->SetAttribute("xmlns", "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd");

        if (m_xml_tree.InsertEndChild(element) == nullptr)
        {
            m_logger.Error("XMLEncoder::Begin(). UANodeSet insert error.");
            return StatusResults::Fail;
        }

        element->InsertNewComment("Value elements are currently not supported in Variable and VariableType nodes.");
        element->InsertNewComment("Definition elements are currently not supported in UADataType.");

        m_xml_ua_nodeset = element;
        m_begin_first = true;
        return StatusResults::Good;
    }

    /**
     * @brief Method for dumping an XML tree into a file or specified buffer.
     *        After the call, all XML tree resources are released.
     * @return Function execution status.
     */
    StatusResults End() override
    {
        m_logger.Trace("Method called: End()");
        if (!BasicCheck("End()"))
        {
            return StatusResults::Fail;
        }

        if (m_out_buffer.has_value())
        {
            XMLPrinter printer;
            m_xml_tree.Print(&printer);
            m_out_buffer.value().get() << std::string(printer.CStr(), printer.CStrSize());
        }
        else
        {
            auto save_result = m_xml_tree.SaveFile(m_filename.c_str());
            if (save_result != XMLError::XML_SUCCESS)
            {
                const auto* error_name = XMLDocument::ErrorIDToName(save_result);
                const auto* error_descr = m_xml_tree.ErrorStr();
                m_logger.Error("XMLEncoder::End(). Save to file error. {}: {}", error_name, error_descr);
                return StatusResults::Fail;
            }
        }

        m_begin_first = false;
        Reset();
        return StatusResults::Good;
    }


    /**
     * @brief Method for adding a NamespaceUris node to the XML tree.
     * @warning The method is called only once before the tree is built (before End() is called).
     * @param namespaces List of OPC UA namespaces.
     * @return Function execution status.
     */
    StatusResults AddNamespaces(const std::vector<std::string>& namespaces) override
    {
        m_logger.Trace("Method called: AddNamespaces()");
        if (m_xml_ua_namespace_uris != nullptr)
        {
            m_logger.Error("XMLEncoder::AddNamespaces(). The method has been called before. Call End() to zero out the execution of the method.");
            return StatusResults::Fail;
        }

        if (!BasicCheck("AddNamespaces()"))
        {
            return StatusResults::Fail;
        }

        auto* const nm_uri = m_xml_tree.NewElement("NamespaceUris");
        if (nm_uri == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNamespaces(). Error setting NamespaceUris.");
            return StatusResults::Fail;
        }
        for (const auto& ua_namespace : namespaces)
        {
            // XML ELEMENTS
            auto* uri = nm_uri->InsertNewChildElement("Uri");
            if (uri == nullptr)
            {
                m_logger.Error("XMLEncoder::AddNamespaces(). Uri: {} insert error.", ua_namespace);
                return StatusResults::Fail;
            }
            uri->SetText(ua_namespace.c_str()); // Judging by experiments with local variables, the text is copied in this method
        }

        if (m_xml_ua_nodeset->InsertFirstChild(nm_uri) == nullptr) // This element must always come first
        {
            m_logger.Error("XMLEncoder::AddNamespaces(). NamespaceUris insert error.");
            return StatusResults::Fail;
        }
        m_xml_ua_namespace_uris = nm_uri;
        return StatusResults::Good;
    }

    /**
     * @brief Method for adding an Aliases node to the XML tree.
     * @warning The method is called only once before the tree is built (before End() is called).
     * @param aliases unique NodeID objects that represent type aliases.
     *        Consists of the BrowseName attribute and the NodeID value.
     *        BrowseName is built on QualifiedName, but according to the XSD schema it is a regular string.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddAliases(const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases) override
    {
        m_logger.Trace("Method called: AddAliases()");
        if (m_xml_ua_aliases != nullptr)
        {
            m_logger.Error("XMLEncoder::AddAliases(). The method has been called before. Call End() to zero out the execution of the method.");
            return StatusResults::Fail;
        }

        if (!BasicCheck("AddAliases()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_alieses = m_xml_tree.NewElement("Aliases");
        if (xml_alieses == nullptr)
        {
            m_logger.Error("XMLEncoder::AddAliases(). Error setting Aliases.");
            return StatusResults::Fail;
        }
        for (const auto& alias : aliases)
        {
            // XML ELEMENTS
            auto* const xml_alias = xml_alieses->InsertNewChildElement("Alias");
            if (xml_alias == nullptr)
            {
                m_logger.Error("XMLEncoder::AddAliases(). Alias: {}:{} insert error.", alias.first, alias.second.ToString());
                return StatusResults::Fail;
            }
            xml_alias->SetText(alias.second.ToString().c_str());
            // XML ATTRIBUTES
            xml_alias->SetAttribute("Alias", alias.first.c_str());
        }

        if (m_xml_ua_namespace_uris != nullptr)
        {
            m_xml_ua_nodeset->InsertAfterChild(m_xml_ua_namespace_uris, xml_alieses);
        }
        else
        {
            m_xml_ua_nodeset->InsertFirstChild(xml_alieses);
        }
        m_xml_ua_aliases = xml_alieses;
        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UAObject node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeObject(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeObject()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeObject(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeObject()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_object = m_xml_ua_nodeset->InsertNewChildElement("UAObject");
        if (xml_object == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeObject(). Error setting UAObject.");
            return StatusResults::Fail;
        }

        // I fill in the basic attributes and elements inherent in each node inheriting from UAINstance
        if (!AddNodeUAInstance(xml_object, node_model, ParentNodeId::Used))
        {
            return StatusResults::Fail;
        }

        // XML ATTRIBUTES
        // Optional
        // EventNotifier
        const auto event_not = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_EVENTNOTIFIER, "EventNotifier", Required::NotRequired);
        if (event_not.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(event_not.value(), UA_ATTRIBUTEID_EVENTNOTIFIER))
                {
                    xml_object->SetAttribute("EventNotifier", ua_to_text::UAPrimitivesToXMLString(event_not.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming EventNotifier wrong data type. Exception message: {}", exc.what());
            }
        }

        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UAObjectType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeObjectType(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeObjectType()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeObjectType(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeObjectType()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_object_type = m_xml_ua_nodeset->InsertNewChildElement("UAObjectType");
        if (xml_object_type == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeObjectType(). Error setting UAObjectType.");
            return StatusResults::Fail;
        }

        // Filling the main attributes and elements inherent in each node inherited from UAType
        if (!AddNodeUAType(xml_object_type, node_model))
        {
            return StatusResults::Fail;
        }

        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UAVariable node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeVariable(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeVariable()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeVariable(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeVariable()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_variable = m_xml_ua_nodeset->InsertNewChildElement("UAVariable");
        if (xml_variable == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeVariable(). Error setting UAVariable.");
            return StatusResults::Fail;
        }

        // Filling the main attributes and elements inherent in each node inheriting from UAINstance
        if (!AddNodeUAInstance(xml_variable, node_model, ParentNodeId::Used))
        {
            return StatusResults::Fail;
        }

        // XML ATTRIBUTES
        // Optional
        // DataType
        const auto data_type = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_DATATYPE, "DataType", Required::NotRequired);
        if (data_type.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(data_type.value(), UA_ATTRIBUTEID_DATATYPE))
                {
                    xml_variable->SetAttribute("DataType", node_model.GetDataTypeAlias().c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming DataType wrong data type. Exception message: {}", exc.what());
            }
        }

        // ValueRank
        const auto value_rank = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_VALUERANK, "ValueRank", Required::NotRequired);
        if (value_rank.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(value_rank.value(), UA_ATTRIBUTEID_VALUERANK))
                {
                    xml_variable->SetAttribute("ValueRank", ua_to_text::UAPrimitivesToXMLString(value_rank.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming ValueRank wrong data type. Exception message: {}", exc.what());
            }
        }

        // ArrayDimensions
        const auto array_dimensions = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_ARRAYDIMENSIONS, "ArrayDimensions", Required::NotRequired);
        if (array_dimensions.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(array_dimensions.value(), UA_ATTRIBUTEID_ARRAYDIMENSIONS))
                {
                    xml_variable->SetAttribute("ArrayDimensions", ua_to_text::UAArrayDimensionToXMLString(array_dimensions.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming ArrayDimensions wrong data type. Exception message: {}", exc.what());
            }
        }

        // AccessLevel
        const auto access_level = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_ACCESSLEVEL, "AccessLevel", Required::NotRequired);
        if (access_level.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(access_level.value(), UA_ATTRIBUTEID_ACCESSLEVEL))
                {
                    xml_variable->SetAttribute("AccessLevel", ua_to_text::UAPrimitivesToXMLString(access_level.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming AccessLevel wrong data type. Exception message: {}", exc.what());
            }
        }

        // UserAccessLevel
        const auto user_access_level = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_USERACCESSLEVEL, "UserAccessLevel", Required::NotRequired);
        if (user_access_level.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(user_access_level.value(), UA_ATTRIBUTEID_USERACCESSLEVEL))
                {
                    xml_variable->SetAttribute("UserAccessLevel", ua_to_text::UAPrimitivesToXMLString(user_access_level.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming UserAccessLevel wrong data type. Exception message: {}", exc.what());
            }
        }

        // MinimumSamplingInterval
        const auto minimum_sampling_interval = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, "MinimumSamplingInterval", Required::NotRequired);
        if (minimum_sampling_interval.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(minimum_sampling_interval.value(), UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL))
                {
                    xml_variable->SetAttribute("MinimumSamplingInterval", ua_to_text::UAPrimitivesToXMLString(minimum_sampling_interval.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming MinimumSamplingInterval wrong data type. Exception message: {}", exc.what());
            }
        }

        // Historizing
        const auto historizing = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_HISTORIZING, "Historizing", Required::NotRequired);
        if (historizing.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(historizing.value(), UA_ATTRIBUTEID_HISTORIZING))
                {
                    xml_variable->SetAttribute("Historizing", ua_to_text::UAPrimitivesToXMLString(historizing.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming Historizing wrong data type. Exception message: {}", exc.what());
            }
        }

        // XML ELEMENTS
        // Optional
        // todo Value Complete the implementation.
        //        auto* const xml_value = xml_variable->InsertNewChildElement("Value");
        //        if (xml_value == nullptr)
        //        {
        //            m_logger.Error("XMLEncoder::AddNodeVariable(). Error setting Value.");
        //            return StatusResults::Fail;
        //        }

        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UAVariableType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeVariableType(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeVariableType()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeVariableType(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeVariableType()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_variable_type = m_xml_ua_nodeset->InsertNewChildElement("UAVariableType");
        if (xml_variable_type == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeVariableType(). Error setting UAVariableType.");
            return StatusResults::Fail;
        }

        // Filling the main attributes and elements inherent in each node inherited from UAType
        if (!AddNodeUAType(xml_variable_type, node_model))
        {
            return StatusResults::Fail;
        }
        // todo
        //  XML ATTRIBUTES
        //  Optional
        //  DataType
        const auto data_type = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_DATATYPE, "DataType", Required::NotRequired);
        if (data_type.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(data_type.value(), UA_ATTRIBUTEID_DATATYPE))
                {
                    xml_variable_type->SetAttribute("DataType", node_model.GetDataTypeAlias().c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming DataType wrong data type. Exception message: {}", exc.what());
            }
        }

        // ValueRank
        const auto value_rank = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_VALUERANK, "ValueRank", Required::NotRequired);
        if (value_rank.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(value_rank.value(), UA_ATTRIBUTEID_VALUERANK))
                {
                    xml_variable_type->SetAttribute("ValueRank", ua_to_text::UAPrimitivesToXMLString(value_rank.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming ValueRank wrong data type. Exception message: {}", exc.what());
            }
        }

        // ArrayDimensions
        const auto array_dimensions = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_ARRAYDIMENSIONS, "ArrayDimensions", Required::NotRequired);
        if (array_dimensions.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(array_dimensions.value(), UA_ATTRIBUTEID_ARRAYDIMENSIONS))
                {
                    xml_variable_type->SetAttribute("ArrayDimensions", ua_to_text::UAArrayDimensionToXMLString(array_dimensions.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming ArrayDimensions wrong data type. Exception message: {}", exc.what());
            }
        }

        // XML ELEMENTS
        // Optional
        // todo Value Complete the implementation.
        //        auto* const xml_value = xml_variable_type->InsertNewChildElement("Value");
        //        if (xml_value == nullptr)
        //        {
        //            m_logger.Error("XMLEncoder::AddNodeVariableType(). Error setting Value.");
        //            return StatusResults::Fail;
        //        }

        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UAReferenceType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeReferenceType(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeReferenceType()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeReferenceType(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeReferenceType()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_reference_type = m_xml_ua_nodeset->InsertNewChildElement("UAReferenceType");
        if (xml_reference_type == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeReferenceType(). Error setting UAReferenceType.");
            return StatusResults::Fail;
        }

        // Fill in the basic attributes and elements inherent in each node inheriting from UAType
        if (!AddNodeUAType(xml_reference_type, node_model))
        {
            return StatusResults::Fail;
        }

        // XML ATTRIBUTES
        // Optional
        // Symmetric
        const auto symmetric = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_SYMMETRIC, "Symmetric", Required::NotRequired);
        if (symmetric.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(symmetric.value(), UA_ATTRIBUTEID_SYMMETRIC))
                {
                    xml_reference_type->SetAttribute("Symmetric", ua_to_text::UAPrimitivesToXMLString(symmetric.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming Symmetric wrong data type. Exception message: {}", exc.what());
            }
        }

        // XML ELEMENTS
        // Optional
        // InverseName
        const auto inverse_name = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_INVERSENAME, "InverseName", Required::NotRequired);
        if (inverse_name.has_value())
        {
            auto* const xml_inverse_name = xml_reference_type->InsertNewChildElement("InverseName");
            if (xml_inverse_name == nullptr)
            {
                m_logger.Error("XMLEncoder::AddNodeReferenceType(). Error setting InverseName.");
                return StatusResults::Fail;
            }
            const auto inverse_name_struct = ua_to_text::UALocalizedTextToXMLString(inverse_name.value());
            if (!inverse_name_struct.locale.empty())
            {
                xml_inverse_name->SetAttribute("Locale", inverse_name_struct.locale.c_str());
            }

            xml_inverse_name->SetText(inverse_name_struct.text.c_str());
        }

        return StatusResults::Good;
    }

    /**
     * @brief Method for adding a UADataType node to the XML tree.
     * @param node_model An intermediate data model representing the necessary information to describe a node.
     * @return Function execution status.
     */
    [[nodiscard]] StatusResults AddNodeDataType(const NodeIntermediateModel& node_model) override
    {
        m_logger.Trace("Method called: AddNodeDataType()");
        if (m_logger.IsEnable(LogLevel::Debug))
        {
            m_logger.Debug("XMLEncoder::AddNodeDataType(). {}", node_model.ToString());
        }

        if (!BasicCheck("AddNodeDataType()"))
        {
            return StatusResults::Fail;
        }

        auto* const xml_data_type = m_xml_ua_nodeset->InsertNewChildElement("UADataType");
        if (xml_data_type == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeDataType(). Error setting UADataType.");
            return StatusResults::Fail;
        }

        // Fill in the basic attributes and elements inherent in each node inheriting from UAType
        if (!AddNodeUAType(xml_data_type, node_model))
        {
            return StatusResults::Fail;
        }

        // XML ATTRIBUTES
        // Optional
        // Purpose - The attribute is not supported by the Open62541 library. (commit 08a5334 in master)

        // XML ELEMENTS
        // Optional
        // todo Definition - Complete the implementation
        //        auto* const xml_definition = xml_data_type->InsertNewChildElement("Definition");
        //        if (xml_definition == nullptr)
        //        {
        //            m_logger.Error("XMLEncoder::AddNodeDataType(). Error setting Definition.");
        //            return StatusResults::Fail;
        //        }

        return StatusResults::Good;
    }

    /**
     * @brief Remove the XML tree and other supporting resources.
     */
    void Reset()
    {
        m_xml_tree.Clear();
        m_xml_tree.ClearError();
        m_xml_ua_nodeset = nullptr;
        m_xml_ua_namespace_uris = nullptr;
        m_xml_ua_aliases = nullptr;
    }

private:
    /**
     * @brief Basic checks for main actions performed or internal variables populated
     * @param method_name The name of the method that will appear in the error in case of a validation error
     * @return false - check failed.
     */
    [[nodiscard]] bool BasicCheck(const std::string& method_name) const
    {
        m_logger.Trace("Method called: BasicCheck()");
        if (!m_begin_first)
        {
            m_logger.Error("XMLEncoder::{}. Begin() didn't run.", method_name);
            return false;
        }
        if (m_xml_ua_nodeset == nullptr)
        {
            m_logger.Error("XMLEncoder::{}. UA_NodeSet doesn't exist.", method_name);
            return false;
        }
        return true;
    }

    /**
     * @brief Prints messages if the searched attribute has an empty value.
     * @param func_name The name of the function in which the attribute was requested.
     * @param attr_name The name of the attribute.
     * @param is_required Is the attribute required?
     */
    void MessageEmptyAttribute(const std::string& func_name, const std::string& node_id, const std::string& attr_name, Required is_required) const
    {
        m_logger.Trace("Method called: MessageEmptyAttribute()");
        if (is_required == Required::Required)
        {
            m_logger.Error("XMLEncoder::{}. NodeId: {}: {} {} has wrong type or the type is not supported or is empty.", func_name, node_id, attr_name, m_required_attr);
        }
        else
        {
            m_logger.Info("XMLEncoder::{}. NodeId: {}: {} {} has wrong type or the type is not supported or is empty.", func_name, node_id, attr_name, m_n_required_attr);
        }
    }

    /**
     * @brief Looks up the specified attribute in the NodeIntermediateModel's attribute/value container.
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @param attr_id The attribute to look for in the node model.
     * @param attr_name The text name of the attribute.
     * @param is_required Is the attribute required?
     * @return Returns std::optional<std::variant> with attribute values. If the attribute is missing or the attribute value is missing, std::nullopt is returned.
     */
    [[nodiscard]] std::optional<VariantsOfAttr> GetAndCheckUaAttribute(const NodeIntermediateModel& node_model, UA_AttributeId attr_id, const std::string& attr_name, Required is_required) const
    {
        m_logger.Trace("Method called: GetAndCheckUaAttribute()");
        try
        {
            auto br_name_opt = node_model.GetAttributes().at(attr_id);
            if (br_name_opt.has_value())
            {
                return br_name_opt;
            }
            MessageEmptyAttribute("GetAndCheckUaAttribute", node_model.GetExpNodeId().ToString(), attr_name, is_required);
        }
        catch (const std::out_of_range& exc)
        {
            m_logger.Error(
                "XMLEncoder::GetAndCheckUaAttribute. NodeID:{} has {} {} attribute not supported ",
                node_model.GetExpNodeId().ToString(),
                is_required == Required::Required ? m_required_attr : "",
                attr_name);
        }
        return std::nullopt;
    }

    /**
     * @brief Adds an object describing UAINstance (UANode + parentNodeId) to the XML tree. If the ParentNodeID output is not required, then the object describes the UANode.
     * @param xml_node An XML element that is based on a UAINstance or UANode (in case the ParentNodeId attribute is set to an empty object).
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @param is_parent_nodeid Sets whether the ParentNodeID attribute is output or not.
     * @return True - if successful, otherwise false.
     */
    [[nodiscard]] bool AddNodeUAInstance(XMLElement* const xml_node, const NodeIntermediateModel& node_model, ParentNodeId is_parent_nodeid) const
    {
        m_logger.Trace("Method called: AddNodeUAInstance()");

        // XML ATTRIBUTES
        // Required - if there is no parameter, exit
        // NodeId
        const auto node_id = ua_to_text::UANodeIDToXMLString(node_model.GetExpNodeId());
        if (node_id.empty())
        {
            MessageEmptyAttribute("AddNodeUAInstance", node_model.GetExpNodeId().ToString(), "NodeId", Required::Required);
            return false;
        }
        xml_node->SetAttribute("NodeId", node_id.c_str());

        // BrowseName
        const auto br_name_str = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_BROWSENAME, "BrowseName", Required::Required);
        if (!br_name_str.has_value())
        {
            return false;
        }
        xml_node->SetAttribute("BrowseName", ua_to_text::UAQualifiedNameToXMLString(br_name_str.value()).c_str());


        // Optional - if there is no parameter, we do not display the parameter
        // WriteMask
        const auto wr_mask = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_WRITEMASK, "WriteMask", Required::NotRequired);
        if (wr_mask.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(wr_mask.value(), UA_ATTRIBUTEID_WRITEMASK))
                {
                    xml_node->SetAttribute("WriteMask", ua_to_text::UAPrimitivesToXMLString(wr_mask.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming WriteMask wrong data type. Exception message: {}", exc.what());
            }
        }

        // UserWriteMask
        const auto user_wr_mask = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_USERWRITEMASK, "UserWriteMask", Required::NotRequired);
        if (user_wr_mask.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(user_wr_mask.value(), UA_ATTRIBUTEID_USERWRITEMASK))
                {
                    xml_node->SetAttribute("UserWriteMask", ua_to_text::UAPrimitivesToXMLString(user_wr_mask.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming UserWriteMask wrong data type. Exception message: {}", exc.what());
            }
        }

        // ParentNodeId
        // Selected from references - not a UA attribute
        if (is_parent_nodeid == ParentNodeId::Used)
        {
            const auto parent_node_id = ua_to_text::UANodeIDToXMLString(node_model.GetParentNodeId());
            if (!parent_node_id.empty())
            {
                xml_node->SetAttribute("ParentNodeId", parent_node_id.c_str());
            }
            else
            {
                m_logger.Warning("XMLEncoder::AddNodeUAInstance(). ParentNodeId is listed as in use but is empty.");
            }
        }

        // XML ELEMENTS
        // Optional - if there is no parameter, we do not display the parameter
        // DisplayName
        const auto disp_name = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_DISPLAYNAME, "DisplayName", Required::NotRequired);
        if (disp_name.has_value())
        {
            const auto disp_name_struct = ua_to_text::UALocalizedTextToXMLString(disp_name.value());
            if (!disp_name_struct.text.empty())
            {
                auto* const xml_disp_name = xml_node->InsertNewChildElement("DisplayName");
                if (xml_disp_name == nullptr)
                {
                    m_logger.Error("XMLEncoder::AddNodeUAInstance(). Error setting DisplayName.");
                    return false;
                }

                if (!disp_name_struct.locale.empty())
                {
                    xml_disp_name->SetAttribute("Locale", disp_name_struct.locale.c_str());
                }

                xml_disp_name->SetText(disp_name_struct.text.c_str());
            }
        }

        // Description
        const auto description = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_DESCRIPTION, "Description", Required::NotRequired);
        if (description.has_value())
        {
            const auto description_struct = ua_to_text::UALocalizedTextToXMLString(description.value());
            if (!description_struct.text.empty())
            {
                auto* const xml_description = xml_node->InsertNewChildElement("Description");
                if (xml_description == nullptr)
                {
                    m_logger.Error("XMLEncoder::AddNodeUAInstance(). Error setting Description.");
                    return false;
                }

                if (!description_struct.locale.empty())
                {
                    xml_description->SetAttribute("Locale", description_struct.locale.c_str());
                }

                xml_description->SetText(description_struct.text.c_str());
            }
        }

        // References
        auto* const xml_references = xml_node->InsertNewChildElement("References");
        if (xml_references != nullptr)
        {
            const auto references = const_cast<NodeIntermediateModel&>(node_model).GetNodeReferenceTypeAliases(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
            if (references.empty())
            {
                m_logger.Info("XMLEncoder::AddNodeUAInstance. References is empty.");
            }
            for (const auto& ref : references)
            {
                auto* const xml_reference = xml_references->InsertNewChildElement("Reference");
                if (xml_reference == nullptr)
                {
                    m_logger.Error("XMLEncoder::AddNodeUAInstance(). Error setting Reference.");
                    return false;
                }
                // XML Attributes
                // Required
                if (ref.second.empty())
                {
                    m_logger.Error("XMLEncoder::AddNodeUAInstance. ReferenceType is empty.");
                    return false;
                }
                xml_reference->SetAttribute("ReferenceType", ref.second.c_str());

                // Optional
                if (!ref.first.GetRef().isForward)
                {
                    xml_reference->SetAttribute("IsForward", "false");
                }
                // XML Elements
                // Required
                if (UA_NodeId_isNull(&ref.first.GetRef().nodeId.nodeId))
                {
                    m_logger.Error("XMLEncoder::AddNodeUAInstance. Reference NodeID is empty.");
                    return false;
                }
                UA_String node_id_txt = UA_STRING_NULL;
                UA_ExpandedNodeId_print(&ref.first.GetRef().nodeId, &node_id_txt);
                xml_reference->SetText(std::string{static_cast<char*>(static_cast<void*>(node_id_txt.data)), node_id_txt.length}.c_str());
            }
        }
        else
        {
            m_logger.Error("XMLEncoder::AddNodeUAInstance(). Error setting References.");
        }

        return true;
    }

    /**
     * @brief Adds an object describing UAType ((UANode + IsAbstract) to the XML tree
     * @param xml_node An XML element that is based on UAType.
     * @param node_model A node model object containing the necessary information for description in XML format.
     * @return True - if successful, otherwise false.
     */
    [[nodiscard]] bool AddNodeUAType(XMLElement* const xml_node, const NodeIntermediateModel& node_model) const
    {
        m_logger.Trace("Method called: AddNodeUAType()");

        // Fill in the basic attributes and elements inherent in UANode
        if (!AddNodeUAInstance(xml_node, node_model, ParentNodeId::NotUsed))
        {
            return false;
        }

        // XML ATTRIBUTES
        // Optional
        // IsAbstract
        const auto is_abstract = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_ISABSTRACT, "IsAbstract", Required::NotRequired);
        if (is_abstract.has_value())
        {
            try
            {
                if (!DefaultValueAttributes::IsDefault(is_abstract.value(), UA_ATTRIBUTEID_ISABSTRACT))
                {
                    xml_node->SetAttribute("IsAbstract", ua_to_text::UAPrimitivesToXMLString(is_abstract.value()).c_str());
                }
            }
            catch (std::bad_variant_access& exc)
            {
                m_logger.Warning("Detected incoming IsAbstract wrong data type. Exception message: {}", exc.what());
            }
        }

        return true;
    }

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
