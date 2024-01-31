//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/NodeIntermediateModel.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/nodeids.h>
#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include <doctest/doctest.h>

#include <string>

using nodesetexporter::open62541::NodeIntermediateModel;
using nodesetexporter::open62541::UATypesContainer;
using nodesetexporter::open62541::VariantsOfAttr;

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::NodeIntermediateModel") // NOLINT
    {
        // SET
        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt.GetRef().referenceTypeId, UA_String_fromChars("i=89")), UA_STATUSCODE_GOOD);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt.GetRef().nodeId.nodeId, UA_String_fromChars("ns=2;i=10")), UA_STATUSCODE_GOOD);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt.GetRef().typeDefinition.nodeId, UA_String_fromChars("i=26")), UA_STATUSCODE_GOOD);
        ref_desc_cnt.GetRef().isForward = false;
        ref_desc_cnt.GetRef().browseName.namespaceIndex = 1;
        ref_desc_cnt.GetRef().browseName.name = UA_String_fromChars("Test browse name");
        ref_desc_cnt.GetRef().nodeClass = UA_NodeClass::UA_NODECLASS_REFERENCETYPE;
        ref_desc_cnt.GetRef().displayName.locale = UA_String_fromChars("en");
        ref_desc_cnt.GetRef().displayName.text = UA_String_fromChars("Test display name");

        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt2(UA_TYPES_REFERENCEDESCRIPTION);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt2.GetRef().referenceTypeId, UA_String_fromChars("i=90")), UA_STATUSCODE_GOOD);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt2.GetRef().nodeId.nodeId, UA_String_fromChars("ns=2;i=11")), UA_STATUSCODE_GOOD);
        CHECK_EQ(UA_NodeId_parse(&ref_desc_cnt2.GetRef().typeDefinition.nodeId, UA_String_fromChars("i=46")), UA_STATUSCODE_GOOD);
        ref_desc_cnt2.GetRef().isForward = false;
        UA_QualifiedName br_name2{1, UA_String_fromChars("TestNodes2")};
        ref_desc_cnt2.GetRef().browseName = br_name2;
        ref_desc_cnt2.GetRef().nodeClass = UA_NodeClass::UA_NODECLASS_REFERENCETYPE;
        ref_desc_cnt2.GetRef().displayName.locale = UA_String_fromChars("ru");
        ref_desc_cnt2.GetRef().displayName.text = UA_String_fromChars("Какое то имя узла");

        std::vector<UATypesContainer<UA_ReferenceDescription>> list_of_ref_desc{ref_desc_cnt, ref_desc_cnt2};

        auto node_class = UA_NodeClass::UA_NODECLASS_VARIABLE;

        UATypesContainer<UA_ExpandedNodeId> parent_id(UA_TYPES_EXPANDEDNODEID);
        CHECK_EQ(UA_NodeId_parse(&parent_id.GetRef().nodeId, UA_String_fromChars("i=65")), UA_STATUSCODE_GOOD);

        UATypesContainer<UA_ExpandedNodeId> node_id(UA_TYPES_EXPANDEDNODEID);
        CHECK_EQ(UA_NodeId_parse(&node_id.GetRef().nodeId, UA_String_fromChars("ns=1;i=1")), UA_STATUSCODE_GOOD);

        UATypesContainer<UA_NodeId> data_type(UA_TYPES_NODEID);
        CHECK_EQ(UA_NodeId_parse(&data_type.GetRef(), UA_String_fromChars("ns=2;i=120")), UA_STATUSCODE_GOOD);
        auto data_type_v = std::make_optional<VariantsOfAttr>({data_type});

        std::map<UA_AttributeId, std::optional<VariantsOfAttr>> map_of_attr{
            {UA_AttributeId::UA_ATTRIBUTEID_DATATYPE, data_type_v}, {UA_AttributeId::UA_ATTRIBUTEID_NODECLASS, std::make_optional<VariantsOfAttr>({UA_NodeClass::UA_NODECLASS_VARIABLETYPE})}};

        /*RAW*/
        auto* node_id_raw = UA_ExpandedNodeId_new();
        auto* parent_id_raw = UA_ExpandedNodeId_new();
        UA_ExpandedNodeId_copy(&node_id.GetRef(), node_id_raw);
        UA_ExpandedNodeId_copy(&parent_id.GetRef(), parent_id_raw);

        auto* ref_desc_cnt_raw = UA_ReferenceDescription_new();
        auto* ref_desc_cnt_raw2 = UA_ReferenceDescription_new();
        UA_ReferenceDescription_copy(&ref_desc_cnt.GetRef(), ref_desc_cnt_raw);
        UA_ReferenceDescription_copy(&ref_desc_cnt2.GetRef(), ref_desc_cnt_raw2);
        std::vector<UA_ReferenceDescription*> list_of_ref_desc_raw{ref_desc_cnt_raw, ref_desc_cnt_raw2};
        /****/

        SUBCASE("Copying")
        {
            NodeIntermediateModel nim;
            nim.SetNodeClass(node_class);
            nim.SetNodeReferences(list_of_ref_desc);
            nim.SetParentNodeId(parent_id);
            nim.SetExpNodeId(node_id);
            nim.SetAttributes(map_of_attr);

            // GET and CHECK
            CHECK_EQ(nim.GetNodeClass(), node_class);
            CHECK(UA_ExpandedNodeId_equal(&nim.GetExpNodeId().GetRef(), &node_id.GetRef()));
            CHECK(UA_ExpandedNodeId_equal(&nim.GetParentNodeId().GetRef(), &parent_id.GetRef()));
            CHECK_EQ(nim.GetNodeReferences().size(), list_of_ref_desc.size());
            for (size_t index = 0; index < nim.GetNodeReferences().size(); index++)
            {
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.locale, &list_of_ref_desc[index].GetRef().displayName.locale));
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.text, &list_of_ref_desc[index].GetRef().displayName.text));
                CHECK(UA_QualifiedName_equal(&nim.GetNodeReferences()[index].GetRef().browseName, &list_of_ref_desc[index].GetRef().browseName));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().nodeId, &list_of_ref_desc[index].GetRef().nodeId));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().typeDefinition, &list_of_ref_desc[index].GetRef().typeDefinition));
                CHECK(UA_NodeId_equal(&nim.GetNodeReferences()[index].GetRef().referenceTypeId, &list_of_ref_desc[index].GetRef().referenceTypeId));
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().isForward, list_of_ref_desc[index].GetRef().isForward);
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().nodeClass, list_of_ref_desc[index].GetRef().nodeClass);
            }
            CHECK_EQ(nim.GetAttributes().size(), map_of_attr.size());
            CHECK(std::equal(
                map_of_attr.begin(),
                map_of_attr.end(),
                nim.GetAttributes().begin(),
                [](auto param1, auto param2)
                {
                    return param1.first == param2.first;
                }));

            // Print test
            std::string print_str;
            CHECK_NOTHROW(print_str = nim.ToString());
            MESSAGE(print_str);
        }

        SUBCASE("Copy-Initialize")
        {
            NodeIntermediateModel nim;
            nim.SetNodeClass(node_class);
            nim.SetNodeReferences(list_of_ref_desc_raw);
            nim.SetParentNodeId(*parent_id_raw);
            nim.SetExpNodeId(*node_id_raw);
            nim.SetAttributes(map_of_attr);

            // GET and CHECK
            CHECK_EQ(nim.GetNodeClass(), node_class);
            CHECK(UA_ExpandedNodeId_equal(&nim.GetExpNodeId().GetRef(), &node_id.GetRef()));
            CHECK(UA_ExpandedNodeId_equal(&nim.GetParentNodeId().GetRef(), &parent_id.GetRef()));
            CHECK_EQ(nim.GetNodeReferences().size(), list_of_ref_desc.size());
            for (size_t index = 0; index < nim.GetNodeReferences().size(); index++)
            {
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.locale, &list_of_ref_desc[index].GetRef().displayName.locale));
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.text, &list_of_ref_desc[index].GetRef().displayName.text));
                CHECK(UA_QualifiedName_equal(&nim.GetNodeReferences()[index].GetRef().browseName, &list_of_ref_desc[index].GetRef().browseName));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().nodeId, &list_of_ref_desc[index].GetRef().nodeId));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().typeDefinition, &list_of_ref_desc[index].GetRef().typeDefinition));
                CHECK(UA_NodeId_equal(&nim.GetNodeReferences()[index].GetRef().referenceTypeId, &list_of_ref_desc[index].GetRef().referenceTypeId));
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().isForward, list_of_ref_desc[index].GetRef().isForward);
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().nodeClass, list_of_ref_desc[index].GetRef().nodeClass);
            }
            CHECK_EQ(nim.GetAttributes().size(), map_of_attr.size());
            CHECK(std::equal(
                map_of_attr.begin(),
                map_of_attr.end(),
                nim.GetAttributes().begin(),
                [](auto param1, auto param2)
                {
                    return param1.first == param2.first;
                })); // SIGSEGV - tries to remove Variant... If GetAttributes() returns a copy.

            //        for (const auto& contain : map_of_attr)
            //        {
            //            const auto& value = nim.GetAttributes().extract(contain.first);
            //            CHECK_FALSE(value.empty());
            //        } // For where GetAttributes() returns a copy.

            // Print test
            std::string print_str;
            CHECK_NOTHROW(print_str = nim.ToString());
            MESSAGE(print_str);
        }

        SUBCASE("Moving")
        {
            NodeIntermediateModel nim;
            std::vector<UATypesContainer<UA_ReferenceDescription>> list_of_ref_desc_tst_cpy(list_of_ref_desc);
            UATypesContainer<UA_ExpandedNodeId> parent_id_tst_cpy(parent_id);
            UATypesContainer<UA_ExpandedNodeId> node_id_tst_cpy(node_id);
            std::map<UA_AttributeId, std::optional<VariantsOfAttr>> map_of_attr_tst_cpy(map_of_attr);

            nim.SetNodeClass(node_class);
            nim.SetNodeReferences(std::move(list_of_ref_desc));
            nim.SetParentNodeId(std::move(parent_id));
            nim.SetExpNodeId(std::move(node_id));
            nim.SetAttributes(std::move(map_of_attr));

            // GET and CHECK
            CHECK_EQ(nim.GetNodeClass(), node_class);
            CHECK(UA_ExpandedNodeId_equal(&nim.GetExpNodeId().GetRef(), &node_id_tst_cpy.GetRef()));
            CHECK(UA_ExpandedNodeId_equal(&nim.GetParentNodeId().GetRef(), &parent_id_tst_cpy.GetRef()));
            CHECK_EQ(nim.GetNodeReferences().size(), list_of_ref_desc_tst_cpy.size());
            for (size_t index = 0; index < nim.GetNodeReferences().size(); index++)
            {
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.locale, &list_of_ref_desc_tst_cpy[index].GetRef().displayName.locale));
                CHECK(UA_String_equal(&nim.GetNodeReferences()[index].GetRef().displayName.text, &list_of_ref_desc_tst_cpy[index].GetRef().displayName.text));
                CHECK(UA_QualifiedName_equal(&nim.GetNodeReferences()[index].GetRef().browseName, &list_of_ref_desc_tst_cpy[index].GetRef().browseName));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().nodeId, &list_of_ref_desc_tst_cpy[index].GetRef().nodeId));
                CHECK(UA_ExpandedNodeId_equal(&nim.GetNodeReferences()[index].GetRef().typeDefinition, &list_of_ref_desc_tst_cpy[index].GetRef().typeDefinition));
                CHECK(UA_NodeId_equal(&nim.GetNodeReferences()[index].GetRef().referenceTypeId, &list_of_ref_desc_tst_cpy[index].GetRef().referenceTypeId));
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().isForward, list_of_ref_desc_tst_cpy[index].GetRef().isForward);
                CHECK_EQ(nim.GetNodeReferences()[index].GetRef().nodeClass, list_of_ref_desc_tst_cpy[index].GetRef().nodeClass);
            }
            CHECK_EQ(nim.GetAttributes().size(), map_of_attr_tst_cpy.size());
            CHECK(std::equal(
                map_of_attr_tst_cpy.begin(),
                map_of_attr_tst_cpy.end(),
                nim.GetAttributes().begin(),
                [](auto param1, auto param2)
                {
                    return param1.first == param2.first;
                }));

            // Print test
            std::string print_str;
            CHECK_NOTHROW(print_str = nim.ToString());
            MESSAGE(print_str);
        }

        SUBCASE("Getting an alias of the type of object stored in a node - GetNodeReferenceTypeAliases")
        {
#pragma region Test data
            std::map<std::string, UATypesContainer<UA_NodeId>> nodeid_aliases{
                {"Boolean", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_BOOLEAN), UA_TYPES_NODEID)},
                {"SByte", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_SBYTE), UA_TYPES_NODEID)},
                {"Byte", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_BYTE), UA_TYPES_NODEID)},
                {"Int16", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_INT16), UA_TYPES_NODEID)},
                {"UInt16", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_UINT16), UA_TYPES_NODEID)},
                {"Int32", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_INT32), UA_TYPES_NODEID)},
                {"UInt32", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_UINT32), UA_TYPES_NODEID)},
                {"Int64", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_INT64), UA_TYPES_NODEID)},
                {"UInt64", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_UINT64), UA_TYPES_NODEID)},
                {"Float", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_FLOAT), UA_TYPES_NODEID)},
                {"Double", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_DOUBLE), UA_TYPES_NODEID)},
                {"DateTime", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_DATETIME), UA_TYPES_NODEID)},
                {"String", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_STRING), UA_TYPES_NODEID)},
                {"ByteString", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_BYTESTRING), UA_TYPES_NODEID)},
                {"Guid", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_GUID), UA_TYPES_NODEID)},
                {"XmlElement", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_XMLELEMENT), UA_TYPES_NODEID)},
                {"NodeId", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_NODEID), UA_TYPES_NODEID)},
                {"ExpandedNodeId", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_EXPANDEDNODEID), UA_TYPES_NODEID)},
                {"QualifiedName", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_QUALIFIEDNAME), UA_TYPES_NODEID)},
                {"LocalizedText", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_LOCALIZEDTEXT), UA_TYPES_NODEID)},
                {"StatusCode", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_STATUSCODE), UA_TYPES_NODEID)},
                {"Structure", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE), UA_TYPES_NODEID)},
                {"DataValue", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_DATAVALUE), UA_TYPES_NODEID)},
                {"BaseDataType", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), UA_TYPES_NODEID)},
                {"DiagnosticInfo", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_DIAGNOSTICINFO), UA_TYPES_NODEID)},
                {"Number", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_NUMBER), UA_TYPES_NODEID)},
                {"Integer", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_INTEGER), UA_TYPES_NODEID)},
                {"UInteger", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_UINTEGER), UA_TYPES_NODEID)},
                {"Enumeration", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_ENUMERATION), UA_TYPES_NODEID)},
                {"Image", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_IMAGE), UA_TYPES_NODEID)},
                {"ns=1;i=32", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(1, 32), UA_TYPES_NODEID)}, // Custom
                {"ns=1;i=144", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(1, 144), UA_TYPES_NODEID)}, // Custom
                {"ns=1;i=4", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(1, 4), UA_TYPES_NODEID)}, // Custom
                {"ns=1;s=SomeRefType", UATypesContainer<UA_NodeId>(UA_NODEID_STRING(1, "SomeRefType"), UA_TYPES_NODEID)}, // Custom
                {"ns=2;i=983", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(2, 983), UA_TYPES_NODEID)}}; // Custom
