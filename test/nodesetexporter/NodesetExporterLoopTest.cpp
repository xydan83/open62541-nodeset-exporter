//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

// todo Redesign the tests into a more understandable form. Remove the generators, form everything manually so that you can see what and where it comes from. Make tests of private methods, not just
//  close-ups.

// fixme testing fork and merge request

#include "nodesetexporter/NodesetExporterLoop.h"
#include "nodesetexporter/open62541/UATypesContainer.h"

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>

#include <random>
#include <vector>

using nodesetexporter::NodeIntermediateModel;
using nodesetexporter::NodesetExporterLoop;
using nodesetexporter::UATypesContainer;
using nodesetexporter::VariantsOfAttr;
using nodesetexporter::interfaces::IEncoder;
using nodesetexporter::interfaces::IOpen62541;
using StatusResults = ::nodesetexporter::common::statuses::StatusResults;

namespace
{
using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LogLevel = nodesetexporter::common::LogLevel;

class Logger final : public LoggerBase
{
public:
    explicit Logger(std::string&& logger_name)
        : LoggerBase<std::string>(std::move(logger_name))
    {
    }

private:
    void VTrace(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VDebug(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VInfo(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VWarning(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VError(std::string&& message) override
    {
        MESSAGE(message);
    }
    void VCritical(std::string&& message) override
    {
        MESSAGE(message);
    }
};

class MockEncoder : public trompeloeil::mock_interface<IEncoder>
{
public:
    MockEncoder(LoggerBase& logger, std::string&& filename_without_ext)
        : trompeloeil::mock_interface<IEncoder>(logger, std::move(filename_without_ext))
    {
    }

    MockEncoder(LoggerBase& logger, std::iostream& out_buffer)
        : trompeloeil::mock_interface<IEncoder>(logger, out_buffer)
    {
    }

    IMPLEMENT_MOCK0(Begin);
    IMPLEMENT_MOCK0(End);
    IMPLEMENT_MOCK1(AddNamespaces);
    IMPLEMENT_MOCK1(AddAliases);
    IMPLEMENT_MOCK1(AddNodeObject);
    IMPLEMENT_MOCK1(AddNodeObjectType);
    IMPLEMENT_MOCK1(AddNodeVariable);
    IMPLEMENT_MOCK1(AddNodeVariableType);
    IMPLEMENT_MOCK1(AddNodeReferenceType);
    IMPLEMENT_MOCK1(AddNodeDataType);
};

class MockOpen62541 : public trompeloeil::mock_interface<IOpen62541>
{
public:
    explicit MockOpen62541(LoggerBase& logger)
        : trompeloeil::mock_interface<IOpen62541>(logger)
    {
    }
    IMPLEMENT_MOCK1(ReadNodeClasses);
    IMPLEMENT_MOCK1(ReadNodeReferences);
    IMPLEMENT_MOCK1(ReadNodesAttributes);
    IMPLEMENT_MOCK2(ReadNodeDataValue);
};

/**
 * @brief Generator of positive, random numbers with setting a range
 * @param min Minimum Number
 * @param max Maximum number
 * @return Number type size_t
 */
inline int64_t GetRandomNumber(int64_t min, int64_t max)
{
    std::random_device random; // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(random()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int64_t> distrib(min, max);
    //    MESSAGE("RND: ", rnd);
    return distrib(gen);
}

/**
 * @brief Generation of UATypesContainer<UA_ReferenceDescription> objects with random filling of most attributes but within reasonable values
 * @param type_id Initial node type (first should be UA_NS0ID_HIERARCHICALREFERENCES)
 * @param is_forward Reference type - forward or reverse (the first must be UA_FALSE)
 * @return UATypesContainer<UA_ReferenceDescription>
 */
inline UATypesContainer<UA_ReferenceDescription> ReferenceDescriptionGen(UA_Int64& type_id, UA_Boolean& is_forward) // NOLINT
{
    constexpr size_t min = 1;
    constexpr size_t max = 199;
    std::string str_node_id;
    auto str_ref_type_id = "i=" + std::to_string(type_id);
    if (GetRandomNumber(0, 1) != 0)
    {
        str_node_id = "ns=2;i=" + std::to_string(GetRandomNumber(min, max));
    }
    else
    {
        str_node_id = "ns=2;s=Some node id" + std::to_string(GetRandomNumber(min, max));
    }
    auto str_type_def = "i=" + std::to_string(GetRandomNumber(UA_NS0ID_BOOLEAN, UA_NS0ID_IMAGE));

    auto str_br_name = "Test browse name " + std::to_string(GetRandomNumber(min, UA_UINT32_MAX));
    auto str_dspl_name = "Test display name " + std::to_string(GetRandomNumber(min, UA_UINT32_MAX));

    UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
    Ensures(UA_NodeId_parse(&ref_desc_cnt.GetRef().referenceTypeId, UA_String_fromChars(str_ref_type_id.c_str())) == UA_STATUSCODE_GOOD);
    Ensures(UA_NodeId_parse(&ref_desc_cnt.GetRef().nodeId.nodeId, UA_String_fromChars(str_node_id.c_str())) == UA_STATUSCODE_GOOD);
    Ensures(UA_NodeId_parse(&ref_desc_cnt.GetRef().typeDefinition.nodeId, UA_String_fromChars(str_type_def.c_str())) == UA_STATUSCODE_GOOD);
    ref_desc_cnt.GetRef().isForward = is_forward;
    ref_desc_cnt.GetRef().browseName.namespaceIndex = 1;
    ref_desc_cnt.GetRef().browseName.name = UA_String_fromChars(str_br_name.c_str());
    ref_desc_cnt.GetRef().nodeClass = UA_NodeClass::UA_NODECLASS_REFERENCETYPE;
    ref_desc_cnt.GetRef().displayName.locale = UA_String_fromChars("en");
    ref_desc_cnt.GetRef().displayName.text = UA_String_fromChars(str_dspl_name.c_str());
    is_forward = is_forward == UA_FALSE ? UA_TRUE : UA_FALSE; // NOLINT
    type_id = GetRandomNumber(UA_NS0ID_REFERENCES, UA_NS0ID_HASHISTORICALCONFIGURATION);
    return ref_desc_cnt;
}

/**
 * @brief Attribute generator by their ID.
 * @param attr_id attribute ID.
 * @return Attribute object.
 */
inline VariantsOfAttr AttributesGen(UA_AttributeId attr_id)
{
    switch (attr_id)
    {
    case UA_ATTRIBUTEID_BROWSENAME:
        return VariantsOfAttr{UATypesContainer<UA_QualifiedName>{UA_QUALIFIEDNAME_ALLOC(1, "Some Browsename"), UA_TYPES_QUALIFIEDNAME}};
    case UA_ATTRIBUTEID_DISPLAYNAME:
        return VariantsOfAttr{UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT_ALLOC("en", "Some displayname"), UA_TYPES_LOCALIZEDTEXT}};
    case UA_ATTRIBUTEID_DESCRIPTION:
        return VariantsOfAttr{UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT_ALLOC("en", "Some attribute description"), UA_TYPES_LOCALIZEDTEXT}};
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        return VariantsOfAttr{static_cast<UA_Byte>(UA_TRUE)};
    case UA_ATTRIBUTEID_ISABSTRACT:
        return VariantsOfAttr{static_cast<UA_Boolean>(UA_FALSE)};
    case UA_ATTRIBUTEID_DATATYPE:
        return VariantsOfAttr{UATypesContainer<UA_NodeId>{UA_NODEID_NUMERIC(0, GetRandomNumber(1, 64)), UA_TYPES_NODEID}}; // NOLINT
    case UA_ATTRIBUTEID_VALUERANK:
        return VariantsOfAttr{static_cast<UA_Int32>(GetRandomNumber(-3, 1))};
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        return VariantsOfAttr{std::vector<UA_UInt32>{
            static_cast<UA_UInt32>(GetRandomNumber(UA_UINT32_MIN, UA_UINT32_MAX)),
            static_cast<UA_UInt32>(GetRandomNumber(UA_UINT32_MIN, UA_UINT32_MAX)),
            static_cast<UA_UInt32>(GetRandomNumber(UA_UINT32_MIN, UA_UINT32_MAX))}};
    case UA_ATTRIBUTEID_VALUE:
    {
        auto test = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
        auto* some_scalar = UA_Int64_new();
        *some_scalar = static_cast<UA_Int64>(GetRandomNumber(UA_INT64_MIN, UA_INT64_MAX));
        UA_Variant_setScalar(&test.GetRef(), some_scalar, &UA_TYPES[UA_TYPES_INT64]);
        return VariantsOfAttr{std::move(test)};
    }
    case UA_ATTRIBUTEID_ACCESSLEVEL:
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        return VariantsOfAttr{static_cast<UA_Byte>(GetRandomNumber(UA_ACCESSLEVELTYPE_NONE, UA_ACCESSLEVELTYPE_CURRENTWRITE))};
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        return VariantsOfAttr{static_cast<UA_Double>(GetRandomNumber(0, 10000))}; // NOLINT
    case UA_ATTRIBUTEID_HISTORIZING:
        return VariantsOfAttr{static_cast<UA_Boolean>(UA_FALSE)};
    case UA_ATTRIBUTEID_INVERSENAME:
        return VariantsOfAttr{UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT_ALLOC("en", "Some inversename"), UA_TYPES_LOCALIZEDTEXT}};
    case UA_ATTRIBUTEID_DATATYPEDEFINITION:
    {
        auto test = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
        // When the UATypesContainer is destroyed, the UA_Variant will be destroyed, which will destroy all objects (structures) inside...
        if (GetRandomNumber(UA_TRUE, UA_FALSE) != 0U)
        {
            auto* str_def = UA_StructureDefinition_new(); // so I create it in the heap.
            UA_StructureDefinition_init(str_def);
            UA_Variant_setScalar(&test.GetRef(), str_def, &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
        }
        else
        {
            auto* enum_def = UA_EnumDefinition_new(); // so I create it in the heap.
            UA_EnumDefinition_init(enum_def);
            UA_Variant_setScalar(&test.GetRef(), enum_def, &UA_TYPES[UA_TYPES_ENUMDEFINITION]);
        }
        return VariantsOfAttr{test};
    }
    case UA_ATTRIBUTEID_WRITEMASK:
    case UA_ATTRIBUTEID_USERWRITEMASK:
        return VariantsOfAttr{static_cast<UA_UInt32>(GetRandomNumber(UA_UINT32_MIN, UA_UINT32_MAX))};
    case UA_ATTRIBUTEID_SYMMETRIC:
        return VariantsOfAttr{static_cast<UA_Boolean>(UA_FALSE)};
    default:
        FAIL(true, " AttributesGen - Non valid attr_id parameter");
    }
    return VariantsOfAttr{};
}

/**
 * @brief Method for comparing two lists with UA_ReferenceDescription in UATypesContainer containers.
 * The fields of each ref_descs_first are compared with each of the ref_descs_second.
 * At the first match, the comparison cycle is interrupted.
 * @param ref_descs_first Test, ideal list of node reference description objects that are compared to another list
 * @param ref_descs_second The tested list of node reference description objects to which the first list is compared.
 */
inline void CheckRefferenceDescriptions(
    const std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_descs_first, // NOLINT
    const std::vector<UATypesContainer<UA_ReferenceDescription>>& ref_descs_second) // NOLINT
{
    MESSAGE("CheckRefferenceDescriptions beginning...");
    bool is_ok = false;
    try
    {
        REQUIRE_EQ(ref_descs_first.size(), ref_descs_second.size());
    }
    catch (...)
    {
        auto whos_bigeer = ref_descs_first.size() > ref_descs_second.size() ? ref_descs_first.size() : ref_descs_second.size();
        std::string txt_buf = "Test (ideal) references\t\t\t\t\t\t|\t\t\t\t\t\tTested references \n";
        for (size_t index = 0; index < whos_bigeer; index++)
        {
            std::array<char, 256> ch_buf{};
            if (ref_descs_first.size() > index)
            {
                std::sprintf( // NOLINT
                    ch_buf.data(),
                    "%*zu) %-30.*s %-.*s <==>",
                    3,
                    index + 1,
                    30,
                    UATypesContainer<UA_NodeId>(ref_descs_first.at(index).GetRef().nodeId.nodeId, UA_TYPES_NODEID).ToString().c_str(),
                    18,
                    ref_descs_first.at(index).GetRef().isForward ? "(IsForward: True)" : "(IsForward: False)");
            }
            else
            {
                std::sprintf(ch_buf.data(), "%*zu) %-48.*s <==>", 3, index + 1, 4, "None"); // NOLINT
            }
            txt_buf += ch_buf.data();
            if (ref_descs_second.size() > index)
            {
                std::sprintf( // NOLINT
                    ch_buf.data(),
                    "%30s %s\n",
                    UATypesContainer<UA_NodeId>(ref_descs_second.at(index).GetRef().nodeId.nodeId, UA_TYPES_NODEID).ToString().c_str(),
                    ref_descs_second.at(index).GetRef().isForward ? "(IsForward: True)" : "(IsForward: False)");
            }
            else
            {
                std::sprintf(ch_buf.data(), "%48s\n", "None"); // NOLINT
            }
            txt_buf += ch_buf.data();
        }
        MESSAGE("Reference NodeIds: \n", txt_buf);
        FAIL(true);
    }

    for (const UATypesContainer<UA_ReferenceDescription>& ref_desc : ref_descs_first)
    {
        is_ok = false;
        for (const UATypesContainer<UA_ReferenceDescription>& internal_ref_desc : ref_descs_second)
        {
            if (UA_ExpandedNodeId_equal(&ref_desc.GetRef().nodeId, &internal_ref_desc.GetRef().nodeId) && ref_desc.GetRef().nodeClass == internal_ref_desc.GetRef().nodeClass
                && ref_desc.GetRef().isForward == internal_ref_desc.GetRef().isForward && UA_NodeId_equal(&ref_desc.GetRef().referenceTypeId, &internal_ref_desc.GetRef().referenceTypeId)
                && UA_ExpandedNodeId_equal(&ref_desc.GetRef().typeDefinition, &internal_ref_desc.GetRef().typeDefinition)
                && UA_QualifiedName_equal(&ref_desc.GetRef().browseName, &internal_ref_desc.GetRef().browseName)
                && UA_String_equal(&ref_desc.GetRef().displayName.locale, &internal_ref_desc.GetRef().displayName.locale)
                && UA_String_equal(&ref_desc.GetRef().displayName.text, &internal_ref_desc.GetRef().displayName.text))
            {
                is_ok = true;
                break;
            }
        }
        if (!is_ok)
        {
            REQUIRE_MESSAGE(is_ok, "Not found equal reference on the expected list :", ref_desc.ToString());
            break;
        }
    }
}
} // namespace


TEST_SUITE("nodesetexporter")
{
    const auto number_of_nodes = GetRandomNumber(80, 199); // NUMBER OF NODES
    const auto max_nodes_to_request_data = GetRandomNumber(1, number_of_nodes - 1); // Number of nodes in a kernel test package with a limit on a single data request
    const auto parent_start_node_replacer = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_TYPES_EXPANDEDNODEID);

