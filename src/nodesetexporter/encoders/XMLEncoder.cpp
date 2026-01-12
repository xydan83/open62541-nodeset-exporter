//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/encoders/XMLEncoder.h"


namespace nodesetexporter::encoders
{

bool XMLEncoder::DefaultValueAttributes::IsDefault(const VariantsOfAttr& var, UA_AttributeId attr)
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
        return std::get<MultidimensionalArray<UA_UInt32>>(var).ArrayLength() == array_dimension;
    case UA_AttributeId::UA_ATTRIBUTEID_SYMMETRIC:
        return std::get<UA_Boolean>(var) == symmetric;
    case UA_AttributeId::UA_ATTRIBUTEID_ISABSTRACT:
        return std::get<UA_Boolean>(var) == is_abstract;
    default:
        break;
    }
    return false;
}

StatusResults XMLEncoder::Begin()
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

    element->InsertNewComment("Definition elements are currently not supported in UADataType.");

    m_xml_ua_nodeset = element;
    m_begin_first = true;
    return StatusResults::Good;
}

StatusResults XMLEncoder::End()
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

StatusResults XMLEncoder::AddNamespaces(const std::vector<std::string>& namespaces)
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

StatusResults XMLEncoder::AddAliases(const std::map<std::string, UATypesContainer<UA_NodeId>>& aliases)
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

StatusResults XMLEncoder::AddNodeObject(const NodeIntermediateModel& node_model)
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

    // Fill in the basic attributes and elements inherent in each node inheriting from UAINstance
    if (!AddNodeUAInstance(xml_object, node_model, ParentNodeId::Used))
    {
        return StatusResults::Fail;
    }

    // XML ATTRIBUTES
    // Optional
    // EventNotifier
    AddAttrEventNotifier(node_model, xml_object);

    return StatusResults::Good;
}

StatusResults XMLEncoder::AddNodeObjectType(const NodeIntermediateModel& node_model)
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

    // Fill in the basic attributes and elements inherent in each node inheriting from UAType
    if (!AddNodeUAType(xml_object_type, node_model))
    {
        return StatusResults::Fail;
    }

    return StatusResults::Good;
}

StatusResults XMLEncoder::AddNodeVariable(const NodeIntermediateModel& node_model)
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

    // Fill in the basic attributes and elements inherent in each node inheriting from UAINstance
    if (!AddNodeUAInstance(xml_variable, node_model, ParentNodeId::Used))
    {
        return StatusResults::Fail;
    }

    // XML ATTRIBUTES
    // Optional
    // DataType
    AddAttrDataType(node_model, xml_variable);

    // ValueRank
    AddAttrValueRank(node_model, xml_variable);

    // ArrayDimensions
    AddAttrArrayDimensions(node_model, xml_variable);

    // AccessLevel
    AddAttrAccessLevel(node_model, xml_variable);

    // UserAccessLevel
    AddAttrUserAccessLevel(node_model, xml_variable);

    // MinimumSamplingInterval
    AddAttrMinimumSamplingInterval(node_model, xml_variable);

    // Historizing
    AddAttrHistorizing(node_model, xml_variable);

    // XML ELEMENTS
    // Optional
    if (AddElementValue(node_model, xml_variable) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    return StatusResults::Good;
}

StatusResults XMLEncoder::AddNodeVariableType(const NodeIntermediateModel& node_model)
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

    // Filling in the basic attributes and elements inherent to each node inheriting from UAType
    if (!AddNodeUAType(xml_variable_type, node_model))
    {
        return StatusResults::Fail;
    }

    // XML ATTRIBUTES
    // Optional
    // DataType
    AddAttrDataType(node_model, xml_variable_type);

    // ValueRank
    AddAttrValueRank(node_model, xml_variable_type);

    // ArrayDimensions
    AddAttrArrayDimensions(node_model, xml_variable_type);

    // XML ELEMENTS
    // Optional
    if (AddElementValue(node_model, xml_variable_type) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    return StatusResults::Good;
}

