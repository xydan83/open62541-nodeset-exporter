//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/encoders/UAValueTypesToXMLText.h"
#include "nodesetexporter/common/DateTime.h"
#include "nodesetexporter/common/Strings.h"
#include "nodesetexporter/open62541/TypeAliases.h"
#include <map>

#include "fmt/format.h"

namespace nodesetexporter::encoders::uavaluetypestoxmltext
{
using nodesetexporter::common::DateTime;
#if defined(OPEN62541_VER_1_3)
using nodesetexporter::common::UA_String_isEmpty;
#endif
using nodesetexporter::common::UaStringToStdString;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::typealiases::ByteString;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;
using nodesetexporter::open62541::typealiases::MultidimensionalArrayAnchor;
using nodesetexporter::open62541::typealiases::StatusCode;

constexpr auto opc_ua_type_namespace = "uax";

#pragma region Types to xml

namespace
{
// Boolean
inline void AddBoleanXml(UA_Boolean var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Boolean", opc_ua_type_namespace).c_str())->SetText(var);
}
auto* AddListOfBoleanXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfBoolean", opc_ua_type_namespace).c_str());
}

// SByte
inline void AddSByteXml(UA_SByte var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:SByte", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfSByteXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfSByte", opc_ua_type_namespace).c_str());
}

// Byte
inline void AddByteXml(UA_Byte var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Byte", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfByteXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfByte", opc_ua_type_namespace).c_str());
}

// Int16
inline void AddInt16Xml(UA_Int16 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Int16", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfInt16Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfInt16", opc_ua_type_namespace).c_str());
}

// UInt16
inline void AddUInt16Xml(UA_UInt16 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:UInt16", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfUInt16Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfUInt16", opc_ua_type_namespace).c_str());
}

// Int32
inline void AddInt32Xml(UA_Int32 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Int32", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfInt32Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfInt32", opc_ua_type_namespace).c_str());
}

// UInt32
inline void AddUInt32Xml(UA_UInt32 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:UInt32", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfUInt32Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfUInt32", opc_ua_type_namespace).c_str());
}

// Int64
inline void AddInt64Xml(UA_Int64 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Int64", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfInt64Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfInt64", opc_ua_type_namespace).c_str());
}

// UInt64
inline void AddUInt64Xml(UA_UInt64 var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:UInt64", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfUInt64Xml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfUInt64", opc_ua_type_namespace).c_str());
}

// Float
inline void AddFloatXml(UA_Float var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Float", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfFloatXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfFloat", opc_ua_type_namespace).c_str());
}

// Double
inline void AddDoubleXml(UA_Double var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Double", opc_ua_type_namespace).c_str())->SetText(var);
}
inline auto* AddListOfDoubleXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfDouble", opc_ua_type_namespace).c_str());
}

// NodeClass
inline void AddNodeClassXml(UA_NodeClass var, XMLElement& xml_root_el)
{
    static std::map<UA_NodeClass, std::string> node_class{
        {UA_NodeClass::UA_NODECLASS_UNSPECIFIED, "Unspecified_0"},
        {UA_NodeClass::UA_NODECLASS_OBJECT, "Object_1"},
        {UA_NodeClass::UA_NODECLASS_VARIABLE, "Variable_2"},
        {UA_NodeClass::UA_NODECLASS_METHOD, "Method_4"},
        {UA_NodeClass::UA_NODECLASS_OBJECTTYPE, "ObjectType_8"},
        {UA_NodeClass::UA_NODECLASS_VARIABLETYPE, "VariableType_16"},
        {UA_NodeClass::UA_NODECLASS_REFERENCETYPE, "ReferenceType_32"},
        {UA_NodeClass::UA_NODECLASS_DATATYPE, "DataType_64"},
        {UA_NodeClass::UA_NODECLASS_VIEW, "View_128"}};
    xml_root_el.InsertNewChildElement(fmt::format("{}:NodeClass", opc_ua_type_namespace).c_str())->SetText(node_class.at(var).c_str());
}
inline auto* AddListOfNodeClassXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfNodeClass", opc_ua_type_namespace).c_str());
}

// StatusCode
inline void AddStatusCodeXml(UA_StatusCode status_code, XMLElement& xml_root_el, bool is_show_top_element_name = true)
{
    auto* loc_el = &xml_root_el;
    if (is_show_top_element_name)
    {
        loc_el = xml_root_el.InsertNewChildElement(fmt::format("{}:StatusCode", opc_ua_type_namespace).c_str());
    }
    loc_el->InsertNewChildElement(fmt::format("{}:Code", opc_ua_type_namespace).c_str())->SetText(status_code);
}
inline auto* AddListOfStatusCodeXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfStatusCode", opc_ua_type_namespace).c_str());
}