    const std::map<std::uint32_t, std::string> types_nodeclasses{
        {UA_NODECLASS_OBJECTTYPE, "UA_NODECLASS_OBJECTTYPE"},
        {UA_NODECLASS_REFERENCETYPE, "UA_NODECLASS_REFERENCETYPE"},
        {UA_NODECLASS_DATATYPE, "UA_NODECLASS_DATATYPE"},
        {UA_NODECLASS_VARIABLETYPE, "UA_NODECLASS_VARIABLETYPE"}};

    TEST_CASE("nodesetexporter::NodesetExporterLoop") // NOLINT
    {
        using trompeloeil::_;
        using trompeloeil::eq;
        using trompeloeil::ne;
        trompeloeil::sequence seq;
        u_int32_t number_of_valid_class_nodes_to_export = 0;
        u_int32_t number_of_add_nodes_to_export = 0;

        auto exp_nodeid_null = UA_EXPANDEDNODEID_NULL;

        constexpr size_t namespace_array_size = 3;

        UATypesContainer<UA_ExpandedNodeId> server_namespace_array_request(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), UA_TYPES_EXPANDEDNODEID);
        std::vector<std::string> valid_namespaces{"http://some_opc_server/UA/", "http://some_devices/UA/"};
        auto* namespace_array = static_cast<UA_String*>(UA_Array_new(namespace_array_size, &UA_TYPES[UA_TYPES_STRING]));
        namespace_array[0] = UA_String_fromChars("http://opcfoundation.org/UA/"); // NOLINT
        namespace_array[1] = UA_String_fromChars(valid_namespaces[0].c_str()); // NOLINT
        namespace_array[2] = UA_String_fromChars(valid_namespaces[1].c_str()); // NOLINT