StatusResults XMLEncoder::AddNodeReferenceType(const NodeIntermediateModel& node_model)
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

    // Filling in the basic attributes and elements inherent to each node inheriting from UAType
    if (!AddNodeUAType(xml_reference_type, node_model))
    {
        return StatusResults::Fail;
    }

    // XML ATTRIBUTES
    // Optional
    // Symmetric
    AddAttrSymmetric(node_model, xml_reference_type);

    // XML ELEMENTS
    // Optional
    // InverseName
    if (AddElementInverseName(node_model, xml_reference_type) == StatusResults::Fail)
    {
        return StatusResults::Fail;
    }

    return StatusResults::Good;
}

StatusResults XMLEncoder::AddNodeDataType(const NodeIntermediateModel& node_model)
{
    m_logger.Trace("Method called: AddAttrDataType()");
    if (m_logger.IsEnable(LogLevel::Debug))
    {
        m_logger.Debug("XMLEncoder::AddAttrDataType(). {}", node_model.ToString());
    }

    if (!BasicCheck("AddAttrDataType()"))
    {
        return StatusResults::Fail;
    }

    auto* const xml_data_type = m_xml_ua_nodeset->InsertNewChildElement("UADataType");
    if (xml_data_type == nullptr)
    {
        m_logger.Error("XMLEncoder::AddAttrDataType(). Error setting UADataType.");
        return StatusResults::Fail;
    }

    // Filling in the basic attributes and elements inherent to each node inheriting from UAType
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
    //            m_logger.Error("XMLEncoder::AddAttrDataType(). Error setting Definition.");
    //            return StatusResults::Fail;
    //        }

    return StatusResults::Good;
}

void XMLEncoder::Reset()
{
    m_xml_tree.Clear();
    m_xml_tree.ClearError();
    m_xml_ua_nodeset = nullptr;
    m_xml_ua_namespace_uris = nullptr;
    m_xml_ua_aliases = nullptr;
}

bool XMLEncoder::BasicCheck(const std::string& method_name) const
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

void XMLEncoder::MessageEmptyAttribute(const std::string& func_name, const std::string& node_id, const std::string& attr_name, XMLEncoder::Required is_required) const
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

std::optional<VariantsOfAttr> XMLEncoder::GetAndCheckUaAttribute(const NodeIntermediateModel& node_model, UA_AttributeId attr_id, const std::string& attr_name, XMLEncoder::Required is_required) const
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

bool XMLEncoder::AddNodeUAInstance(XMLElement* xml_node, const NodeIntermediateModel& node_model, XMLEncoder::ParentNodeId is_parent_nodeid) const
{
    m_logger.Trace("Method called: AddNodeUAInstance()");

    // XML ATTRIBUTES
    // Required - if the parameter is missing, exit
    // NodeId
    if (!AddReqAttrNodeId(xml_node, node_model))
    {
        return false;
    }

    // BrowseName
    if (!AddReqAttrBrowseName(xml_node, node_model))
    {
        return false;
    }

    // Optional - в случае отсутствия параметра - не выводим параметр
    // WriteMask
    AddAttrWriteMask(node_model, xml_node);

    // UserWriteMask
    AddAttrUserWriteMask(node_model, xml_node);

    // ParentNodeId
    // Selected from links - not a UA attribute
    AddNodeParent(node_model, is_parent_nodeid, xml_node);

    // XML ELEMENTS
    // Optional - in case of absence parameter - not the sy if parameter
    // DisplayName
    if (!AddElementDisplayName(node_model, xml_node))
    {
        return false;
    }

    // Description
    if (!AddElementDescription(node_model, xml_node))
    {
        return false;
    }

    // References
    if (!AddNodeReferences(node_model, xml_node))
    {
        return false;
    }

    return true;
}

bool XMLEncoder::AddNodeUAType(XMLElement* xml_node, const NodeIntermediateModel& node_model) const
{
    m_logger.Trace("Method called: AddNodeUAType()");

    // Filling in the basic attributes and elements inherent to UANode
    if (!AddNodeUAInstance(xml_node, node_model, ParentNodeId::NotUsed))
    {
        return false;
    }

    // XML ATTRIBUTES
    // Optional
    // IsAbstract
    AddAttrIsAbstract(node_model, xml_node);

    return true;
}