#pragma endregion Test data

            SUBCASE("All standard types should be returned with aliases, and custom types should be returned with a text NodeId")
            {
                for (const auto& nodeid_aliase : nodeid_aliases)
                {
                    NodeIntermediateModel nim;
                    nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLE);
                    nim.SetAttributes(std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_AttributeId::UA_ATTRIBUTEID_DATATYPE, nodeid_aliase.second}});
                    MESSAGE(nodeid_aliase.first, " == ", nim.GetDataTypeAlias());
                    CHECK_EQ(nodeid_aliase.first, nim.GetDataTypeAlias());
                }
            }
            SUBCASE("Only the Variable and VariableType node classes should be aliased, the rest should be given an empty string")
            {
                NodeIntermediateModel nim;
                nim.SetAttributes(std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_AttributeId::UA_ATTRIBUTEID_DATATYPE, nodeid_aliases.at("Boolean")}});
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLE);
                CHECK_EQ(std::string("Boolean"), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_DATATYPE);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_REFERENCETYPE);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLETYPE);
                CHECK_EQ(std::string("Boolean"), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_OBJECTTYPE);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_OBJECT);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_METHOD);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_VIEW);
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
            }

            SUBCASE("In case the request occurs before the UA_ATTRIBUTEID_DATATYPE attribute is set in the NodeIntermediateModel, there must be an empty string")
            {
                NodeIntermediateModel nim;
                nim.SetNodeClass(UA_NodeClass::UA_NODECLASS_VARIABLE);
                CHECK_NE(std::string("Boolean"), nim.GetDataTypeAlias());
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());

                // Even if there are some attributes, but not the one that is needed, it's still an empty string
                nim.SetAttributes(std::map<UA_AttributeId, std::optional<VariantsOfAttr>>{{UA_AttributeId::UA_ATTRIBUTEID_SYMMETRIC, std::optional<VariantsOfAttr>(static_cast<UA_Boolean>(true))}});
                CHECK_NE(std::string("Boolean"), nim.GetDataTypeAlias());
                CHECK_EQ(std::string(""), nim.GetDataTypeAlias());
            }
        }

        SUBCASE("Retrieving links with their aliases - GetNodeReferenceTypeAliases()")
        {
#pragma region Test data
            std::map<std::string, UATypesContainer<UA_NodeId>> nodeid_ref_types{
                {"References", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), UA_TYPES_NODEID)},
                {"NonHierarchicalReferences", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES), UA_TYPES_NODEID)},
                {"HierarchicalReferences", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), UA_TYPES_NODEID)},
                {"HasChild", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), UA_TYPES_NODEID)},
                {"Organizes", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_TYPES_NODEID)},
                {"HasEventSource", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE), UA_TYPES_NODEID)},
                {"HasModellingRule", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE), UA_TYPES_NODEID)},
                {"HasEncoding", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASENCODING), UA_TYPES_NODEID)},
                {"HasDescription", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASDESCRIPTION), UA_TYPES_NODEID)},
                {"HasTypeDefinition", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), UA_TYPES_NODEID)},
                {"GeneratesEvent", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_GENERATESEVENT), UA_TYPES_NODEID)},
                {"Aggregates", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), UA_TYPES_NODEID)},
                {"HasSubtype", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_TYPES_NODEID)},
                {"HasProperty", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), UA_TYPES_NODEID)},
                {"HasComponent", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TYPES_NODEID)},
                {"HasNotifier", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASNOTIFIER), UA_TYPES_NODEID)},
                {"HasOrderedComponent", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_TYPES_NODEID)},
                {"FromState", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_FROMSTATE), UA_TYPES_NODEID)},
                {"ToState", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_TOSTATE), UA_TYPES_NODEID)},
                {"HasCause", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCAUSE), UA_TYPES_NODEID)},
                {"HasEffect", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASEFFECT), UA_TYPES_NODEID)},
                {"HasHistoricalConfiguration", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASHISTORICALCONFIGURATION), UA_TYPES_NODEID)},
                {"HasTrueSubState", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASTRUESUBSTATE), UA_TYPES_NODEID)},
                {"HasFalseSubState", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASFALSESUBSTATE), UA_TYPES_NODEID)},
                {"HasDictionaryEntry", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASDICTIONARYENTRY), UA_TYPES_NODEID)},
                {"HasCondition", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION), UA_TYPES_NODEID)},
                {"HasGuard", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASGUARD), UA_TYPES_NODEID)},
                {"HasAddIn", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASADDIN), UA_TYPES_NODEID)},
                {"HasAddIn", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASADDIN), UA_TYPES_NODEID)},
                {"HasInterface", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(0, UA_NS0ID_HASINTERFACE), UA_TYPES_NODEID)},
                {"ns=1;i=556", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(1, 556), UA_TYPES_NODEID)}, // Custom
                {"ns=1;s=SomeRefType", UATypesContainer<UA_NodeId>(UA_NODEID_STRING(1, "SomeRefType"), UA_TYPES_NODEID)}, // Custom
                {"ns=1;i=23", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(1, 23), UA_TYPES_NODEID)}, // Custom
                {"ns=3;i=123", UATypesContainer<UA_NodeId>(UA_NODEID_NUMERIC(3, 123), UA_TYPES_NODEID)}}; // Custom

            std::vector<UATypesContainer<UA_ReferenceDescription>> v_ref_desc;
            for (const auto& nodeid_ref_type : nodeid_ref_types)
            {
                UATypesContainer<UA_ReferenceDescription> ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
                UA_NodeId_copy(&nodeid_ref_type.second.GetRef(), &ref_desc.GetRef().referenceTypeId);
                v_ref_desc.emplace_back(std::move(ref_desc));
            }
#pragma endregion Test data
            SUBCASE("If there are no references in the NodeIntermediateModel object, an empty object is returned")
            {
                NodeIntermediateModel nim;
                CHECK(nim.GetNodeReferenceTypeAliases().empty());
            }

            SUBCASE("All standard link types should be returned with aliases, custom links should be returned with a text NodeId")
            {
                NodeIntermediateModel nim;
                nim.SetNodeReferences(v_ref_desc);
                auto ref_results = nim.GetNodeReferenceTypeAliases();
                CHECK(!ref_results.empty());
                size_t index = 0;
                for (const auto& nodeid_ref_type : nodeid_ref_types)
                {
                    MESSAGE(ref_results.at(index).second, " == ", nodeid_ref_type.first);
                    CHECK_EQ(ref_results.at(index).second, nodeid_ref_type.first);
                    index++;
                }
            }
        }

        /*DELETE C-resources*/
        UA_ExpandedNodeId_delete(node_id_raw);
        UA_ExpandedNodeId_delete(parent_id_raw);
        for (auto& raw_ref : list_of_ref_desc_raw)
        {
            UA_ReferenceDescription_delete(raw_ref);
        }
        /****************/
    }
}