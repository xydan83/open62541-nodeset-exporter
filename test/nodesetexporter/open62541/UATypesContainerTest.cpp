//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include <doctest/doctest.h>

#include <set>
#include <string>

using nodesetexporter::open62541::UATypesContainer;

TEST_SUITE("nodesetexporter::open62541")
{
    TEST_CASE("nodesetexporter::open62541::UATypesContainer UA_TYPES_NODEID") // NOLINT
    {
        UA_NodeId ua_node_id;
        constexpr auto node_string_name = "new node";
        constexpr auto node_namespace = 65;
        std::string node_id_txt = "ns=65;s=";
        node_id_txt.append(node_string_name);

        CHECK_EQ(UA_NodeId_parse(&ua_node_id, UA_String_fromChars(node_id_txt.c_str())), UA_STATUSCODE_GOOD);

        SUBCASE("Testing a Constructor with a Container Type Parameter")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(UA_TYPES_NODEID);

            CHECK_EQ(c_ua_nodeid.GetType(), UA_TYPES_NODEID);
        }

        SUBCASE("Testing the creation of the UA_NodeID type in the container by deep copying (constructor with parameter)")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            CHECK((&ua_node_id != &c_ua_nodeid.GetRef())); // NOLINT
            CHECK_EQ(c_ua_nodeid.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }

        SUBCASE("Testing Creation of the type UA_NODEID and assigning a container by surface copying of the pointer to the object")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(&ua_node_id, UA_TYPES_NODEID);

            CHECK((&ua_node_id == &c_ua_nodeid.GetRef())); // NOLINT
            CHECK_EQ(c_ua_nodeid.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }

        SUBCASE("Testing the Copy Constructor")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(c_ua_nodeid);

            CHECK((&c_ua_nodeid2.GetRef() != &c_ua_nodeid.GetRef())); // NOLINT
            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }

        SUBCASE("Testing the copy assignment operator")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(UA_TYPES_NODEID);
            c_ua_nodeid2 = c_ua_nodeid;

            CHECK((&c_ua_nodeid2.GetRef() != &c_ua_nodeid.GetRef())); // NOLINT
            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }

        SUBCASE("Testing the movement designer")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(std::move(c_ua_nodeid));


            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }

        SUBCASE("Testing the assignment operator")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(UA_TYPES_NODEID);
            c_ua_nodeid2 = std::move(c_ua_nodeid);

            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_STRING_ALLOC(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
            UA_String_clear(&ua_node_string_name);
        }
        UA_NodeId_clear(&ua_node_id);
    }

    TEST_CASE("idsmart::connector::open62541cpp::common::UATypesContainer::SetParamFromString") // NOLINT
    {
        UA_NodeId ua_node_id_1;
        UA_ExpandedNodeId ua_exp_node_id_1;
        SUBCASE("String identifier")
        {
            constexpr auto node_id_string = "ns=2;s=some.node.id";
            constexpr auto exp_node_id_string = "svr=1;nsu=http://test.org/UA/Data/;s=some.node.id";
            CHECK_EQ(UA_NodeId_parse(&ua_node_id_1, UA_STRING(std::string(node_id_string).data())), UA_STATUSCODE_GOOD);
            CHECK_EQ(UA_ExpandedNodeId_parse(&ua_exp_node_id_1, UA_STRING(std::string(exp_node_id_string).data())), UA_STATUSCODE_GOOD);

            SUBCASE("SetParamFromString от UA_NodeID (str move)")
            {
                UATypesContainer<UA_NodeId> node_id(UA_TYPES_NODEID);
                node_id.SetParamFromString(node_id_string);
                CHECK_EQ(node_id.GetType(), UA_TYPES_NODEID);
                CHECK_EQ(node_id.ToString(), node_id_string);
                CHECK(UA_NodeId_equal(&ua_node_id_1, &node_id.GetRef()));
                MESSAGE(node_id.ToString());
            }

            SUBCASE("SetParamFromString от UA_NodeID (str ref)")
            {
                UATypesContainer<UA_NodeId> node_id(UA_TYPES_NODEID);
                std::string str_node_id(node_id_string);
                node_id.SetParamFromString(str_node_id);
                CHECK_EQ(node_id.GetType(), UA_TYPES_NODEID);
                CHECK_EQ(node_id.ToString(), node_id_string);
                CHECK(UA_NodeId_equal(&ua_node_id_1, &node_id.GetRef()));
                MESSAGE(node_id.ToString());
            }

            SUBCASE("SetParamFromString from UA_ExpandedNodeID (str move)")
            {
                UATypesContainer<UA_ExpandedNodeId> exp_node_id(UA_TYPES_EXPANDEDNODEID);
                exp_node_id.SetParamFromString(exp_node_id_string);
                CHECK_EQ(exp_node_id.GetType(), UA_TYPES_EXPANDEDNODEID);
                CHECK_EQ(exp_node_id.ToString(), exp_node_id_string);
                CHECK(UA_ExpandedNodeId_equal(&ua_exp_node_id_1, &exp_node_id.GetRef()));
                MESSAGE(exp_node_id.ToString());
            }

            SUBCASE("SetParamFromString from UA_ExpandedNodeID (str ref)")
            {
                UATypesContainer<UA_ExpandedNodeId> exp_node_id(UA_TYPES_EXPANDEDNODEID);
                std::string str_node_id(exp_node_id_string);
                exp_node_id.SetParamFromString(str_node_id);
                CHECK_EQ(exp_node_id.GetType(), UA_TYPES_EXPANDEDNODEID);
                CHECK_EQ(exp_node_id.ToString(), exp_node_id_string);
                CHECK(UA_ExpandedNodeId_equal(&ua_exp_node_id_1, &exp_node_id.GetRef()));
                MESSAGE(exp_node_id.ToString());
            }
        }

        SUBCASE("Integer identifier")
        {
            constexpr auto node_id_string = "ns=2;i=6009";
            constexpr auto exp_node_id_string = "svr=2;ns=2;i=6009";
            CHECK_EQ(UA_NodeId_parse(&ua_node_id_1, UA_STRING(std::string(node_id_string).data())), UA_STATUSCODE_GOOD);
            CHECK_EQ(UA_ExpandedNodeId_parse(&ua_exp_node_id_1, UA_STRING(std::string(exp_node_id_string).data())), UA_STATUSCODE_GOOD);

            SUBCASE("SetParamFromString from UA_NodeID (str move)")
            {
                UATypesContainer<UA_NodeId> node_id(UA_TYPES_NODEID);
                node_id.SetParamFromString(node_id_string);
                CHECK_EQ(node_id.GetType(), UA_TYPES_NODEID);
                CHECK_EQ(node_id.ToString(), node_id_string);
                CHECK(UA_NodeId_equal(&ua_node_id_1, &node_id.GetRef()));
                MESSAGE(node_id.ToString());
            }

            SUBCASE("SetParamFromString from UA_NodeID (str ref)")
            {
                UATypesContainer<UA_NodeId> node_id(UA_TYPES_NODEID);
                std::string str_node_id(node_id_string);
                node_id.SetParamFromString(str_node_id);
                CHECK_EQ(node_id.GetType(), UA_TYPES_NODEID);
                CHECK_EQ(node_id.ToString(), node_id_string);
                CHECK(UA_NodeId_equal(&ua_node_id_1, &node_id.GetRef()));
                MESSAGE(node_id.ToString());
            }

            SUBCASE("SetParamFromString from UA_ExpandedNodeID (str move)")
            {
                UATypesContainer<UA_ExpandedNodeId> exp_node_id(UA_TYPES_EXPANDEDNODEID);
                exp_node_id.SetParamFromString(exp_node_id_string);
                CHECK_EQ(exp_node_id.GetType(), UA_TYPES_EXPANDEDNODEID);
                CHECK_EQ(exp_node_id.ToString(), exp_node_id_string);
                CHECK(UA_ExpandedNodeId_equal(&ua_exp_node_id_1, &exp_node_id.GetRef()));
                MESSAGE(exp_node_id.ToString());
            }

            SUBCASE("SetParamFromString from UA_ExpandedNodeID (str ref)")
            {
                UATypesContainer<UA_ExpandedNodeId> exp_node_id(UA_TYPES_EXPANDEDNODEID);
                std::string str_node_id(exp_node_id_string);
                exp_node_id.SetParamFromString(str_node_id);
                CHECK_EQ(exp_node_id.GetType(), UA_TYPES_EXPANDEDNODEID);
                CHECK_EQ(exp_node_id.ToString(), exp_node_id_string);
                CHECK(UA_ExpandedNodeId_equal(&ua_exp_node_id_1, &exp_node_id.GetRef()));
                MESSAGE(exp_node_id.ToString());
            }
        }
        UA_NodeId_clear(&ua_node_id_1);
        UA_ExpandedNodeId_clear(&ua_exp_node_id_1);
    }

    TEST_CASE("idsmart::connector::nodesetexporter::open62541::UATypesContainer UA_TYPES_UINT64") // NOLINT
    {
        SUBCASE("Testing a Constructor with a Container Type Parameter")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_TYPES_UINT64);
            c_ua_int64.GetRef() = UA_INT64_MAX;

            CHECK_EQ(c_ua_int64.GetType(), UA_TYPES_UINT64);
        }

        SUBCASE("Testing the creation of the UA_Int64 type in a container by deep copying (constructor with parameter)")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_INT64_MAX, UA_TYPES_UINT64);

            CHECK_EQ(c_ua_int64.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64.ToString(), std::to_string(UA_INT64_MAX));
        }

        SUBCASE("Testing Creation of the type UA_INT64 and assigning a container by surface copying of the pointer to the object")
        {
            UA_Int64* ua_int64 = UA_Int64_new();
            *ua_int64 = UA_INT64_MAX;
            UATypesContainer<UA_Int64> c_ua_int64(ua_int64, UA_TYPES_UINT64);

            CHECK_EQ(ua_int64, &c_ua_int64.GetRef());
            CHECK_EQ(c_ua_int64.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64.ToString(), std::to_string(UA_INT64_MAX));
            UA_Int64_delete(ua_int64);
        }

        SUBCASE("Testing the Copy Constructor")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_INT64_MAX, UA_TYPES_UINT64);

            UATypesContainer<UA_Int64> c_ua_int64_2(c_ua_int64);

            CHECK((&c_ua_int64_2.GetRef() != &c_ua_int64.GetRef())); // NOLINT
            CHECK_EQ(c_ua_int64_2.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64_2.ToString(), std::to_string(UA_INT64_MAX));
        }

        SUBCASE("Testing the Copy Assignment Operator")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_INT64_MAX, UA_TYPES_UINT64);

            UATypesContainer<UA_Int64> c_ua_int64_2(UA_TYPES_UINT64);
            c_ua_int64_2 = c_ua_int64;

            CHECK((&c_ua_int64_2.GetRef() != &c_ua_int64.GetRef())); // NOLINT
            CHECK_EQ(c_ua_int64_2.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64_2.ToString(), std::to_string(UA_INT64_MAX));
        }

        SUBCASE("Testing the move constructor")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_INT64_MAX, UA_TYPES_UINT64);

            UATypesContainer<UA_Int64> c_ua_int64_2(std::move(c_ua_int64));

            CHECK_EQ(c_ua_int64_2.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64_2.ToString(), std::to_string(UA_INT64_MAX));
        }

        SUBCASE("Testing the move assignment operator")
        {
            UATypesContainer<UA_Int64> c_ua_int64(UA_INT64_MAX, UA_TYPES_UINT64);

            UATypesContainer<UA_Int64> c_ua_int64_2(UA_TYPES_UINT64);
            c_ua_int64_2 = std::move(c_ua_int64);

            CHECK_EQ(c_ua_int64_2.GetRef(), UA_INT64_MAX);
            CHECK_EQ(c_ua_int64_2.ToString(), std::to_string(UA_INT64_MAX));
        }
    }

    TEST_CASE("std::less<UATypesContainer<TUA_Struct>>") // NOLINT
    {
        UA_NodeId ua_node_id_1;
        CHECK_EQ(UA_NodeId_parse(&ua_node_id_1, UA_STRING("ns=2;i=1")), UA_STATUSCODE_GOOD);
        UA_NodeId ua_node_id_2;
        CHECK_EQ(UA_NodeId_parse(&ua_node_id_2, UA_STRING("ns=2;i=10")), UA_STATUSCODE_GOOD);

        SUBCASE("Full copying of the object")
        {
            SUBCASE("Checking the assembly premises in the SET container, where the key is the UATypesContainer<UA_Nodeid>")
            {
                UATypesContainer<UA_NodeId> c_ua_nodeid1(ua_node_id_1, UA_TYPES_NODEID);
                UATypesContainer<UA_NodeId> c_ua_nodeid1_sec(ua_node_id_1, UA_TYPES_NODEID);
                UATypesContainer<UA_NodeId> c_ua_nodeid2(ua_node_id_2, UA_TYPES_NODEID);

                std::set<UATypesContainer<UA_NodeId>> container{c_ua_nodeid1, c_ua_nodeid1_sec, c_ua_nodeid2};
                CHECK_EQ(container.size(), 2);
                auto extract_node = container.extract(c_ua_nodeid1);
                CHECK_FALSE(extract_node.empty());
                CHECK(UA_NodeId_equal(&extract_node.value().GetRef(), &c_ua_nodeid1.GetRef()));
                MESSAGE(extract_node.value().ToString());
                CHECK_EQ(extract_node.value().ToString(), "ns=2;i=1");
            }

            SUBCASE("Verifying that a node is placed in a set container where the key is UATypesContainer<UA_NodeId>")
            {
                auto ua_exp_nodeid1 = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1.serverIndex = 2;
                ua_exp_nodeid1.namespaceUri = UA_STRING("urn:some:test");
                auto ua_exp_nodeid1_sec = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1_sec.serverIndex = 2;
                ua_exp_nodeid1_sec.namespaceUri = UA_STRING("urn:some:test");
                auto ua_exp_nodeid2 = UA_EXPANDEDNODEID_NODEID(ua_node_id_2);

                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1(ua_exp_nodeid1, UA_TYPES_EXPANDEDNODEID);
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1_sec(ua_exp_nodeid1_sec, UA_TYPES_EXPANDEDNODEID);
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid2(ua_exp_nodeid2, UA_TYPES_EXPANDEDNODEID);

                std::set<UATypesContainer<UA_ExpandedNodeId>> container{c_ua_exp_nodeid1, c_ua_exp_nodeid1_sec, c_ua_exp_nodeid2};
                CHECK_EQ(container.size(), 2);
                auto extract_node = container.extract(c_ua_exp_nodeid2);
                CHECK_FALSE(extract_node.empty());
                CHECK(UA_ExpandedNodeId_equal(&extract_node.value().GetRef(), &c_ua_exp_nodeid2.GetRef()));
                MESSAGE(extract_node.value().ToString());
                CHECK_EQ(extract_node.value().ToString(), "ns=2;i=10");
                extract_node = container.extract(c_ua_exp_nodeid1);
                CHECK(UA_ExpandedNodeId_equal(&extract_node.value().GetRef(), &ua_exp_nodeid1));
            }
        }

        SUBCASE("Superficial copying of the object") // NOLINT
        {
            SUBCASE("Checking the assembly premises in the SET container, where the key is the UATypesContainer<UA_Nodeid>")
            {
                UATypesContainer<UA_NodeId> c_ua_nodeid1(&ua_node_id_1, UA_TYPES_NODEID);
                UATypesContainer<UA_NodeId> c_ua_nodeid1_sec(&ua_node_id_1, UA_TYPES_NODEID);
                UATypesContainer<UA_NodeId> c_ua_nodeid2(&ua_node_id_2, UA_TYPES_NODEID);

                std::set<UATypesContainer<UA_NodeId>> container{c_ua_nodeid1, c_ua_nodeid1_sec, c_ua_nodeid2};
                CHECK_EQ(container.size(), 2);
                auto extract_node = container.extract(c_ua_nodeid1);
                CHECK_FALSE(extract_node.empty());
                CHECK(UA_NodeId_equal(&extract_node.value().GetRef(), &c_ua_nodeid1.GetRef()));
                MESSAGE(extract_node.value().ToString());
                CHECK_EQ(extract_node.value().ToString(), "ns=2;i=1");
            }

            SUBCASE("Checking the assembly premises in the SET container, where the key is the UATypesContainer<UA_ExpandedNodeId>")
            {
                auto ua_exp_nodeid1 = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1.serverIndex = 2;
                ua_exp_nodeid1.namespaceUri = UA_STRING("urn:some:test");
                auto ua_exp_nodeid1_sec = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1_sec.serverIndex = 2;
                ua_exp_nodeid1_sec.namespaceUri = UA_STRING("urn:some:test");
                auto ua_exp_nodeid2 = UA_EXPANDEDNODEID_NODEID(ua_node_id_2);

                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1(&ua_exp_nodeid1, UA_TYPES_EXPANDEDNODEID);
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1_sec(&ua_exp_nodeid1_sec, UA_TYPES_EXPANDEDNODEID);
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid2(&ua_exp_nodeid2, UA_TYPES_EXPANDEDNODEID);

                std::set<UATypesContainer<UA_ExpandedNodeId>> container{c_ua_exp_nodeid1, c_ua_exp_nodeid1_sec, c_ua_exp_nodeid2};
                CHECK_EQ(container.size(), 2);
                auto extract_node = container.extract(c_ua_exp_nodeid2);
                CHECK_FALSE(extract_node.empty());
                CHECK(UA_ExpandedNodeId_equal(&extract_node.value().GetRef(), &ua_exp_nodeid2));
                MESSAGE(extract_node.value().ToString());
                CHECK_EQ(extract_node.value().ToString(), "ns=2;i=10");
                extract_node = container.extract(c_ua_exp_nodeid1_sec);
                CHECK(UA_ExpandedNodeId_equal(&extract_node.value().GetRef(), &ua_exp_nodeid1_sec));
            }
        }
        UA_NodeId_clear(&ua_node_id_1);
        UA_NodeId_clear(&ua_node_id_2);
    }

    TEST_CASE("std::hash<Open62541TypesContainer<TUA_Struct>>") // NOLINT
    {
        UA_NodeId ua_node_id_1;
        CHECK_EQ(UA_NodeId_parse(&ua_node_id_1, UA_STRING("ns=2;i=1")), UA_STATUSCODE_GOOD);
        std::size_t hash = 0;

        SUBCASE("Full copying of the object")
        {

            SUBCASE("Checking hash with UATypesContainer<UA_NodeId>")
            {
                UATypesContainer<UA_NodeId> c_ua_nodeid1(ua_node_id_1, UA_TYPES_NODEID);

                CHECK_NOTHROW(hash = std::hash<UATypesContainer<UA_NodeId>>{}(c_ua_nodeid1));
                CHECK_NE(hash, 0);
                MESSAGE(hash);
            }

            SUBCASE("Checking hash with UATypesContainer<UA_ExpandedNodeId>")
            {
                auto ua_exp_nodeid1 = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1.serverIndex = 2;
                ua_exp_nodeid1.namespaceUri = UA_STRING("urn:some:test");
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1(ua_exp_nodeid1, UA_TYPES_EXPANDEDNODEID);

                CHECK_NOTHROW(hash = std::hash<UATypesContainer<UA_ExpandedNodeId>>{}(c_ua_exp_nodeid1));
                CHECK_NE(hash, 0);
                MESSAGE(hash);
            }
        }

        SUBCASE("Superficial copying of the object")
        {
            SUBCASE("Checking hash with UATypesContainer<UA_NodeId>")
            {
                UATypesContainer<UA_NodeId> c_ua_nodeid1(&ua_node_id_1, UA_TYPES_NODEID);
                CHECK_NOTHROW(hash = std::hash<UATypesContainer<UA_NodeId>>{}(c_ua_nodeid1));
                CHECK_NE(hash, 0);
                MESSAGE(hash);
            }

            SUBCASE("Checking hash with UATypesContainer<UA_ExpandedNodeId>")
            {
                auto ua_exp_nodeid1 = UA_EXPANDEDNODEID_NODEID(ua_node_id_1);
                ua_exp_nodeid1.serverIndex = 2;
                ua_exp_nodeid1.namespaceUri = UA_STRING("urn:some:test");
                UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1(&ua_exp_nodeid1, UA_TYPES_EXPANDEDNODEID);
                CHECK_NOTHROW(hash = std::hash<UATypesContainer<UA_ExpandedNodeId>>{}(c_ua_exp_nodeid1));
                CHECK_NE(hash, 0);
                MESSAGE(hash);
            }
        }
        UA_NodeId_clear(&ua_node_id_1);
    }
}