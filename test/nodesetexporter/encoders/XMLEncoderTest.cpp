//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/encoders/XMLEncoder.h"
#include "LogMacro.h"
#include "XmlHelperFunctions.h"

#include <open62541/types.h>

#include <doctest/doctest.h>
#include <tinyxml2.h> // Used to generate XML.

namespace
{
TEST_LOGGER_INIT

using LogLevel = nodesetexporter::common::LogLevel;
using XMLEncoder = ::nodesetexporter::encoders::XMLEncoder;
using StatusResults = nodesetexporter::common::statuses::StatusResults<>;
using NodeIntermediateModel = nodesetexporter::open62541::NodeIntermediateModel;
using ::nodesetexporter::open62541::UATypesContainer;
using ::nodesetexporter::open62541::VariantsOfAttr;
using tinyxml2::XMLDocument;

} // namespace


TEST_SUITE("nodesetexporter::encoders")
{

    TEST_CASE("Perform a self-test of the test environment")
    {
        std::string log_message;
        xmlpp::XsdValidator valid("UANodeSet.xsd"); // Schema for XML validation

        SUBCASE("Checking the operation of the scheme for validation (negative test)")
        {
            XMLDocument doc_valid_test;
            auto* decl_valid = doc_valid_test.NewDeclaration();
            doc_valid_test.InsertFirstChild(decl_valid);

            auto* nm_uri_valid = doc_valid_test.NewElement("NamespaceUris");
            auto* uri_valid = nm_uri_valid->InsertNewChildElement("Uri");
            uri_valid->SetText("http://devnetiot.com/opcua/");
            nm_uri_valid->InsertEndChild(uri_valid);
            doc_valid_test.InsertEndChild(nm_uri_valid);

            tinyxml2::XMLPrinter printer_valid;
            doc_valid_test.Print(&printer_valid);
            std::string valid_test_str(printer_valid.CStr(), printer_valid.CStrSize());
            // At some point the error "Line 44, column 1 (fatal): Extra content at the end of the document" appeared.
            // Swears at the presence of a line feed at the end.
            // The libxml2 library may have been updated in libxmlpp. I haven't found another solution yet.
            // Since it is used only for testing - due to its non-criticality - I will introduce a temporary solution.
            // Similar solutions will exist wherever a buffer is used. There are no such problems when checking files.
            valid_test_str.erase(valid_test_str.rfind('\n'));
            MESSAGE(valid_test_str);

            xmlpp::DomParser parser_valid;
            CHECK_NOTHROW(parser_valid.parse_memory(valid_test_str));
            CHECK_THROWS(valid.validate(parser_valid.get_document())); // Schematic Validation
        }

#pragma region Data Preparation

        XMLDocument doc;
        auto* decl = doc.NewDeclaration();
        doc.InsertFirstChild(decl);
        auto* element = doc.NewElement("UANodeSet");
        element->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"); // It's not an attribute - it's a namespace
        element->SetAttribute("xmlns:uax", "http://opcfoundation.org/UA/2008/02/Types.xsd"); // It's not an attribute - it's a namespace
        element->SetAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema"); // It's not an attribute - it's a namespace
        element->SetAttribute("xmlns", "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd"); // It's not an attribute - it's a namespace

        auto* nm_uri = element->InsertNewChildElement("NamespaceUris");
        auto* uri = nm_uri->InsertNewChildElement("Uri");
        uri->SetText("http://devnetiot.com/opcua/");
        auto* uri2 = nm_uri->InsertNewChildElement("Uri");
        uri2->SetText("http://devnetiot.com/opcua/2");

        auto* aliases = element->InsertNewChildElement("Aliases");
        auto* alias_int = aliases->InsertNewChildElement("Alias");
        alias_int->SetAttribute("Alias", "Int64");
        alias_int->SetText("i=8");
        auto* alias_double = aliases->InsertNewChildElement("Alias");
        alias_double->SetAttribute("Alias", "Double");
        alias_double->SetText("i=11");

        auto* ua_obj = element->InsertNewChildElement("UAObject");
        ua_obj->SetAttribute("NodeId", "ns=1;i=1");
        ua_obj->SetAttribute("BrowseName", "1:vPLC1");
        ua_obj->SetAttribute("ParentNodeId", "i=85");
        auto* ua_obj_dn = ua_obj->InsertNewChildElement("DisplayName");
        ua_obj_dn->SetText("vPLC1");
        auto* ua_obj_desc = ua_obj->InsertNewChildElement("Description");
        ua_obj_desc->SetText("Description vPLC1");

        doc.InsertEndChild(element);

        tinyxml2::XMLPrinter printer;
        doc.Print(&printer);

        // Test packaging in std::iostream& out_buffer
        std::stringstream in_test_buffer;
        in_test_buffer << std::string(printer.CStr(), printer.CStrSize());

#pragma endregion Data Preparation

        xmlpp::DomParser parser;
        std::string xpath;
        xmlpp::Attribute::NodeSet nodes_obj;

        SUBCASE("Checking for root node processing")
        {
            xpath = "//xmlns:UANodeSet"; // Node to be checked
            MESSAGE(in_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(nodes_obj = GetFindXMLNode(xpath, parser, valid, in_test_buffer));
            MESSAGE("Nodes size = ", nodes_obj.size());
            CHECK_EQ(nodes_obj.size(), 1);
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[0], "UANodeSet"));
            MESSAGE(log_message);
        }

        SUBCASE("Checking for Single Node Processing Without Attribute and Text")
        {
            xpath = "//xmlns:NamespaceUris"; // Node to be checked
            MESSAGE(in_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(nodes_obj = GetFindXMLNode(xpath, parser, valid, in_test_buffer));
            MESSAGE("Nodes size = ", nodes_obj.size());
            CHECK_NE(nodes_obj.size(), 0);
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[0], "NamespaceUris"));
            MESSAGE(log_message);
        }

        SUBCASE("Checking for Processing Multiple Nodes with Text")
        {
            xpath = "//xmlns:NamespaceUris/xmlns:Uri"; // Node to be checked
            MESSAGE(in_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(nodes_obj = GetFindXMLNode(xpath, parser, valid, in_test_buffer));
            MESSAGE("Nodes size = ", nodes_obj.size());
            CHECK_NE(nodes_obj.size(), 0);
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[0], "Uri", "http://devnetiot.com/opcua/"));
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[1], "Uri", "http://devnetiot.com/opcua/2"));
            MESSAGE(log_message);
        }

        SUBCASE("Checking for Processing Multiple Nodes with the Text Attribute")
        {
            xpath = "//xmlns:Aliases/xmlns:Alias"; // Node to be checked
            MESSAGE(in_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(nodes_obj = GetFindXMLNode(xpath, parser, valid, in_test_buffer));
            MESSAGE("Nodes size = ", nodes_obj.size());
            CHECK_NE(nodes_obj.size(), 0);
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[0], "Alias", "i=8", std::map<std::string, std::string>({{"Alias", "Int64"}})));
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[1], "Alias", "i=11", std::map<std::string, std::string>({{"Alias", "Double"}})));
            MESSAGE(log_message);
        }

        SUBCASE("Checking for processing a node with attributes and without text")
        {
            xpath = "//xmlns:UAObject"; // Node to be checked
            MESSAGE(in_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(nodes_obj = GetFindXMLNode(xpath, parser, valid, in_test_buffer));
            MESSAGE("Nodes size = ", nodes_obj.size());
            CHECK_NE(nodes_obj.size(), 0);
            CHECK_NOTHROW(CheckXMLNode(log_message, nodes_obj[0], "UAObject", "", std::map<std::string, std::string>({{"NodeId", "ns=1;i=1"}, {"BrowseName", "1:vPLC1"}, {"ParentNodeId", "i=85"}})));
            MESSAGE(log_message);
        }
    }

    TEST_CASE("nodesetexporter::encoders::XMLEncoder") // NOLINT(google-readability-function-size,readability-function-size)
    {
        std::string log_message;
        //  Be sure to use Begin and End in every test. Begin will wrap the function result to be checked in a global declaration (otherwise it won't go through the scheme),
        //  and End will ensure that it is unloaded into a buffer.
        xmlpp::DomParser parser;
        xmlpp::XsdValidator valid("UANodeSet.xsd");

        Logger logger("test");
        logger.SetLevel(LogLevel::Debug);
        std::stringstream out_test_buffer;

        XMLEncoder xmlEncoder(logger, out_test_buffer);
        std::string xpath;
        xmlpp::Attribute::NodeSet xml_nodes;

#pragma region Test data
        std::map<std::string, UATypesContainer<UA_NodeId>> aliases(
            {{"Int64", UATypesContainer<UA_NodeId>{UA_NODEID("i=8"), UA_TYPES_NODEID}},
             {"String", UATypesContainer<UA_NodeId>{UA_NODEID("i=12"), UA_TYPES_NODEID}},
             {"HasProperty", UATypesContainer<UA_NodeId>{UA_NODEID("i=46"), UA_TYPES_NODEID}},
             {"Argument", UATypesContainer<UA_NodeId>{UA_NODEID("i=296"), UA_TYPES_NODEID}},
             {"HasComponent", UATypesContainer<UA_NodeId>{UA_NODEID("i=47"), UA_TYPES_NODEID}},
             {"Organizes", UATypesContainer<UA_NodeId>{UA_NODEID("i=35"), UA_TYPES_NODEID}},
             {"HasSubtype", UATypesContainer<UA_NodeId>{UA_NODEID("i=45"), UA_TYPES_NODEID}},
             {"HasTypeDefinition", UATypesContainer<UA_NodeId>{UA_NODEID("i=40"), UA_TYPES_NODEID}}});

        UA_ReferenceDescription ref_desc_organize;
        ref_desc_organize.nodeId = UA_EXPANDEDNODEID("i=85");
        ref_desc_organize.displayName = UA_LOCALIZEDTEXT("", "Objects");
        ref_desc_organize.browseName = UA_QUALIFIEDNAME(0, "Objects");
        ref_desc_organize.typeDefinition = UA_EXPANDEDNODEID("i=61"); // FolderType
        ref_desc_organize.nodeClass = UA_NodeClass::UA_NODECLASS_OBJECT;
        ref_desc_organize.referenceTypeId = UA_NODEID("i=35"); // Organizes
        ref_desc_organize.isForward = false;

        UA_ReferenceDescription ref_desc_has_type_def;
        ref_desc_has_type_def.nodeId = UA_EXPANDEDNODEID("i=58");
        ref_desc_has_type_def.displayName = UA_LOCALIZEDTEXT("", "BaseObjectType");
        ref_desc_has_type_def.browseName = UA_QUALIFIEDNAME(0, "BaseObjectType");
        ref_desc_has_type_def.typeDefinition = UA_EXPANDEDNODEID_NULL;
        ref_desc_has_type_def.nodeClass = UA_NodeClass::UA_NODECLASS_OBJECT;
        ref_desc_has_type_def.referenceTypeId = UA_NODEID("i=40"); // HasTypeDefinition
        ref_desc_has_type_def.isForward = true;

        UA_ReferenceDescription ref_desc_has_component;
        ref_desc_has_component.nodeId = UA_EXPANDEDNODEID("ns=1;i=2");
        ref_desc_has_component.displayName = UA_LOCALIZEDTEXT("", "temperature");
        ref_desc_has_component.browseName = UA_QUALIFIEDNAME(1, "temperature");
        ref_desc_has_component.typeDefinition = UA_EXPANDEDNODEID("i=63"); // BaseDataVariableType
        ref_desc_has_component.nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE;
        ref_desc_has_component.referenceTypeId = UA_NODEID("i=47"); // HasComponent
        ref_desc_has_component.isForward = true;

        UA_ReferenceDescription ref_desc_has_component_parent = ref_desc_has_component;
        ref_desc_has_component_parent.isForward = false;

        UA_ReferenceDescription ref_desc_has_property;
        ref_desc_has_property.nodeId = UA_EXPANDEDNODEID("ns=1;i=11");
        ref_desc_has_property.displayName = UA_LOCALIZEDTEXT("", "MyProperty");
        ref_desc_has_property.browseName = UA_QUALIFIEDNAME(1, "MyProperty");
        ref_desc_has_property.typeDefinition = UA_EXPANDEDNODEID("i=68"); // PropertyType
        ref_desc_has_property.nodeClass = UA_NodeClass::UA_NODECLASS_VARIABLE;
        ref_desc_has_property.referenceTypeId = UA_NODEID("i=46"); // HasProperty
        ref_desc_has_property.isForward = true;

        UA_ReferenceDescription ref_desc_has_subtype;
        ref_desc_has_subtype.nodeId = UA_EXPANDEDNODEID("i=58");
        ref_desc_has_subtype.displayName = UA_LOCALIZEDTEXT("", "BaseObjectType");
        ref_desc_has_subtype.browseName = UA_QUALIFIEDNAME(0, "BaseObjectType");
        ref_desc_has_subtype.typeDefinition = UA_EXPANDEDNODEID_NULL;
        ref_desc_has_subtype.nodeClass = UA_NodeClass::UA_NODECLASS_OBJECTTYPE;
        ref_desc_has_subtype.referenceTypeId = UA_NODEID("i=45"); // HasSubtype
        ref_desc_has_subtype.isForward = false;

        /* UA_NODECLASS_OBJECT */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_object{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(1, "vPLC1"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("en", "vPLC1"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description vPLC1"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}},
            {UA_ATTRIBUTEID_EVENTNOTIFIER, std::optional<VariantsOfAttr>{static_cast<UA_Byte>(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT | UA_EVENTNOTIFIER_HISTORY_READ)}}};
        NodeIntermediateModel nim_object;
        nim_object.SetExpNodeId(UA_EXPANDEDNODEID("ns=1;i=1"));
        nim_object.SetNodeReferences({&ref_desc_organize, &ref_desc_has_type_def, &ref_desc_has_component, &ref_desc_has_property});
        nim_object.SetNodeClass(UA_NodeClass::UA_NODECLASS_OBJECT);
        nim_object.SetParentNodeId(UA_EXPANDEDNODEID("i=85"));
        nim_object.SetAttributes(attrs_object);

        /* UA_NODECLASS_OBJECTTYPE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_object_type{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(1, "FolderType"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "FolderType"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description Info Base Types"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_ISABSTRACT, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}}};
        NodeIntermediateModel nim_object_type;
        nim_object_type.SetExpNodeId(UA_EXPANDEDNODEID("ns=1;i=61"));
        nim_object_type.SetNodeReferences({&ref_desc_has_subtype});
        nim_object_type.SetNodeClass(UA_NodeClass::UA_NODECLASS_OBJECTTYPE);
        nim_object_type.SetAttributes(attrs_object_type);

        /* UA_NODECLASS_VARIABLE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_variable_scalar{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(1, "static_param1"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "static_param1"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description static_param1"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}},
            {UA_ATTRIBUTEID_DATATYPE, std::optional<VariantsOfAttr>{UATypesContainer<UA_NodeId>(UA_NODEID("i=8"), UA_TYPES_NODEID)}},
            {UA_ATTRIBUTEID_VALUERANK, std::optional<VariantsOfAttr>{static_cast<UA_Int32>(UA_VALUERANK_ANY)}},
            {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::optional<VariantsOfAttr>{std::nullopt}},
            {UA_ATTRIBUTEID_VALUE, std::optional<VariantsOfAttr>{std::nullopt}},
            {UA_ATTRIBUTEID_ACCESSLEVEL, std::optional<VariantsOfAttr>{static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE)}},
            {UA_ATTRIBUTEID_USERACCESSLEVEL, std::optional<VariantsOfAttr>{static_cast<UA_Byte>(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD)}},
            {UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, std::optional<VariantsOfAttr>{static_cast<UA_Double>(1000)}},
            {UA_ATTRIBUTEID_HISTORIZING, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}}};
        NodeIntermediateModel nim_variable_scalar;
        nim_variable_scalar.SetExpNodeId(UA_EXPANDEDNODEID("ns=1;i=18"));
        nim_variable_scalar.SetNodeReferences(std::vector<UA_ReferenceDescription*>{&ref_desc_has_component_parent, &ref_desc_has_type_def});
        nim_variable_scalar.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLE);
        nim_variable_scalar.SetParentNodeId(UA_EXPANDEDNODEID("ns=1;i=2"));
        nim_variable_scalar.SetAttributes(attrs_variable_scalar);

        /* UA_NODECLASS_VARIABLE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_variable_array{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(1, "EnumValues"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "EnumValues"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{std::nullopt}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(0)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(0)}},
            {UA_ATTRIBUTEID_DATATYPE, std::optional<VariantsOfAttr>{UATypesContainer<UA_NodeId>(UA_NODEID("i=7594"), UA_TYPES_NODEID)}},
            {UA_ATTRIBUTEID_VALUERANK, std::optional<VariantsOfAttr>{static_cast<UA_Int32>(UA_VALUERANK_TWO_DIMENSIONS)}},
            {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::optional<VariantsOfAttr>{std::vector<UA_UInt32>{5, 3}}},
            {UA_ATTRIBUTEID_VALUE, std::optional<VariantsOfAttr>{std::nullopt}},
            {UA_ATTRIBUTEID_ACCESSLEVEL, std::optional<VariantsOfAttr>{static_cast<UA_Byte>(1)}},
            {UA_ATTRIBUTEID_USERACCESSLEVEL, std::optional<VariantsOfAttr>{static_cast<UA_Byte>(1)}},
            {UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, std::optional<VariantsOfAttr>{static_cast<UA_Double>(0)}},
            {UA_ATTRIBUTEID_HISTORIZING, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(false)}}};
        NodeIntermediateModel nim_variable_array;
        nim_variable_array.SetExpNodeId(UA_EXPANDEDNODEID("ns=1;i=22"));
        nim_variable_array.SetNodeReferences({&ref_desc_has_component_parent, &ref_desc_has_type_def, &ref_desc_has_property});
        nim_variable_array.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLE);
        nim_variable_array.SetParentNodeId(UA_EXPANDEDNODEID("ns=1;i=2"));
        nim_variable_array.SetAttributes(attrs_variable_array);

        /* UA_NODECLASS_VARIABLETYPE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_variable_type{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(0, "BaseVariableType"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description BaseVariableType"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "BaseVariableType"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_DATATYPE, std::optional<VariantsOfAttr>{UATypesContainer<UA_NodeId>(UA_NODEID("i=45"), UA_TYPES_NODEID)}},
            {UA_ATTRIBUTEID_ISABSTRACT, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}},
            {UA_ATTRIBUTEID_ARRAYDIMENSIONS, std::optional<VariantsOfAttr>{std::vector<UA_UInt32>{2}}},
            {UA_ATTRIBUTEID_VALUERANK, std::optional<VariantsOfAttr>{static_cast<UA_Int32>(UA_VALUERANK_ONE_DIMENSION)}},
            {UA_ATTRIBUTEID_VALUE, std::optional<VariantsOfAttr>{std::nullopt}}};
        NodeIntermediateModel nim_variable_type;
        nim_variable_type.SetExpNodeId(UA_EXPANDEDNODEID("i=62"));
        nim_variable_type.SetNodeReferences({&ref_desc_has_subtype});
        nim_variable_type.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLETYPE);
        nim_variable_type.SetParentNodeId(UA_EXPANDEDNODEID("ns=1;i=2"));
        nim_variable_type.SetAttributes(attrs_variable_type);

        /* UA_NODECLASS_REFERENCETYPE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_reference_type{
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description References"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(0, "References"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "References"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_ISABSTRACT, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}},
            {UA_ATTRIBUTEID_INVERSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("en", "InverseHierarchicalReferences"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_SYMMETRIC, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}}};
        NodeIntermediateModel nim_reference_type;
        nim_reference_type.SetExpNodeId(UA_EXPANDEDNODEID("i=31"));
        nim_reference_type.SetNodeReferences({&ref_desc_has_subtype});
        nim_reference_type.SetNodeClass(UA_NodeClass::UA_NODECLASS_REFERENCETYPE);
        nim_reference_type.SetAttributes(attrs_reference_type);

        /* UA_NODECLASS_DATATYPE */
        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> attrs_data_type{
            {UA_ATTRIBUTEID_BROWSENAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_QualifiedName>(UA_QUALIFIEDNAME(0, "My Number"), UA_TYPES_QUALIFIEDNAME)}},
            {UA_ATTRIBUTEID_DESCRIPTION, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "Description My Number"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_DISPLAYNAME, std::optional<VariantsOfAttr>{UATypesContainer<UA_LocalizedText>(UA_LOCALIZEDTEXT("", "My Number"), UA_TYPES_LOCALIZEDTEXT)}},
            {UA_ATTRIBUTEID_USERWRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_BROWSENAME)}},
            {UA_ATTRIBUTEID_WRITEMASK, std::optional<VariantsOfAttr>{static_cast<UA_UInt32>(UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION)}},
            {UA_ATTRIBUTEID_ISABSTRACT, std::optional<VariantsOfAttr>{static_cast<UA_Boolean>(true)}},
            {UA_ATTRIBUTEID_DATATYPEDEFINITION, std::optional<VariantsOfAttr>{std::nullopt}}};
        NodeIntermediateModel nim_data_type;
        nim_data_type.SetExpNodeId(UA_EXPANDEDNODEID("ns=1;i=26"));
        nim_data_type.SetNodeReferences({&ref_desc_has_subtype});
        nim_data_type.SetNodeClass(UA_NodeClass::UA_NODECLASS_DATATYPE);
        nim_data_type.SetAttributes(attrs_data_type);

        std::vector<std::string> namespaces{"urn:EDDMappingDemo", "https://devnetiot.com/opcua/", "urn:Server1:OPCUA_TestDevice"};