        // Prepare test nodes
        std::vector<UATypesContainer<UA_ExpandedNodeId>> nodes_id; // They are passed to the input of the kernel.
        std::map<UATypesContainer<UA_ExpandedNodeId>, UA_NodeClass> nodes_with_class; // Used to populate node classes in MOCK methods of the Open62541 interface.
        std::set<UATypesContainer<UA_ExpandedNodeId>> valid_node_id; // Valid node_ids for operation are used to filter test references.
        size_t power = 0;
        for (size_t index = 0; index < number_of_nodes; index++)
        {
            UA_ExpandedNodeId ua_node_id = UA_EXPANDEDNODEID_NULL;
            std::string node_id_txt;
            if (GetRandomNumber(0, 1) != 0)
            {
                node_id_txt = "ns=2;i=" + std::to_string(index + 1);
            }
            else
            {
                node_id_txt = "ns=2;s=Some node id" + std::to_string(index + 1);
            }

            CHECK_EQ(UA_NodeId_parse(&ua_node_id.nodeId, UA_String_fromChars(node_id_txt.c_str())), UA_STATUSCODE_GOOD);
            nodes_id.emplace_back(ua_node_id, UA_TYPES_EXPANDEDNODEID);
            // Assign classes to nodes randomly
            auto node_class = static_cast<UA_NodeClass>(std::pow(2, power));
            nodes_with_class.try_emplace(UATypesContainer<UA_ExpandedNodeId>(ua_node_id, UA_TYPES_EXPANDEDNODEID), node_class);
            if (node_class == UA_NODECLASS_OBJECT || node_class == UA_NODECLASS_VARIABLE || node_class == UA_NODECLASS_OBJECTTYPE || node_class == UA_NODECLASS_VARIABLETYPE
                || node_class == UA_NODECLASS_REFERENCETYPE || node_class == UA_NODECLASS_DATATYPE)
            {
                number_of_valid_class_nodes_to_export++;
                // I add only valid NODE IDs to Set
                valid_node_id.emplace(ua_node_id, UA_TYPES_EXPANDEDNODEID);
            }
            power++;
            power = power == 8 ? 0 : power;
        }