// ByteString
inline void AddByteStringXml(const UATypesContainer<ByteString>& var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:ByteString", opc_ua_type_namespace).c_str())->SetText(var.ToString().c_str());
}
inline auto* AddListOfByteStringXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfByteString", opc_ua_type_namespace).c_str());
}

// DateTime
inline void AddDateTimeXml(const UATypesContainer<UA_DateTime>& var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:DateTime", opc_ua_type_namespace).c_str())->SetText(DateTime::UADateTimeToString(var.GetRef()).c_str());
}
inline auto* AddListOfDateTimeXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfDateTime", opc_ua_type_namespace).c_str());
}

// Guid
inline void AddGuidXml(const UATypesContainer<UA_Guid>& var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:Guid", opc_ua_type_namespace).c_str())
        ->InsertNewChildElement(fmt::format("{}:String", opc_ua_type_namespace).c_str())
        ->SetText(var.ToString().c_str());
}
inline auto* AddListOfGuidXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfGuid", opc_ua_type_namespace).c_str());
}

// String
inline void AddStringXml(const UATypesContainer<UA_String>& var, XMLElement& xml_root_el)
{
    xml_root_el.InsertNewChildElement(fmt::format("{}:String", opc_ua_type_namespace).c_str())->SetText(var.ToString().c_str());
}
inline auto* AddListOfStringXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfString", opc_ua_type_namespace).c_str());
}

// NodeId
inline void AddNodeIdXml(const UATypesContainer<UA_NodeId>& var, XMLElement& xml_root_el)
{
    auto* node_el = xml_root_el.InsertNewChildElement(fmt::format("{}:NodeId", opc_ua_type_namespace).c_str());
    if (!UA_NodeId_isNull(&var.GetRef())) // May be absent
    {
        node_el->InsertNewChildElement(fmt::format("{}:Identifier", opc_ua_type_namespace).c_str())->SetText(var.ToString().c_str());
    }
}
inline auto* AddListOfNodeIdXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfNodeId", opc_ua_type_namespace).c_str());
}

// ExpandedNodeId
inline void AddExpandedNodeIdXml(const UATypesContainer<UA_ExpandedNodeId>& var, XMLElement& xml_root_el)
{
    auto* exp_node_el = xml_root_el.InsertNewChildElement(fmt::format("{}:ExpandedNodeId", opc_ua_type_namespace).c_str());
    if (!UA_NodeId_isNull(&var.GetRef().nodeId)) // May be absent
    {
        exp_node_el->InsertNewChildElement(fmt::format("{}:Identifier", opc_ua_type_namespace).c_str())->SetText(var.ToString().c_str());
    }
}
inline auto* AddListOfExpandedNodeIdXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfExpandedNodeId", opc_ua_type_namespace).c_str());
}

// QualifiedName
inline void AddQualifiedNameXml(const UATypesContainer<UA_QualifiedName>& var, XMLElement& xml_root_el)
{
    auto* qf_el = xml_root_el.InsertNewChildElement(fmt::format("{}:QualifiedName", opc_ua_type_namespace).c_str());
    if (var.GetRef().namespaceIndex != 0) // If zero, you can not display it
    {
        qf_el->InsertNewChildElement(fmt::format("{}:NamespaceIndex", opc_ua_type_namespace).c_str())->SetText(var.GetRef().namespaceIndex);
    }
    if (!UA_String_isEmpty(&var.GetRef().name)) // May be empty
    {
        qf_el->InsertNewChildElement(fmt::format("{}:Name", opc_ua_type_namespace).c_str())->SetText(UaStringToStdString(var.GetRef().name).c_str());
    }
}
inline auto* AddListOfQualifiedNameXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfQualifiedName", opc_ua_type_namespace).c_str());
}