#pragma endregion Test data

        // The sequence of attributes, as well as nested elements, will be determined by the schema in the padding methods.
        SUBCASE("Begin() - End()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            MESSAGE(out_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

            xpath = "//xmlns:UANodeSet"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser, valid, out_test_buffer));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1); // UANodeSet is only one
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UANodeSet"));
                MESSAGE(log_message);
            }
        }

        /*
         * Composition attribute:
         * Composition of elements: Uri
         */
        SUBCASE("AddNamespaces()")
        {
            SUBCASE("Single Output")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNamespaces(namespaces).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:NamespaceUris"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "NamespaceUris"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:NamespaceUris/xmlns:Uri"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), namespaces.size());
                if (!xml_nodes.empty())
                {
                    for (size_t index = 0; index < namespaces.size(); index++)
                    {
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Uri", namespaces[index]));
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                }
            }

            SUBCASE("Try to withdraw twice. The second one must be unsuccessful.")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNamespaces(namespaces).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.AddNamespaces(namespaces).GetStatus(), StatusResults::Fail); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                MESSAGE(out_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

                xpath = "//xmlns:NamespaceUris"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser, valid, out_test_buffer));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
            }
        }
        /*
         * Composition Attribute: Alias
         * Composition of elements: Alias
         */
        SUBCASE("AddAliases()")
        {
            SUBCASE("Single Output")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddAliases(aliases).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:Aliases"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:Aliases/xmlns:Alias"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), aliases.size());
                if (!xml_nodes.empty())
                {
                    size_t index = 0;
                    for (const auto& alias : aliases)
                    {
                        // Map sorts the key, the order will be stored in XML, and then it will also be transferred to std::vector when searching Based on this,
                        // we can use the index to synchronize the test and test items.
                        CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[index], "Alias", alias.second.ToString(), std::map<std::string, std::string>({{"Alias", alias.first}})));
                        index++;
                        MESSAGE(log_message);
                        log_message.clear();
                    }
                }
            }

            SUBCASE("Try to withdraw twice. The second one must be unsuccessful.")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddAliases(aliases).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.AddAliases(aliases).GetStatus(), StatusResults::Fail); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                MESSAGE(out_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

                xpath = "//xmlns:Aliases"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser, valid, out_test_buffer));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
                    MESSAGE(log_message);
                }
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, ParentNodeId, EventNotifier
         * Composition of elements: DisplayName, Description, References
         */
        SUBCASE("AddNodeObject()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.AddNodeObject(nim_object).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            std::string out_xml(out_test_buffer.str());
            out_xml.erase(out_xml.rfind('\n'));
            MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(parser.parse_memory(out_xml));
            CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

            xpath = "//xmlns:UAObject"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(
                    log_message,
                    xml_nodes[0],
                    "UAObject",
                    "",
                    std::map<std::string, std::string>(
                        {{"NodeId", "ns=1;i=1"}, {"BrowseName", "1:vPLC1"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"ParentNodeId", "i=85"}, {"EventNotifier", "5"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObject/xmlns:DisplayName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "vPLC1", std::map<std::string, std::string>({{"Locale", "en"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObject/xmlns:Description"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description vPLC1"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObject/xmlns:References"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObject/xmlns:References/xmlns:Reference"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), nim_object.GetNodeReferences().size());
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=85", std::map<std::string, std::string>({{"ReferenceType", "Organizes"}, {"IsForward", "false"}})));
                MESSAGE(log_message);
                log_message.clear();
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                MESSAGE(log_message);
                log_message.clear();
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=1;i=2", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}})));
                MESSAGE(log_message);
                log_message.clear();
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[3], "Reference", "ns=1;i=11", std::map<std::string, std::string>({{"ReferenceType", "HasProperty"}})));
                MESSAGE(log_message);
                log_message.clear();
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, IsAbstract
         * Composition of elements: DisplayName, Description, References,
         */
        SUBCASE("AddNodeObjectType()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.AddNodeObjectType(nim_object_type).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            std::string out_xml(out_test_buffer.str());
            out_xml.erase(out_xml.rfind('\n'));
            MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(parser.parse_memory(out_xml));
            CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

            xpath = "//xmlns:UAObjectType"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(
                    log_message,
                    xml_nodes[0],
                    "UAObjectType",

                    "",
                    std::map<std::string, std::string>({{"NodeId", "ns=1;i=61"}, {"BrowseName", "1:FolderType"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObjectType/xmlns:DisplayName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "FolderType"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObjectType/xmlns:Description"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description Info Base Types"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObjectType/xmlns:References"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAObjectType/xmlns:References/xmlns:Reference"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), nim_object_type.GetNodeReferences().size());
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasSubtype"}, {"IsForward", "false"}})));
                MESSAGE(log_message);
                log_message.clear();
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, ParentNodeId, DataType, ValueRank, ArrayDimensions, AccessLevel, UserAccessLevel, MinimumSamplingInterval, Historizing
         * Composition of elements: DisplayName, Description, References, Value
         */
        SUBCASE("AddNodeVariable()")
        {
            SUBCASE("Scalar")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_scalar).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:UAVariable"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=18"},
                             {"BrowseName", "1:static_param1"},
                             {"WriteMask", "96"},
                             {"UserWriteMask", "68"},
                             {"ParentNodeId", "ns=1;i=2"},
                             {"DataType", "Int64"},
                             {"ValueRank", "-2"},
                             {"AccessLevel", "3"},
                             {"UserAccessLevel", "7"},
                             {"MinimumSamplingInterval", "1000"},
                             {"Historizing", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:DisplayName"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "static_param1"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:Description"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description static_param1"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:References"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:References/xmlns:Reference"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), nim_variable_scalar.GetNodeReferences().size());
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "ns=1;i=2", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}, {"IsForward", "false"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }
            }

            SUBCASE("Array")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_array).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:UAVariable"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=22"}, {"BrowseName", "1:EnumValues"}, {"ParentNodeId", "ns=1;i=2"}, {"DataType", "EnumValueType"}, {"ValueRank", "2"}, {"ArrayDimensions", "5,3"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:DisplayName"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "EnumValues"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:References"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable/xmlns:References/xmlns:Reference"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), nim_variable_array.GetNodeReferences().size());
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "ns=1;i=2", std::map<std::string, std::string>({{"ReferenceType", "HasComponent"}, {"IsForward", "false"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[1], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasTypeDefinition"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[2], "Reference", "ns=1;i=11", std::map<std::string, std::string>({{"ReferenceType", "HasProperty"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, IsAbstract, DataType, ValueRank, ArrayDimensions.
         * Composition of elements: DisplayName, Description, References, Value
         */
        SUBCASE("AddNodeVariableType()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.AddNodeVariableType(nim_variable_type).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            std::string out_xml(out_test_buffer.str());
            out_xml.erase(out_xml.rfind('\n'));
            MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(parser.parse_memory(out_xml));
            CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

            xpath = "//xmlns:UAVariableType"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(
                    log_message,
                    xml_nodes[0],
                    "UAVariableType",

                    "",
                    std::map<std::string, std::string>(
                        {{"NodeId", "i=62"},
                         {"BrowseName", "BaseVariableType"},
                         {"WriteMask", "96"},
                         {"UserWriteMask", "68"},
                         {"IsAbstract", "true"},
                         {"DataType", "i=45"},
                         {"ValueRank", "1"},
                         {"ArrayDimensions", "2"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAVariableType/xmlns:DisplayName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "BaseVariableType"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAVariableType/xmlns:Description"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description BaseVariableType"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAVariableType/xmlns:References"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAVariableType/xmlns:References/xmlns:Reference"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), nim_variable_type.GetNodeReferences().size());
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasSubtype"}, {"IsForward", "false"}})));
                MESSAGE(log_message);
                log_message.clear();
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, IsAbstract, Symmetric
         * Composition of elements: DisplayName, Description, References, InverseName
         */
        SUBCASE("AddNodeReferenceType()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.AddNodeReferenceType(nim_reference_type).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            std::string out_xml(out_test_buffer.str());
            out_xml.erase(out_xml.rfind('\n'));
            MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(parser.parse_memory(out_xml));
            CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

            xpath = "//xmlns:UAReferenceType"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(
                    log_message,
                    xml_nodes[0],
                    "UAReferenceType",

                    "",
                    std::map<std::string, std::string>(
                        {{"NodeId", "i=31"}, {"BrowseName", "References"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}, {"Symmetric", "true"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAReferenceType/xmlns:DisplayName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAReferenceType/xmlns:Description"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAReferenceType/xmlns:References"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAReferenceType/xmlns:References/xmlns:Reference"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), nim_reference_type.GetNodeReferences().size());
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasSubtype"}, {"IsForward", "false"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UAReferenceType/xmlns:InverseName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "InverseName", "InverseHierarchicalReferences", std::map<std::string, std::string>({{"Locale", "en"}})));
                MESSAGE(log_message);
                log_message.clear();
            }
        }

        /*
         * Composition attribute: NodeId, BrowseName, WriteMask, UserWriteMask, IsAbstract
         * Composition of elements: DisplayName, Description, References, Definition(type DataTypeDefinition)
         *
         * DataTypeDefinition (attributes: Name, SymbolicName, IsUnion, IsOptionSet. Elements: Field (type DataTypeField))
         * DataTypeField (attributes: Name, SymbolicName, DataType, ValueRank,ArrayDimensions,MaxStringLength,Value, IsOptional, AllowSubTypes. Elements: DisplayName, Description)
         */
        // todo Finalize the DataTypeDefinition level
        SUBCASE("AddNodeDataType()")
        {
            CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
            CHECK_EQ(xmlEncoder.AddNodeDataType(nim_data_type).GetStatus(), StatusResults::Good); // MAIN TEST METHOD
            CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
            std::string out_xml(out_test_buffer.str());
            out_xml.erase(out_xml.rfind('\n'));
            MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

            CHECK_NOTHROW(parser.parse_memory(out_xml));
            CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

            xpath = "//xmlns:UADataType"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(
                    log_message,
                    xml_nodes[0],
                    "UADataType",

                    "",
                    std::map<std::string, std::string>({{"NodeId", "ns=1;i=26"}, {"BrowseName", "My Number"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UADataType/xmlns:DisplayName"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "DisplayName", "My Number"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UADataType/xmlns:Description"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Description", "Description My Number"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UADataType/xmlns:References"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), 1);
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "References"));
                MESSAGE(log_message);
                log_message.clear();
            }

            xpath = "//xmlns:UADataType/xmlns:References/xmlns:Reference"; // Node to be checked
            CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
            MESSAGE("Nodes size = ", xml_nodes.size());
            CHECK_EQ(xml_nodes.size(), nim_data_type.GetNodeReferences().size());
            if (!xml_nodes.empty())
            {
                CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Reference", "i=58", std::map<std::string, std::string>({{"ReferenceType", "HasSubtype"}, {"IsForward", "false"}})));
                MESSAGE(log_message);
                log_message.clear();
            }
        }

        SUBCASE("Combined Multiple Nodes")
        {
            SUBCASE("Sequential Addition")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNamespaces(namespaces).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddAliases(aliases).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeObject(nim_object).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeObjectType(nim_object_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_scalar).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_array).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariableType(nim_variable_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeReferenceType(nim_reference_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeDataType(nim_data_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:UANodeSet"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1); // UANodeSet is only one
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UANodeSet"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:NamespaceUris"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "NamespaceUris"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:Aliases"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAObject"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAObject",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=1"}, {"BrowseName", "1:vPLC1"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"ParentNodeId", "i=85"}, {"EventNotifier", "5"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAObjectType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAObjectType",
                        "",
                        std::map<std::string, std::string>({{"NodeId", "ns=1;i=61"}, {"BrowseName", "1:FolderType"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 2);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=18"},
                             {"BrowseName", "1:static_param1"},
                             {"WriteMask", "96"},
                             {"UserWriteMask", "68"},
                             {"ParentNodeId", "ns=1;i=2"},
                             {"DataType", "Int64"},
                             {"ValueRank", "-2"},
                             {"AccessLevel", "3"},
                             {"UserAccessLevel", "7"},
                             {"MinimumSamplingInterval", "1000"},
                             {"Historizing", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[1],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=22"}, {"BrowseName", "1:EnumValues"}, {"ParentNodeId", "ns=1;i=2"}, {"DataType", "EnumValueType"}, {"ValueRank", "2"}, {"ArrayDimensions", "5,3"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariableType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariableType",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "i=62"},
                             {"BrowseName", "BaseVariableType"},
                             {"WriteMask", "96"},
                             {"UserWriteMask", "68"},
                             {"IsAbstract", "true"},
                             {"DataType", "i=45"},
                             {"ValueRank", "1"},
                             {"ArrayDimensions", "2"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAReferenceType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAReferenceType",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "i=31"}, {"BrowseName", "References"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}, {"Symmetric", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UADataType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UADataType",
                        "",
                        std::map<std::string, std::string>({{"NodeId", "ns=1;i=26"}, {"BrowseName", "My Number"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }
            }

            SUBCASE("Basic sequence elements added at the end")
            {
                CHECK_EQ(xmlEncoder.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeDataType(nim_data_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeObjectType(nim_object_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_scalar).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeObject(nim_object).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariable(nim_variable_array).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeReferenceType(nim_reference_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddAliases(aliases).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNodeVariableType(nim_variable_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.AddNamespaces(namespaces).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder.End().GetStatus(), StatusResults::Good);
                std::string out_xml(out_test_buffer.str());
                out_xml.erase(out_xml.rfind('\n'));
                MESSAGE(out_xml); // Output of the generated xml as a result of the encoder functions.

                CHECK_NOTHROW(parser.parse_memory(out_xml));
                CHECK_NOTHROW(valid.validate(parser.get_document())); // Schematic Validation

                xpath = "//xmlns:UANodeSet"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1); // UANodeSet is only one
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "UANodeSet"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:NamespaceUris"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "NamespaceUris"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:Aliases"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(log_message, xml_nodes[0], "Aliases"));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAObject"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAObject",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=1"}, {"BrowseName", "1:vPLC1"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"ParentNodeId", "i=85"}, {"EventNotifier", "5"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAObjectType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAObjectType",
                        "",
                        std::map<std::string, std::string>({{"NodeId", "ns=1;i=61"}, {"BrowseName", "1:FolderType"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariable"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 2);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=18"},
                             {"BrowseName", "1:static_param1"},
                             {"WriteMask", "96"},
                             {"UserWriteMask", "68"},
                             {"ParentNodeId", "ns=1;i=2"},
                             {"DataType", "Int64"},
                             {"ValueRank", "-2"},
                             {"AccessLevel", "3"},
                             {"UserAccessLevel", "7"},
                             {"MinimumSamplingInterval", "1000"},
                             {"Historizing", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[1],
                        "UAVariable",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "ns=1;i=22"}, {"BrowseName", "1:EnumValues"}, {"ParentNodeId", "ns=1;i=2"}, {"DataType", "EnumValueType"}, {"ValueRank", "2"}, {"ArrayDimensions", "5,3"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAVariableType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAVariableType",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "i=62"},
                             {"BrowseName", "BaseVariableType"},
                             {"WriteMask", "96"},
                             {"UserWriteMask", "68"},
                             {"IsAbstract", "true"},
                             {"DataType", "i=45"},
                             {"ValueRank", "1"},
                             {"ArrayDimensions", "2"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UAReferenceType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UAReferenceType",
                        "",
                        std::map<std::string, std::string>(
                            {{"NodeId", "i=31"}, {"BrowseName", "References"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}, {"Symmetric", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }

                xpath = "//xmlns:UADataType"; // Node to be checked
                CHECK_NOTHROW(xml_nodes = GetFindXMLNode(xpath, parser));
                MESSAGE("Nodes size = ", xml_nodes.size());
                CHECK_EQ(xml_nodes.size(), 1);
                if (!xml_nodes.empty())
                {
                    CHECK_NOTHROW(CheckXMLNode(
                        log_message,
                        xml_nodes[0],
                        "UADataType",
                        "",
                        std::map<std::string, std::string>({{"NodeId", "ns=1;i=26"}, {"BrowseName", "My Number"}, {"WriteMask", "96"}, {"UserWriteMask", "68"}, {"IsAbstract", "true"}})));
                    MESSAGE(log_message);
                    log_message.clear();
                }
            }

            SUBCASE("The main elements of the sequence are added at the end with the output to the file ")
            {
                static constexpr auto filename = "nodeset_test.xml";
                XMLEncoder xmlEncoder_save_to_file(logger, filename);

                CHECK_EQ(xmlEncoder_save_to_file.Begin().GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeDataType(nim_data_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeObjectType(nim_object_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeVariable(nim_variable_scalar).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeObject(nim_object).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNamespaces(namespaces).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeReferenceType(nim_reference_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeVariableType(nim_variable_type).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddAliases(aliases).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.AddNodeVariable(nim_variable_array).GetStatus(), StatusResults::Good);
                CHECK_EQ(xmlEncoder_save_to_file.End().GetStatus(), StatusResults::Good);
                MESSAGE(out_test_buffer.str()); // Output of the generated xml as a result of the encoder functions.

                xmlpp::DomParser file_parser;
                file_parser.parse_file(filename);
                valid.validate(file_parser.get_document()); // Schematic Validation
                CHECK(file_parser);
                MESSAGE(file_parser.get_document()->write_to_string_formatted());
            }
        }
    }
}