        // Additionally, I add nodes with BrowsePath. These nodes must not have an Invert Reference.
        // During the process, the algorithm in the GetNodesData method will detect that there are no back references and will add its own version based on the formation from BrowsePath.
        std::string first_browse_path_node = "BrowsePathParent";
        std::string second_browse_path_node = "BrowsePathParent.BrowsePathChild";
        auto ua_node_first_browse_path_node = UA_EXPANDEDNODEID_STRING(2, first_browse_path_node.data());
        nodes_id.emplace_back(ua_node_first_browse_path_node, UA_TYPES_EXPANDEDNODEID);
        number_of_valid_class_nodes_to_export++;
        nodes_with_class.try_emplace(UATypesContainer<UA_ExpandedNodeId>(ua_node_first_browse_path_node, UA_TYPES_EXPANDEDNODEID), UA_NODECLASS_OBJECT);
        valid_node_id.emplace(ua_node_first_browse_path_node, UA_TYPES_EXPANDEDNODEID);
        auto ua_node_second_browse_path_node = UA_EXPANDEDNODEID_STRING(2, second_browse_path_node.data());
        nodes_id.emplace_back(ua_node_second_browse_path_node, UA_TYPES_EXPANDEDNODEID);
        number_of_valid_class_nodes_to_export++;
        nodes_with_class.try_emplace(UATypesContainer<UA_ExpandedNodeId>(ua_node_second_browse_path_node, UA_TYPES_EXPANDEDNODEID), UA_NODECLASS_VARIABLE);
        valid_node_id.emplace(ua_node_second_browse_path_node, UA_TYPES_EXPANDEDNODEID);