// UA_LocalizedText
inline void AddLocalizedTextXml(const UATypesContainer<UA_LocalizedText>& var, XMLElement& xml_root_el)
{
    auto* loc_el = xml_root_el.InsertNewChildElement(fmt::format("{}:LocalizedText", opc_ua_type_namespace).c_str());
    if (!UA_String_isEmpty(&var.GetRef().locale)) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:Locale", opc_ua_type_namespace).c_str())->SetText(UaStringToStdString(var.GetRef().locale).c_str());
    }
    if (!UA_String_isEmpty(&var.GetRef().text)) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:Text", opc_ua_type_namespace).c_str())->SetText(UaStringToStdString(var.GetRef().text).c_str());
    }
}
inline auto* AddListOfLocalizedTextXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfLocalizedText", opc_ua_type_namespace).c_str());
}

// UA_DiagnosticInfo
inline void AddDiagnosticInfoXml(const UATypesContainer<UA_DiagnosticInfo>& var, XMLElement& xml_root_el, bool is_show_top_element_name = true) // NOLINT(misc-no-recursion)
{
    auto* loc_el = &xml_root_el;
    if (is_show_top_element_name)
    {
        loc_el = xml_root_el.InsertNewChildElement(fmt::format("{}:DiagnosticInfo", opc_ua_type_namespace).c_str());
    }
    if (var.GetRef().hasSymbolicId) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:SymbolicId", opc_ua_type_namespace).c_str())->SetText(var.GetRef().symbolicId);
    }
    if (var.GetRef().hasNamespaceUri) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:NamespaceUri", opc_ua_type_namespace).c_str())->SetText(var.GetRef().namespaceUri);
    }
    if (var.GetRef().hasLocale) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:Locale", opc_ua_type_namespace).c_str())->SetText(var.GetRef().locale);
    }
    if (var.GetRef().hasLocalizedText) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:LocalizedText", opc_ua_type_namespace).c_str())->SetText(var.GetRef().localizedText);
    }
    if (var.GetRef().hasAdditionalInfo) // May be empty
    {
        loc_el->InsertNewChildElement(fmt::format("{}:AdditionalInfo", opc_ua_type_namespace).c_str())->SetText(UaStringToStdString(var.GetRef().additionalInfo).c_str());
    }
    if (var.GetRef().hasInnerStatusCode) // May be empty
    {
        auto* status_code_el = loc_el->InsertNewChildElement(fmt::format("{}:InnerStatusCode", opc_ua_type_namespace).c_str());
        AddStatusCodeXml(var.GetRef().innerStatusCode, *status_code_el, false); // NOLINT
    }
    if (var.GetRef().hasInnerDiagnosticInfo) // May be empty
    {
        auto* inner_diag_info_el = loc_el->InsertNewChildElement(fmt::format("{}:InnerDiagnosticInfo", opc_ua_type_namespace).c_str());
        const auto inner_diag_info = UATypesContainer(*var.GetRef().innerDiagnosticInfo, UA_TYPES_DIAGNOSTICINFO);
        AddDiagnosticInfoXml(inner_diag_info, *inner_diag_info_el, false);
    }
}
inline auto* AddListOfDiagnosticInfoXml(XMLElement& xml_root_el)
{
    return xml_root_el.InsertNewChildElement(fmt::format("{}:ListOfDiagnosticInfo", opc_ua_type_namespace).c_str());
}
#pragma endregion Types to xml

/**
 * @brief Method for adding scalar data types to the XML tree.
 * @tparam T data type template
 * @param arg Data
 * @param xml_root_el XML element into which elements with value will be added.
 */
