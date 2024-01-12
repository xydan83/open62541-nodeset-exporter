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
#include <gsl/gsl>

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

        Ensures(UA_NodeId_parse(&ua_node_id, UA_String_fromChars(node_id_txt.c_str())) == UA_STATUSCODE_GOOD);

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
            auto ua_node_string_name = UA_String_fromChars(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid.ToString(), node_id_txt);
        }

        SUBCASE("Testing the Copy Constructor")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(c_ua_nodeid);

            CHECK((&c_ua_nodeid2.GetRef() != &c_ua_nodeid.GetRef())); // NOLINT
            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_String_fromChars(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
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
            auto ua_node_string_name = UA_String_fromChars(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
        }

        SUBCASE("Testing the move constructor")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(std::move(c_ua_nodeid));


            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_String_fromChars(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
        }

        SUBCASE("Testing the move assignment operator")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(ua_node_id, UA_TYPES_NODEID);

            UATypesContainer<UA_NodeId> c_ua_nodeid2(UA_TYPES_NODEID);
            c_ua_nodeid2 = std::move(c_ua_nodeid);

            CHECK_EQ(c_ua_nodeid2.GetType(), UA_TYPES_NODEID);
            CHECK_EQ(c_ua_nodeid2.GetRef().namespaceIndex, node_namespace);
            CHECK_EQ(c_ua_nodeid2.GetRef().identifierType, UA_NodeIdType::UA_NODEIDTYPE_STRING);
            auto ua_node_string_name = UA_String_fromChars(node_string_name);
            CHECK(UA_String_equal(&c_ua_nodeid2.GetRef().identifier.string, &ua_node_string_name)); // NOLINT
            CHECK_EQ(c_ua_nodeid2.ToString(), node_id_txt);
        }

        SUBCASE("Testing the status code")
        {
            UATypesContainer<UA_NodeId> c_ua_nodeid(UA_TYPES_NODEID);

            UA_StatusCode test_code = UA_STATUSCODE_BADAGGREGATEINVALIDINPUTS;
            c_ua_nodeid.SetStatusCode(test_code);

            CHECK_EQ(c_ua_nodeid.GetStatusCode(), UA_STATUSCODE_BADAGGREGATEINVALIDINPUTS);
        }
    }

    TEST_CASE("nodesetexporter::open62541::UATypesContainer UA_TYPES_UINT64") // NOLINT
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
        Ensures(UA_NodeId_parse(&ua_node_id_1, UA_String_fromChars("ns=2;i=1")) == UA_STATUSCODE_GOOD);
        UA_NodeId ua_node_id_2;
        Ensures(UA_NodeId_parse(&ua_node_id_2, UA_String_fromChars("ns=2;i=10")) == UA_STATUSCODE_GOOD);
        UATypesContainer<UA_NodeId> c_ua_nodeid1(ua_node_id_1, UA_TYPES_NODEID);
        UATypesContainer<UA_NodeId> c_ua_nodeid1_sec(ua_node_id_1, UA_TYPES_NODEID);
        UATypesContainer<UA_NodeId> c_ua_nodeid2(ua_node_id_2, UA_TYPES_NODEID);
        UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1(UA_EXPANDEDNODEID_NODEID(ua_node_id_1), UA_TYPES_EXPANDEDNODEID);
        UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid1_sec(UA_EXPANDEDNODEID_NODEID(ua_node_id_1), UA_TYPES_EXPANDEDNODEID);
        UATypesContainer<UA_ExpandedNodeId> c_ua_exp_nodeid2(UA_EXPANDEDNODEID_NODEID(ua_node_id_2), UA_TYPES_EXPANDEDNODEID);

        SUBCASE("Verifying that a node is placed in a set container where the key is UATypesContainer<UA_NodeId>")
        {
            std::set<UATypesContainer<UA_NodeId>> container{c_ua_nodeid1, c_ua_nodeid1_sec, c_ua_nodeid2};
            CHECK_EQ(container.size(), 2);
            auto extract_node = container.extract(c_ua_nodeid1);
            CHECK_FALSE(extract_node.empty());
            MESSAGE(extract_node.value().ToString());
            CHECK_EQ(extract_node.value().ToString(), "ns=2;i=1");
        }

        SUBCASE("Verifying that a node is placed in a set container where the key is UATypesContainer<UA_ExpandedNodeId>")
        {
            std::set<UATypesContainer<UA_ExpandedNodeId>> container{c_ua_exp_nodeid1, c_ua_exp_nodeid1_sec, c_ua_exp_nodeid2};
            CHECK_EQ(container.size(), 2);
            auto extract_node = container.extract(c_ua_exp_nodeid2);
            CHECK_FALSE(extract_node.empty());
            MESSAGE(extract_node.value().ToString());
            CHECK_EQ(extract_node.value().ToString(), "ns=2;i=10");
        }
    }
}