        // Additionally, I add a node that will have HasTypeDefinition= i=62 [BaseVariableType]
        std::string change_definition_browse_path_node = "KEPDefinitionProblem";
        auto ua_node_change_definition_browse_path_node = UA_EXPANDEDNODEID_STRING(2, change_definition_browse_path_node.data());
        nodes_id.emplace_back(ua_node_change_definition_browse_path_node, UA_TYPES_EXPANDEDNODEID);
        number_of_valid_class_nodes_to_export++;
        nodes_with_class.try_emplace(UATypesContainer<UA_ExpandedNodeId>(ua_node_change_definition_browse_path_node, UA_TYPES_EXPANDEDNODEID), UA_NODECLASS_VARIABLE);
        valid_node_id.emplace(ua_node_change_definition_browse_path_node, UA_TYPES_EXPANDEDNODEID);

        MESSAGE("Valid nodes");
        for (const auto& valid_node : nodes_with_class)
        {
            MESSAGE(valid_node.first.ToString(), "  ", valid_node.second, " is valid: ", valid_node_id.contains(valid_node.first));
        }

        std::map<UATypesContainer<UA_ExpandedNodeId>, std::vector<UATypesContainer<UA_ReferenceDescription>>> cmp_ref_descr;
        Logger logger("test");
        logger.SetLevel(LogLevel::Debug);

        MockOpen62541 open(logger);
        MockEncoder encoder(logger, "nodeset");

        const auto parent_ref = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

#pragma region MockEncoder - exporting nodes
        // In any case, these methods can be called many times in a loop.
        REQUIRE_CALL(encoder, AddNodeObject(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_OBJECT)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeVariable(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeObjectType(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_OBJECTTYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeVariableType(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLETYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeReferenceType(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_REFERENCETYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeDataType(_))
            .WITH(_1.GetExpNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetStatusCode() == UA_STATUSCODE_GOOD)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_DATATYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

#pragma endregion MockEncoder - exporting nodes

#pragma region Initial steps are the same for both test conditions
        REQUIRE_CALL(encoder, Begin()).RETURN(StatusResults::Good).IN_SEQUENCE(seq);

        REQUIRE_CALL(open, ReadNodeDataValue(ANY(const UATypesContainer<UA_ExpandedNodeId>&), ANY(UATypesContainer<UA_Variant>&)))
            .WITH(UA_ExpandedNodeId_equal(&_1.GetRef(), &server_namespace_array_request.GetRef()) == true)
            .LR_SIDE_EFFECT(UA_Variant_setArray(&_2.GetRef(), namespace_array, namespace_array_size, &UA_TYPES[UA_TYPES_STRING]);)
            .RETURN(StatusResults::Good)
            .IN_SEQUENCE(seq);

        REQUIRE_CALL(encoder, AddNamespaces(eq<std::vector<std::string>>(valid_namespaces))).RETURN(StatusResults::Good).IN_SEQUENCE(seq);

#pragma endregion Initial steps are the same for both test conditions

        SUBCASE("A core test without specifying a limit on a single data request")
        {
            REQUIRE_CALL(open, ReadNodeClasses(_))
                .WITH(_1.empty() == false)
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeClassesRequestResponse& ncs
                                     : _1) { ncs.node_class = nodes_with_class.at(ncs.exp_node_id); })
                .RETURN(StatusResults::Good)
                .IN_SEQUENCE(seq);

            REQUIRE_CALL(open, ReadNodesAttributes(_))
                .WITH(_1.empty() == false)
                .SIDE_EFFECT(for (MockOpen62541::NodeAttributesRequestResponse& narr
                                  : _1) {
                    MESSAGE("NodeAttributesRequestResponse nodeID: ", narr.exp_node_id.ToString());
                    for (auto& attr : narr.attrs)
                    {
                        try
                        {
                            MESSAGE("Attr: ", attr.first, ", data: ", VariantsOfAttrToString(AttributesGen(attr.first)));
                            attr.second.emplace(std::move(AttributesGen(attr.first)));
                        }
                        catch (std::exception& exc)
                        {
                            MESSAGE(exc.what());
                            EXIT_FAILURE;
                        }
                    }
                })
                .RETURN(StatusResults::Good)
                .IN_SEQUENCE(seq);

            REQUIRE_CALL(open, ReadNodeReferences(_))
                .WITH(_1.empty() == false)
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeReferencesRequestResponse& nrrr
                                     : _1) {
                    size_t count = GetRandomNumber(50, 100);
                    UA_Int64 start_ref_type_id = UA_NS0ID_HIERARCHICALREFERENCES;
                    UA_Boolean start_is_fwd = UA_FALSE;
                    const auto current_node_class = nodes_with_class.at(nrrr.exp_node_id);
                    auto first_browse_path_node_str = UA_STRING(first_browse_path_node.data());
                    auto second_browse_path_node_str = UA_STRING(second_browse_path_node.data());
                    auto change_definition_browse_path_node_str = UA_STRING(change_definition_browse_path_node.data());
                    for (size_t index = 0; index < count; index++)
                    {
                        // For some nodes, you only need to generate direct references. BrowsePath validation test.
                        if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                            && (UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &first_browse_path_node_str) // NOLINT(cppcoreguidelines-pro-type-union-access)
                                || UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &second_browse_path_node_str))) // NOLINT(cppcoreguidelines-pro-type-union-access)
                        {
                            start_is_fwd = UA_TRUE;
                            nrrr.references.push_back(ReferenceDescriptionGen(start_ref_type_id, start_is_fwd));
                        }
                        else
                        {
                            // During generation, it is possible that identical references will be generated for the same node, this will make it possible to check the formation of Alias,
                            // since aliases can only be generated from one instance of the type.
                            nrrr.references.push_back(ReferenceDescriptionGen(start_ref_type_id, start_is_fwd));
                        }
                    }