template <typename T>
void inline AddScalarTypes(const auto& arg, XMLElement& xml_root_el)
{
    if constexpr (std::is_same_v<T, UA_Boolean> || std::is_same_v<T, std::_Bit_reference>)
    {
        AddBoleanXml(static_cast<UA_Boolean>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_SByte>)
    {
        AddSByteXml(static_cast<UA_SByte>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Byte>)
    {
        AddByteXml(static_cast<UA_Byte>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Int16>)
    {
        AddInt16Xml(static_cast<UA_Int16>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_UInt16>)
    {
        AddUInt16Xml(static_cast<UA_UInt16>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Int32>)
    {
        AddInt32Xml(static_cast<UA_Int32>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_UInt32>)
    {
        AddUInt32Xml(static_cast<UA_UInt32>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Int64>)
    {
        AddInt64Xml(static_cast<UA_Int64>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_UInt64>)
    {
        AddUInt64Xml(static_cast<UA_UInt64>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Float>)
    {
        AddFloatXml(static_cast<UA_Float>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_Double>)
    {
        AddDoubleXml(static_cast<UA_Double>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UA_NodeClass>)
    {
        AddNodeClassXml(static_cast<UA_NodeClass>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, StatusCode>)
    {
        AddStatusCodeXml(static_cast<StatusCode>(arg).m_status_code, xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<ByteString>>)
    {
        AddByteStringXml(static_cast<UATypesContainer<ByteString>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_DateTime>>)
    {
        AddDateTimeXml(static_cast<UATypesContainer<UA_DateTime>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_Guid>>)
    {
        AddGuidXml(static_cast<UATypesContainer<UA_Guid>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_String>>)
    {
        AddStringXml(static_cast<UATypesContainer<UA_String>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_NodeId>>)
    {
        AddNodeIdXml(static_cast<UATypesContainer<UA_NodeId>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_ExpandedNodeId>>)
    {
        AddExpandedNodeIdXml(static_cast<UATypesContainer<UA_ExpandedNodeId>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_QualifiedName>>)
    {
        AddQualifiedNameXml(static_cast<UATypesContainer<UA_QualifiedName>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_LocalizedText>>)
    {
        AddLocalizedTextXml(static_cast<UATypesContainer<UA_LocalizedText>>(arg), xml_root_el);
    }
    else if constexpr (std::is_same_v<T, UATypesContainer<UA_DiagnosticInfo>>)
    {
        AddDiagnosticInfoXml(static_cast<UATypesContainer<UA_DiagnosticInfo>>(arg), xml_root_el);
    }
    else
    {
        throw std::runtime_error("Data type is not supported.");
    }
}

/**
 * @brief Method for adding an array of scalar data types to the XML tree.
 * @tparam T data type template
 * @param arg Data
 * @param xml_root_el XML element into which elements with value will be added.
 */
template <typename T>
void inline AddArrayTypes(const auto& arg, XMLElement& xml_root_el)
{
    // todo Since nodesetloader does not support multidimensional arrays, in the case of multidimensional arrays
    // we will not fill anything (without this condition, multidimensional arrays will not be filled correctly).
    const auto multidimension_array_check = [](const auto& array)
    {
        if (array.ArrayDimensionsLength() > 0 && array.ArrayLength() > 0)
        {
            throw std::runtime_error("Arrays with dimensions greater than one are not supported.");
        }
    };

    const auto array_processing_func = [](const auto& array, XMLElement& xml_element)
    {
        for (const auto& val : array.GetArray())
        {
            using TInternal = std::decay_t<decltype(val)>;
            AddScalarTypes<TInternal>(val, xml_element);
        }
    };

    if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Boolean>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Boolean>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfBoleanXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_SByte>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_SByte>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfSByteXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Byte>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Byte>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfByteXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Int16>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Int16>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfInt16Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_UInt16>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_UInt16>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfUInt16Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Int32>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Int32>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfInt32Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_UInt32>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_UInt32>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfUInt32Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Int64>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Int64>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfInt64Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_UInt64>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_UInt64>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfUInt64Xml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Float>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Float>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfFloatXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UA_Double>>)
    {
        const auto array = static_cast<MultidimensionalArray<UA_Double>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfDoubleXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<StatusCode>>)
    {
        const auto array = static_cast<MultidimensionalArray<StatusCode>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfStatusCodeXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<ByteString>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<ByteString>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfByteStringXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_DateTime>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_DateTime>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfDateTimeXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_Guid>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_Guid>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfGuidXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_String>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_String>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfStringXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_NodeId>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_NodeId>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfNodeIdXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfExpandedNodeIdXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_QualifiedName>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_QualifiedName>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfQualifiedNameXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_LocalizedText>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_LocalizedText>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfLocalizedTextXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else if constexpr (std::is_same_v<T, MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>>)
    {
        const auto array = static_cast<MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>>(arg);
        multidimension_array_check(array);
        auto* list_node = AddListOfDiagnosticInfoXml(xml_root_el);
        array_processing_func(array, *list_node);
    }
    else
    {
        throw std::runtime_error("Data type is not supported.");
    }
}
} // namespace

void AddValueToXml(const VariantsOfAttr& var, XMLElement& xml_root_el)
{
    std::visit(
        [&xml_root_el]<class T>(const T& arg) -> void
        {
            if constexpr (std::is_base_of_v<MultidimensionalArrayAnchor, T>)
            {
                // Array processing
                AddArrayTypes<T>(arg, xml_root_el);
            }
            else
            {
                // Processing scalar data
                AddScalarTypes<T>(arg, xml_root_el);
            }
        },
        var);
}
} // namespace nodesetexporter::encoders::uavaluetypestoxmltext