bool XMLEncoder::AddReqAttrNodeId(XMLElement* xml_node, const NodeIntermediateModel& node_model) const
{
    const auto node_id = ua_to_text::UANodeIDToXMLString(node_model.GetExpNodeId());
    if (node_id.empty())
    {
        MessageEmptyAttribute("AddNodeUAInstance", node_model.GetExpNodeId().ToString(), "NodeId", Required::Required);
        return false;
    }
    xml_node->SetAttribute("NodeId", node_id.c_str());
    return true;
}

bool XMLEncoder::AddReqAttrBrowseName(XMLElement* xml_node, const NodeIntermediateModel& node_model) const
{
    const auto br_name_str = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_BROWSENAME, "BrowseName", Required::Required);
    if (!br_name_str.has_value())
    {
        return false;
    }
    xml_node->SetAttribute("BrowseName", ua_to_text::UAQualifiedNameToXMLString(br_name_str.value()).c_str());
    return true;
}

void XMLEncoder::AddAttrDataType(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrValueRank(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrArrayDimensions(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrAccessLevel(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrUserAccessLevel(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrMinimumSamplingInterval(const NodeIntermediateModel& node_model, XMLElement* xml_variable)
{
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
}

void XMLEncoder::AddAttrHistorizing(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
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
}

void XMLEncoder::AddAttrEventNotifier(const NodeIntermediateModel& node_model, XMLElement* const xml_object)
{
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
}

void XMLEncoder::AddAttrSymmetric(const NodeIntermediateModel& node_model, XMLElement* const xml_reference_type)
{
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
}

void XMLEncoder::AddAttrIsAbstract(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
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
}

void XMLEncoder::AddAttrWriteMask(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
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
}

void XMLEncoder::AddAttrUserWriteMask(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
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
}

StatusResults XMLEncoder::AddElementValue(const NodeIntermediateModel& node_model, XMLElement* const xml_variable)
{
    const auto value = GetAndCheckUaAttribute(node_model, UA_ATTRIBUTEID_VALUE, "Value", Required::NotRequired);
    if (value.has_value())
    {
        auto* const xml_value = xml_variable->InsertNewChildElement("Value");
        if (xml_value == nullptr)
        {
            m_logger.Error("XMLEncoder::AddNodeVariable(). Error setting Value.");
            return StatusResults::Fail;
        }
        try
        {
            uavaluetypestoxmltext::AddValueToXml(*value, *xml_value);
        }
        catch (std::runtime_error& exc)
        {
            m_logger.Warning("NodeID '{}' value data error: {}", node_model.GetExpNodeId().ToString(), exc.what());
        }
    }
    return StatusResults::Good;
}

StatusResults XMLEncoder::AddElementInverseName(const NodeIntermediateModel& node_model, XMLElement* const xml_reference_type)
{
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

bool XMLEncoder::AddElementDisplayName(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
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
    return true;
}

bool XMLEncoder::AddElementDescription(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
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
    return true;
}

bool XMLEncoder::AddNodeReferences(const NodeIntermediateModel& node_model, XMLElement* xml_node) const
{
    auto* const xml_references = xml_node->InsertNewChildElement("References");
    if (xml_references != nullptr)
    {
        const auto references = node_model.GetNodeReferenceTypeAliases();
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
            // XML ATTRIBUTES
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
            // XML ELEMENTS
            // Required
            if (UA_NodeId_isNull(&ref.first.GetRef().nodeId.nodeId))
            {
                m_logger.Error("XMLEncoder::AddNodeUAInstance. Reference NodeID is empty.");
                return false;
            }
            open62541::UATypesContainer<UA_String> node_id_txt(UA_TYPES_STRING);
            UA_ExpandedNodeId_print(&ref.first.GetRef().nodeId, &node_id_txt.GetRef());
            xml_reference->SetText(std::string{reinterpret_cast<char*>(node_id_txt.GetRef().data), node_id_txt.GetRef().length}.c_str()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
    }
    else
    {
        m_logger.Error("XMLEncoder::AddNodeUAInstance(). Error setting References.");
    }
    return true;
}

void XMLEncoder::AddNodeParent(const NodeIntermediateModel& node_model, const XMLEncoder::ParentNodeId& is_parent_nodeid, XMLElement* xml_node) const
{
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
}

} // namespace nodesetexporter::encoders