                    // I fill in the list of references for comparison, excluding ignored nodes
                    std::vector<UATypesContainer<UA_ReferenceDescription>> new_references;
                    const auto has_subtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);

                    // The algorithm uses a mechanism for adding a reference to the base object, so the same reference must be added to the starting node of test references (test data).
                    if (UA_ExpandedNodeId_equal(&nrrr.exp_node_id.GetRef(), &nodes_id[0].GetRef()))
                    {
                        const auto start_node_object = UATypesContainer<UA_ExpandedNodeId>(parent_ref, UA_TYPES_EXPANDEDNODEID);
                        UATypesContainer<UA_ReferenceDescription> insertion_ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
                        insertion_ref_desc.GetRef().isForward = false;
                        auto organize_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                        UA_NodeId_copy(&organize_node_id, &insertion_ref_desc.GetRef().referenceTypeId);
                        UA_NodeId_copy(&start_node_object.GetRef().nodeId, &insertion_ref_desc.GetRef().nodeId.nodeId);
                        new_references.push_back(std::move(insertion_ref_desc));
                    }

                    for (auto& reference : nrrr.references)
                    {
                        const auto ch_node_id = UATypesContainer<UA_ExpandedNodeId>(reference.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
                        if (valid_node_id.contains(ch_node_id))
                        {
                            // If the nodes are of any (Instance) class, then I add all the references
                            if (!types_nodeclasses.contains(current_node_class))
                            {
                                new_references.push_back(reference);
                            }
                            // If the nodes are of the TYPE class, then only direct nodes, since the back references will be removed by the algorithm.
                            else
                            {
                                if (reference.GetRef().isForward || UA_NodeId_equal(&reference.GetRef().referenceTypeId, &has_subtype))
                                {
                                    new_references.push_back(reference);
                                }
                            }
                        }
                    }

                    // This reference should not be included in the comparison list, because the reference will be changed by the exporter and will not be found by the test, which will cause an error.
                    // Therefore, I place the code behind filling new_references.
                    if ( // I'm inserting an incorrect HasTypeDefinition reference. KEP insertion test. Only for node with identifier = KEPDefinitionProblem
                        nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && (UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &change_definition_browse_path_node_str))) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        auto reference_hastypedef_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_hastypedef_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE); // i=62 [BaseVariableType]
                        ref_desc_cnt.GetRef().isForward = true;
                        nrrr.references.push_back(ref_desc_cnt);
                    }

                    // Additionally, I add BrowsePath nodes to the list to compare what should happen.
                    auto reference_type_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &first_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_type_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        UA_ExpandedNodeId_copy(&parent_start_node_replacer.GetRef(), &ref_desc_cnt.GetRef().nodeId); // i=85
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                    }

                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &second_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_type_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        UA_ExpandedNodeId_copy(&ua_node_first_browse_path_node, &ref_desc_cnt.GetRef().nodeId); // ns=2;s=BrowsePathParent
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                    }

                    // For KEP insertion testing, I expect the reference with HasTypeDefinition = i=62 [BaseVariableType] to be replaced with HasTypeDefinition = i=63 [BaseDataVariableType]
                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &change_definition_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        ref_desc_cnt.GetRef().referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
                        ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE); // i=63 [BaseDataVariableType]
                        ref_desc_cnt.GetRef().isForward = true;
                        new_references.push_back(ref_desc_cnt);
                    }

                    // To test reference of type HasSubtype, I add back references of this type, since other references in the TYPE classes will be removed
                    if (types_nodeclasses.contains(current_node_class))
                    {
                        MESSAGE("ADD SUBTYPE TO NODE: ", nrrr.exp_node_id.ToString(), ", class: ", types_nodeclasses.at(current_node_class));
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        switch (current_node_class)
                        {
                        case UA_NODECLASS_OBJECTTYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE); // i=58 [BaseObjectType]
                            break;
                        case UA_NODECLASS_VARIABLETYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE); // i=62 [BaseVariableType]
                            break;
                        case UA_NODECLASS_REFERENCETYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES); // i=31 [References]
                            break;
                        case UA_NODECLASS_DATATYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE); // i=24 [BaseDataType]
                            break;
                        default:
                            FAIL(true);
                        }
                        ref_desc_cnt.GetRef().referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                        nrrr.references.push_back(ref_desc_cnt);
                    }

                    cmp_ref_descr.insert({nrrr.exp_node_id, new_references});
                })
                .RETURN(StatusResults::Good)
                .IN_SEQUENCE(seq);

            REQUIRE_CALL(encoder, AddAliases(_)).WITH(_1.empty() == false).RETURN(StatusResults::Good).IN_SEQUENCE(seq);
            REQUIRE_CALL(encoder, End()).RETURN(StatusResults::Good).IN_SEQUENCE(seq);

            NodesetExporterLoop
                exporter_loop(std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>{{nodes_id[0].ToString(), nodes_id}}, open, encoder, logger, parent_start_node_replacer, false);
            StatusResults status_result = StatusResults::Fail;
            CHECK_NOTHROW(status_result = exporter_loop.StartExport());
            // The number of nodes suitable for export should be equal to the number of nodes that will actually be exported
            REQUIRE_EQ(number_of_valid_class_nodes_to_export, number_of_add_nodes_to_export);
            CHECK_EQ(status_result, StatusResults::Good);
            MESSAGE("Number of nodes: ", nodes_id.size(), ", number of nodes to be exported under incoming classes: ", number_of_add_nodes_to_export);
        }

        SUBCASE("Core test with a limit on a single data request")
        {
            REQUIRE_CALL(open, ReadNodeClasses(_))
                .WITH(_1.empty() == false)
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeClassesRequestResponse& ncs
                                     : _1) { ncs.node_class = nodes_with_class.at(ncs.exp_node_id); })
                .RETURN(StatusResults::Good)
                .TIMES(AT_LEAST(2)); // This method must be called more than one time in a loop with a set number of nodes

            REQUIRE_CALL(open, ReadNodesAttributes(_))
                .WITH(_1.empty() == false)
                .SIDE_EFFECT(for (MockOpen62541::NodeAttributesRequestResponse& narr
                                  : _1) {
                    MESSAGE("NodeAttributesRequestResponse nodeID: ", narr.exp_node_id.ToString());
                    for (auto& attr : narr.attrs)
                    {
                        try
                        {
                            MESSAGE("Attr: ", attr.first, ", data: ", VariantsOfAttrToString(AttributesGen(attr.first)));
                            attr.second.emplace(std::move(AttributesGen(attr.first)));
                        }
                        catch (std::exception& exc)
                        {
                            MESSAGE(exc.what());
                            EXIT_FAILURE;
                        }
                    }
                })
                .RETURN(StatusResults::Good)
                .TIMES(AT_LEAST(2)); // This method must be called more than one time in a loop with a set number of nodes

            REQUIRE_CALL(open, ReadNodeReferences(_))
                .WITH(_1.empty() == false)
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeReferencesRequestResponse& nrrr
                                     : _1) {
                    size_t count = GetRandomNumber(50, 100);
                    UA_Int64 start_ref_type_id = UA_NS0ID_HIERARCHICALREFERENCES;
                    UA_Boolean start_is_fwd = UA_FALSE;
                    const auto current_node_class = nodes_with_class.at(nrrr.exp_node_id);
                    auto first_browse_path_node_str = UA_STRING(first_browse_path_node.data());
                    auto second_browse_path_node_str = UA_STRING(second_browse_path_node.data());
                    auto change_definition_browse_path_node_str = UA_STRING(change_definition_browse_path_node.data());
                    for (size_t index = 0; index < count; index++)
                    {
                        // For some nodes, you only need to generate direct references. BrowsePath validation test.
                        if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                            && (UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &first_browse_path_node_str) // NOLINT(cppcoreguidelines-pro-type-union-access)
                                || UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &second_browse_path_node_str))) // NOLINT(cppcoreguidelines-pro-type-union-access)
                        {
                            start_is_fwd = UA_TRUE;
                            nrrr.references.push_back(ReferenceDescriptionGen(start_ref_type_id, start_is_fwd));
                        }
                        else
                        {
                            // During generation, it is possible that identical references will be generated for the same node, this will make it possible to check the formation of Alias,
                            // since aliases can only be generated from one instance of the type.
                            nrrr.references.push_back(ReferenceDescriptionGen(start_ref_type_id, start_is_fwd));
                        }
                    }

                    // Populate the list of references to compare, excluding ignored nodes
                    std::vector<UATypesContainer<UA_ReferenceDescription>> new_references;

                    // The algorithm uses a mechanism for adding a reference to the base object, so the same reference must be added to the starting node of test references (test data).
                    if (UA_ExpandedNodeId_equal(&nrrr.exp_node_id.GetRef(), &nodes_id[0].GetRef()))
                    {
                        const auto start_node_object = UATypesContainer<UA_ExpandedNodeId>(parent_ref, UA_TYPES_EXPANDEDNODEID);
                        UATypesContainer<UA_ReferenceDescription> insertion_ref_desc(UA_TYPES_REFERENCEDESCRIPTION);
                        insertion_ref_desc.GetRef().isForward = false;
                        auto organize_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                        UA_NodeId_copy(&organize_node_id, &insertion_ref_desc.GetRef().referenceTypeId);
                        UA_NodeId_copy(&start_node_object.GetRef().nodeId, &insertion_ref_desc.GetRef().nodeId.nodeId);
                        new_references.push_back(std::move(insertion_ref_desc));
                    }

                    const auto has_subtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
                    for (auto& reference : nrrr.references)
                    {
                        const auto ch_node_id = UATypesContainer<UA_ExpandedNodeId>(reference.GetRef().nodeId, UA_TYPES_EXPANDEDNODEID);
                        if (valid_node_id.contains(ch_node_id))
                        {
                            // If the nodes are of any (Instance) class, then I add all the references
                            if (!types_nodeclasses.contains(current_node_class))
                            {
                                new_references.push_back(reference);
                            }
                            // If the nodes are of the TYPE class, then only direct nodes, since the back references will be removed by the algorithm.
                            else
                            {
                                if (reference.GetRef().isForward || UA_NodeId_equal(&reference.GetRef().referenceTypeId, &has_subtype))
                                {
                                    new_references.push_back(reference);
                                }
                            }
                        }
                    }

                    // This reference should not be included in the comparison list, because the reference will be changed by the exporter and will not be found by the test,
                    // which will cause an error. Therefore, I place the code behind filling new_references.
                    if ( // Inserting an incorrect HasTypeDefinition reference. KEP insertion test. Only for node with identifier = KEPDefinitionProblem
                        nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && (UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &change_definition_browse_path_node_str))) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        auto reference_hastypedef_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_hastypedef_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE); // i=62 [BaseVariableType]
                        ref_desc_cnt.GetRef().isForward = true;
                        nrrr.references.push_back(ref_desc_cnt);
                    }

                    // Additionally, you need to add the BrowsePath nodes to the list to compare what should happen.
                    auto reference_type_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &first_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_type_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        UA_ExpandedNodeId_copy(&parent_start_node_replacer.GetRef(), &ref_desc_cnt.GetRef().nodeId); // i=85
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                    }

                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &second_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_type_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        UA_ExpandedNodeId_copy(&ua_node_first_browse_path_node, &ref_desc_cnt.GetRef().nodeId); // ns=2;s=BrowsePathParent
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                    }

                    // For KEP insertion testing, Expect the reference with HasTypeDefinition = i=62 [BaseVariableType] to be replaced with HasTypeDefinition = i=63 [BaseDataVariableType]
                    if (nrrr.exp_node_id.GetRef().nodeId.identifierType == UA_NodeIdType::UA_NODEIDTYPE_STRING
                        && UA_String_equal(&nrrr.exp_node_id.GetRef().nodeId.identifier.string, &change_definition_browse_path_node_str)) // NOLINT(cppcoreguidelines-pro-type-union-access)
                    {
                        auto reference_hastypedef_id = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        UA_NodeId_copy(&reference_hastypedef_id, &ref_desc_cnt.GetRef().referenceTypeId);
                        ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE); // i=63 [BaseDataVariableType]
                        ref_desc_cnt.GetRef().isForward = true;
                        new_references.push_back(ref_desc_cnt);
                    }

                    // To test references of type HasSubtype, I add back reference of this type, since other references in the TYPE classes will be removed
                    if (types_nodeclasses.contains(current_node_class))
                    {
                        MESSAGE("ADD SUBTYPE TO NODE: ", nrrr.exp_node_id.ToString(), ", class: ", types_nodeclasses.at(current_node_class));
                        UATypesContainer<UA_ReferenceDescription> ref_desc_cnt(UA_TYPES_REFERENCEDESCRIPTION);
                        switch (current_node_class)
                        {
                        case UA_NODECLASS_OBJECTTYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE); // i=58 [BaseObjectType]
                            break;
                        case UA_NODECLASS_VARIABLETYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE); // i=62 [BaseVariableType]
                            break;
                        case UA_NODECLASS_REFERENCETYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES); // i=31 [References]
                            break;
                        case UA_NODECLASS_DATATYPE:
                            ref_desc_cnt.GetRef().nodeId = UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE); // i=24 [BaseDataType]
                            break;
                        default:
                            FAIL(true);
                        }
                        ref_desc_cnt.GetRef().referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
                        ref_desc_cnt.GetRef().isForward = false;
                        new_references.push_back(ref_desc_cnt);
                        nrrr.references.push_back(ref_desc_cnt);
                    }

                    cmp_ref_descr.insert({nrrr.exp_node_id, new_references});
                })
                .RETURN(StatusResults::Good)
                .TIMES(AT_LEAST(2)); // This method must be called more than one time in a loop with a set number of nodes

            REQUIRE_CALL(encoder, AddAliases(_)).WITH(_1.empty() == false).RETURN(StatusResults::Good).IN_SEQUENCE(seq);
            REQUIRE_CALL(encoder, End()).RETURN(StatusResults::Good).IN_SEQUENCE(seq);

            NodesetExporterLoop
                exporter_loop(std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>{{nodes_id[0].ToString(), nodes_id}}, open, encoder, logger, parent_start_node_replacer, false);
            exporter_loop.SetNumberOfMaxNodesToRequestData(max_nodes_to_request_data);
            StatusResults status_result = StatusResults::Fail;
            CHECK_NOTHROW(status_result = exporter_loop.StartExport());
            // The number of nodes suitable for export should be equal to the number of nodes that will actually be exported
            REQUIRE_EQ(number_of_valid_class_nodes_to_export, number_of_add_nodes_to_export);
            CHECK_EQ(status_result, StatusResults::Good);
            MESSAGE("Number of nodes: ", nodes_id.size(), ", number of nodes to be exported under incoming classes: ", number_of_add_nodes_to_export);
        }
    }
}