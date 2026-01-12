//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//


#include "nodesetexporter/NodesetExporterLoop.h"
#include "LogMacro.h"
#include "nodesetexporter/common/v_1.3_Compatibility.h"
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
using StatusResults = nodesetexporter::common::statuses::StatusResults<>;
using nodesetexporter::open62541::typealiases::MultidimensionalArray;

namespace
{
TEST_LOGGER_INIT

using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LogLevel = nodesetexporter::common::LogLevel;

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
    IMPLEMENT_MOCK2(ReadNodesAttributes);
    IMPLEMENT_MOCK2(ReadNodeDataValue);
    IMPLEMENT_MOCK1(SetRequestedMaxReferencesPerNode);
};

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

// The structure of a complete description of the node without nodeid with additional assistance methods
struct NodeDescription
{
    UA_NodeClass node_class = UA_NodeClass::__UA_NODECLASS_FORCE32BIT; // Класс узла

    class Attributes // The attributes of the node
    {
    public:
        enum class DefenitionTypes : u_int8_t
        {
            Enum,
            Structure
        };

        void SetBrowseName(u_int16_t ns, std::string browse_name)
        {
            m_browse_name = UATypesContainer<UA_QualifiedName>{UA_QUALIFIEDNAME(ns, browse_name.data()), UA_TYPES_QUALIFIEDNAME};
        }
        void SetDisplayName(std::string locale, std::string display_name)
        {
            m_display_name = UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT(locale.data(), display_name.data()), UA_TYPES_LOCALIZEDTEXT};
        }
        void SetDescription(std::string locale, std::string descr)
        {
            m_description = UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT(locale.data(), descr.data()), UA_TYPES_LOCALIZEDTEXT};
        }
        void SetEventNotifier(uint8_t event)
        {
            m_eventnotifier = event;
        }
        void SetIsAbstract(bool is_abstr)
        {
            m_is_abstract = is_abstr;
        }
        void SetDataType(const std::string& node_id)
        {
            const auto node_id_tmp = UA_NODEID(node_id.data());
            m_data_type = UATypesContainer<UA_NodeId>{node_id_tmp, UA_TYPES_NODEID};
        }
        void SetValueRank(int32_t val_rank)
        {
            m_value_rank = val_rank;
        }
        void SetArrayDimmension(std::vector<uint32_t>&& arr_dim)
        {
            m_array_dimension.SetArray({}, std::move(arr_dim));
        }
        void SetValueScalar(int64_t value)
        {
            auto* some_scalar = UA_Int64_new();
            *some_scalar = static_cast<UA_Int64>(value);
            UA_Variant_setScalar(&scalar_value.GetRef(), some_scalar, &UA_TYPES[UA_TYPES_INT64]);
        }

        void SetValueScalar(std::string text)
        {
            auto some_text = UA_STRING_ALLOC(text.data());
            UA_Variant_setScalarCopy(&scalar_value.GetRef(), &some_text, &UA_TYPES[UA_TYPES_STRING]);
            UA_String_clear(&some_text);
        }

        void SetAccessLevel(uint8_t acc_level)
        {
            m_access_level = acc_level;
        }
        void SetUserAccessLevel(uint8_t user_access_level)
        {
            m_user_access_level = user_access_level;
        }
        void SetMinimumSamplingInterval(double minimum_sampling_interval)
        {
            m_minimum_sampling_interval = minimum_sampling_interval;
        }
        void SetHistorizing(bool histor)
        {
            m_historizing = histor;
        }
        void SetInverseName(std::string locale, std::string name)
        {
            m_inverse_name = UATypesContainer<UA_LocalizedText>{UA_LOCALIZEDTEXT(locale.data(), name.data()), UA_TYPES_LOCALIZEDTEXT};
        }
        void SetDataTypeDef(DefenitionTypes def_types, int64_t scalar)
        {
            switch (def_types)
            {
            case DefenitionTypes::Enum:
            {
                auto enum_def = UATypesContainer<UA_EnumDefinition>(UA_TYPES_ENUMDEFINITION);
                enum_def.GetRef().fieldsSize = 1;
                auto enum_f = UATypesContainer<UA_EnumField>(UA_TYPES_ENUMFIELD);
                enum_def.GetRef().fields = &enum_f.GetRef();
                const UA_String name = UA_STRING_ALLOC("Test enum");
                UA_String_copy(&name, &enum_def.GetRef().fields->name);
                enum_def.GetRef().fields->value = scalar;
                UA_Variant_setScalar(&scalar_value.GetRef(), &enum_def.GetRef(), &UA_TYPES[UA_TYPES_ENUMDEFINITION]);
            }
            break;
            case DefenitionTypes::Structure:
                auto str_def = UATypesContainer<UA_StructureDefinition>(UA_TYPES_STRUCTUREDEFINITION);
                str_def.GetRef().structureType = UA_StructureType::UA_STRUCTURETYPE_STRUCTURE;
                str_def.GetRef().fieldsSize = 1;
                str_def.GetRef().baseDataType = UA_NODEID_NUMERIC(0, 8);
                auto str_f = UATypesContainer<UA_StructureField>(UA_TYPES_STRUCTUREFIELD);
                str_def.GetRef().fields = &str_f.GetRef();
                const UA_String name = UA_STRING_ALLOC("Test struct");
                UA_String_copy(&name, &str_def.GetRef().fields->name);
                UA_Variant_setScalar(&scalar_value.GetRef(), &str_def.GetRef(), &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
                break;
            }
        }
        void SetWriteMask(uint32_t write_mask)
        {
            m_write_mask = write_mask;
        }
        void SetUserWriteMask(UA_UInt32 user_write_mask)
        {
            m_user_write_mask = user_write_mask;
        }
        void SetSymmetric(bool symm)
        {
            m_symmetric = symm;
        }

        [[nodiscard]] VariantsOfAttr GetWrappAttr(UA_AttributeId attr_id) const
        {
            switch (attr_id)
            {
            case UA_ATTRIBUTEID_BROWSENAME:
                return VariantsOfAttr{m_browse_name};
            case UA_ATTRIBUTEID_DISPLAYNAME:
                return VariantsOfAttr{m_display_name};
            case UA_ATTRIBUTEID_DESCRIPTION:
                return VariantsOfAttr{m_description};
            case UA_ATTRIBUTEID_EVENTNOTIFIER:
                return VariantsOfAttr{m_eventnotifier};
            case UA_ATTRIBUTEID_ISABSTRACT:
                return VariantsOfAttr{m_is_abstract};
            case UA_ATTRIBUTEID_DATATYPE:
                return VariantsOfAttr{m_data_type};
            case UA_ATTRIBUTEID_VALUERANK:
                return VariantsOfAttr{m_value_rank};
            case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
                return VariantsOfAttr{m_array_dimension};
            case UA_ATTRIBUTEID_VALUE:
                return VariantsOfAttr{scalar_value};
            case UA_ATTRIBUTEID_ACCESSLEVEL:
                return VariantsOfAttr{m_access_level};
            case UA_ATTRIBUTEID_USERACCESSLEVEL:
                return VariantsOfAttr{m_user_access_level};
            case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
                return VariantsOfAttr{m_minimum_sampling_interval};
            case UA_ATTRIBUTEID_HISTORIZING:
                return VariantsOfAttr{m_historizing};
            case UA_ATTRIBUTEID_INVERSENAME:
                return VariantsOfAttr{m_inverse_name};
            case UA_ATTRIBUTEID_DATATYPEDEFINITION:
                return VariantsOfAttr{data_type_definition};
            case UA_ATTRIBUTEID_WRITEMASK:
                return VariantsOfAttr{m_write_mask};
            case UA_ATTRIBUTEID_USERWRITEMASK:
                return VariantsOfAttr{m_user_write_mask};
            case UA_ATTRIBUTEID_SYMMETRIC:
                return VariantsOfAttr{m_symmetric};
            }
            FAIL(true);
        }

    private:
        UATypesContainer<UA_QualifiedName> m_browse_name = UATypesContainer<UA_QualifiedName>(UA_TYPES_QUALIFIEDNAME);
        UATypesContainer<UA_LocalizedText> m_display_name = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
        UATypesContainer<UA_LocalizedText> m_description = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
        UA_Byte m_eventnotifier = 0;
        UA_Boolean m_is_abstract{};
        UATypesContainer<UA_NodeId> m_data_type = UATypesContainer<UA_NodeId>(UA_TYPES_NODEID);
        UA_Int32 m_value_rank = 0;
        MultidimensionalArray<UA_UInt32> m_array_dimension;
        UATypesContainer<UA_Variant> scalar_value = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
        UA_Byte m_access_level = 0;
        UA_Byte m_user_access_level = 0;
        UA_Double m_minimum_sampling_interval = 0;
        UA_Boolean m_historizing{};
        UATypesContainer<UA_LocalizedText> m_inverse_name = UATypesContainer<UA_LocalizedText>(UA_TYPES_LOCALIZEDTEXT);
        UATypesContainer<UA_Variant> data_type_definition = UATypesContainer<UA_Variant>(UA_TYPES_VARIANT);
        UA_UInt32 m_write_mask = 0;
        UA_UInt32 m_user_write_mask = 0;
        UA_Boolean m_symmetric = false;
    } attributes;

    class References
    {
    public:
        void SetReferenceTypeId(const std::string& node_id)
        {
            const auto node_id_tmp = UA_NODEID(node_id.data());
            UA_NodeId_copy(&node_id_tmp, &m_reference.GetRef().referenceTypeId);
        }
        void SetIsForward(bool is_forward)
        {
            m_reference.GetRef().isForward = is_forward;
        }
        void SetNodeId(const std::string& node_id)
        {
            auto node_id_tmp = UA_EXPANDEDNODEID(node_id.data());
            UA_ExpandedNodeId_copy(&node_id_tmp, &m_reference.GetRef().nodeId);
            UA_ExpandedNodeId_clear(&node_id_tmp);
        }
        void SetBrowseName(u_int16_t ns, std::string browse_name)
        {
            const auto br_name = UA_QUALIFIEDNAME(ns, browse_name.data());
            UA_QualifiedName_copy(&br_name, &m_reference.GetRef().browseName);
        }
        void SetDisplayName(std::string locale, std::string display_name)
        {
            const auto disp_name = UA_LOCALIZEDTEXT(locale.data(), display_name.data());
            UA_LocalizedText_copy(&disp_name, &m_reference.GetRef().displayName);
        }
        void SetNodeClass(UA_NodeClass nd_class)
        {
            m_reference.GetRef().nodeClass = nd_class;
        }
        void SetTypeDefinition(const std::string& node_id)
        {
            const auto node_id_tmp = UA_EXPANDEDNODEID(node_id.data());
            UA_ExpandedNodeId_copy(&node_id_tmp, &m_reference.GetRef().typeDefinition);
        }

        /**
         * @brief Метод добавления ссылки в массив. Кажды раз при заполнении ссылки необходимо в конце вызвать данный метод.
         * @param is_this_ref_will_be_delete - В случае, если предполагается, что ядро должно удалить ссылку - нужно ее пометить как true.
         */
        void AddReferenceToVector(bool is_this_ref_will_be_delete = false)
        {
            m_references.push_back(std::move(m_reference));
            m_reference = UATypesContainer<UA_ReferenceDescription>(UA_TYPES_REFERENCEDESCRIPTION);
            m_list_is_this_refs_will_be_delete.push_back(is_this_ref_will_be_delete);
        };

        void SetReferences(std::vector<UATypesContainer<UA_ReferenceDescription>>&& ref)
        {
            m_references = std::move(ref);
        }

        [[nodiscard]] std::vector<UATypesContainer<UA_ReferenceDescription>> GetReferences() const
        {
            return m_references;
        }

        [[nodiscard]] std::vector<bool> GetListOfUsageRef() const
        {
            return m_list_is_this_refs_will_be_delete;
        }

    private:
        UATypesContainer<UA_ReferenceDescription> m_reference = UATypesContainer<UA_ReferenceDescription>(UA_TYPES_REFERENCEDESCRIPTION); // Ссылки узла
        std::vector<UATypesContainer<UA_ReferenceDescription>> m_references;
        std::vector<bool> m_list_is_this_refs_will_be_delete;
    } references;
};

} // namespace


TEST_SUITE("nodesetexporter")
{
    const auto parent_start_node_replacer = UATypesContainer<UA_ExpandedNodeId>(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_TYPES_EXPANDEDNODEID);

    // Classes / text names.
    const std::map<std::uint32_t, std::string> types_nodeclasses{
        {UA_NODECLASS_OBJECT, "UA_NODECLASS_OBJECT"},
        {UA_NODECLASS_VARIABLE, "UA_NODECLASS_VARIABLE"},
        {UA_NODECLASS_METHOD, "UA_NODECLASS_METHOD"},
        {UA_NODECLASS_OBJECTTYPE, "UA_NODECLASS_DATATYPE"},
        {UA_NODECLASS_VARIABLETYPE, "UA_NODECLASS_VARIABLETYPE"},
        {UA_NODECLASS_REFERENCETYPE, "UA_NODECLASS_REFERENCETYPE"},
        {UA_NODECLASS_DATATYPE, "UA_NODECLASS_DATATYPE"},
        {UA_NODECLASS_VIEW, "UA_NODECLASS_VIEW"}};

    TEST_CASE("nodesetexporter::NodesetExporterLoop") // NOLINT
    {
        using trompeloeil::_;
        using trompeloeil::eq;
        using trompeloeil::ne;
        trompeloeil::sequence seq;
        size_t number_of_valid_class_nodes_to_export = 0;
        size_t number_of_add_nodes_to_export = 0;

        auto exp_nodeid_null = UA_EXPANDEDNODEID_NULL;

        constexpr size_t namespace_array_size = 3;

        // Set namespace
        const UATypesContainer<UA_ExpandedNodeId> server_namespace_array_request(UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), UA_TYPES_EXPANDEDNODEID);
        std::vector<std::string> valid_namespaces{"http://some_opc_server/UA/", "http://some_devices/UA/"};
        auto* namespace_array = static_cast<UA_String*>(UA_Array_new(namespace_array_size, &UA_TYPES[UA_TYPES_STRING]));
        namespace_array[0] = UA_String_fromChars("http://opcfoundation.org/UA/"); // NOLINT
        namespace_array[1] = UA_String_fromChars(valid_namespaces[0].c_str()); // NOLINT
        namespace_array[2] = UA_String_fromChars(valid_namespaces[1].c_str()); // NOLINT

        // Prepare test nodes
        std::vector<UATypesContainer<UA_ExpandedNodeId>> nodes_ids; // Передаются на вход ядру.
        std::map<UATypesContainer<UA_ExpandedNodeId>, NodeDescription> nodes_description; // Используются для заполнения описания узлов
        std::map<UATypesContainer<UA_ExpandedNodeId>, NodeDescription> cmp_ref_descr; // Выходная тестовая сравнительная модель структуры узлов с учетом всех изменений, производимых ядром.
        std::set<UATypesContainer<UA_ExpandedNodeId>> valid_node_id; // Валидные для работы node_id, используются для фильтрации тестовых ссылок.

        // Adding nodes
        // NODE ns=2;i=100 - Root node tied to Objects
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_NUMERIC(2, 100), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        auto node_desc = std::make_unique<NodeDescription>();
        node_desc->node_class = UA_NODECLASS_OBJECT;
        node_desc->attributes.SetBrowseName(1, "vPLC1");
        node_desc->attributes.SetDisplayName("en", "vPLC1");
        node_desc->attributes.SetDescription("en", "Description vPLC1");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=61");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "FolderType");
        node_desc->references.SetBrowseName(0, "FolderType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("i=85");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=35");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "Objects");
        node_desc->references.SetBrowseName(0, "Objects");
        node_desc->references.AddReferenceToVector();
        // Ref 3 - Forward reference
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector();
        // Ref 4 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2");
        node_desc->references.AddReferenceToVector();
        // Ref 5 - Forward reference to the ignored type of View, respectively, will be removed by the core
        node_desc->references.SetNodeId("ns=2;g=879317f1-992b-4d94-841d-bc7510b3922d");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VIEW);
        node_desc->references.SetDisplayName("en", "View test");
        node_desc->references.SetBrowseName(0, "View test");
        node_desc->references.AddReferenceToVector(true);
        // Ref 6 - Forward reference
// todo In some of early version 1.3.x library has a problem with bytestring.
//  Need to find this version, but now - just change node type if we use 1.3.x.
#ifdef OPEN62541_VER_1_4
        node_desc->references.SetNodeId("ns=4;b=Qnl0ZVN0cmluZ05vZGU=");
#elif defined(OPEN62541_VER_1_3)
        node_desc->references.SetNodeId("ns=4;s=ByteStringNode");
#endif
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "byte string");
        node_desc->references.SetBrowseName(0, "byte string");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;
        // Ref 7 - Direct reference to a node with incorrect HasTypeDefinition references
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "HasTypeDefinition fix test");
        node_desc->references.SetBrowseName(0, "HasTypeDefinition fix test");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=4;b=Qnl0ZVN0cmluZ05vZGU= - A variable node tied to the node of the object (root)
        node_desc = std::make_unique<NodeDescription>();
#ifdef OPEN62541_VER_1_4
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_BYTESTRING(4, "ByteStringNode"), UA_TYPES_EXPANDEDNODEID);
#elif defined(OPEN62541_VER_1_3)
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(4, "ByteStringNode"), UA_TYPES_EXPANDEDNODEID);
#endif
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "byte string");
        node_desc->attributes.SetDisplayName("en", "byte string");
        node_desc->attributes.SetDescription("en", "Description byte string");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=11");
        node_desc->attributes.SetValueScalar(133);
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(100);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=100");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1");
        node_desc->references.SetBrowseName(0, "vPLC1");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;i=200 - A variable node tied to the node of the object (root)
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_NUMERIC(2, 200), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "temperature");
        node_desc->attributes.SetDisplayName("en", "temperature");
        node_desc->attributes.SetDescription("en", "Description temperature");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=11");
        node_desc->attributes.SetValueScalar(133);
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(100);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=100");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1");
        node_desc->references.SetBrowseName(0, "vPLC1");
        node_desc->references.AddReferenceToVector();
        // Ref3 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "Test node 2");
        node_desc->references.SetBrowseName(0, "Test node 2");
        node_desc->references.AddReferenceToVector();
        // Ref4 Forward reference on the ignored type method
        node_desc->references.SetNodeId("ns=3;i=300");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_METHOD);
        node_desc->references.SetDisplayName("en", "method");
        node_desc->references.SetBrowseName(0, "method");
        node_desc->references.AddReferenceToVector(true);
        // Ref5 - Forward reference
        node_desc->references.SetNodeId("ns=5;i=1299");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "pressure");
        node_desc->references.SetBrowseName(0, "pressure");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=3;i=300 --- Ignored node method
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_NUMERIC(3, 300), UA_TYPES_EXPANDEDNODEID);
        node_desc->node_class = UA_NODECLASS_METHOD;
        node_desc->attributes.SetBrowseName(1, "method");
        node_desc->attributes.SetDisplayName("en", "method");
        node_desc->attributes.SetDescription("en", "Description method");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        // Ref1 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector(true);
        // Ref2 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=Test node 1");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "Test node 1");
        node_desc->references.SetBrowseName(0, "Test node 1");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=5;i=1299 - The node is tied to another node - variable
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_NUMERIC(5, 1299), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "pressure");
        node_desc->attributes.SetDisplayName("en", "pressure");
        node_desc->attributes.SetDescription("en", "Description pressure");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=11");
        node_desc->attributes.SetValueScalar(133);
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(5999);
        node_desc->attributes.SetHistorizing(true);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;s=Test node 1 - The node tied to Method - should also be ignored
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "Test_node_1"), UA_TYPES_EXPANDEDNODEID);
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "Test node 1");
        node_desc->attributes.SetDisplayName("en", "Test node 1");
        node_desc->attributes.SetDescription("en", "Description Test node 1");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueScalar("Test message");
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(100);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector(true);
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("ns=3;i=300");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_METHOD);
        node_desc->references.SetDisplayName("en", "method");
        node_desc->references.SetBrowseName(0, "method");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;s=vPLC1.Test_node_2 - The node refers to 2 parental nodes, and the first will be the main
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "Test node 2");
        node_desc->attributes.SetDisplayName("en", "Test node 2");
        node_desc->attributes.SetDescription("en", "Description Test node 2");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueScalar("Test node 2 text");
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(200);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=100");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1");
        node_desc->references.SetBrowseName(0, "vPLC1");
        node_desc->references.AddReferenceToVector();
        // Ref3 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector();
        // Ref 4 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2._Statistics");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2._Statistics");
        node_desc->references.AddReferenceToVector();
        // Ref 5 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2._Statistics");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2._Statistics");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2._Statistics");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=5;s=Тестовый_узел_5 - Node type Datatype
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(5, "Тестовый_узел_5"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_DATATYPE;
        node_desc->attributes.SetBrowseName(1, "Union");
        node_desc->attributes.SetDisplayName("en", "Union");
        node_desc->attributes.SetDescription("en", "Description Union");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(true);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=22");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=45");
        node_desc->references.SetNodeClass(UA_NODECLASS_DATATYPE);
        node_desc->references.SetDisplayName("en", "Structure");
        node_desc->references.SetBrowseName(0, "Structure");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference - In the types, it will have to be removed by the nucleus.
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;g=d8597619-5361-47c3-b81f-26b46a5b47fb - ReferenceType type node
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING_GUID(2, UA_GUID("d8597619-5361-47c3-b81f-26b46a5b47fb")), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_REFERENCETYPE;
        node_desc->attributes.SetBrowseName(1, "SomeReferences_test");
        node_desc->attributes.SetDisplayName("en", "SomeReferences_test");
        node_desc->attributes.SetDescription("en", "Description SomeReferences_test");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        node_desc->attributes.SetSymmetric(true);
        node_desc->attributes.SetInverseName("en", "InvName1");
        // Ref1 - Type
        node_desc->references.SetNodeId("i=33");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=45");
        node_desc->references.SetNodeClass(UA_NODECLASS_REFERENCETYPE);
        node_desc->references.SetDisplayName("en", "HierarchicalReferences");
        node_desc->references.SetBrowseName(0, "HierarchicalReferences");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference - In the types, it will have to be removed by the nucleus.
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=3;g=3c74ac5c-f29e-497b-900c-6dce9381b688 - Node type VariableType
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING_GUID(3, UA_GUID("3c74ac5c-f29e-497b-900c-6dce9381b688")), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLETYPE;
        node_desc->attributes.SetBrowseName(1, "DataTypeDescriptionType_test");
        node_desc->attributes.SetDisplayName("en", "DataTypeDescriptionType_test");
        node_desc->attributes.SetDescription("en", "Description DataTypeDescriptionType_test");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueRank(UA_VALUERANK_ANY);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=45");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;
        // Ref2 - Forward reference
        node_desc->references.SetNodeId("i=104");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=46");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "HasTypeDefinition");
        node_desc->references.SetBrowseName(0, "HasTypeDefinition");
        node_desc->references.AddReferenceToVector();
        // Ref3 - Forward reference
        node_desc->references.SetNodeId("i=105");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=46");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "DictionaryFragment");
        node_desc->references.SetBrowseName(0, "DictionaryFragment");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference - In the types, it will have to be removed by the nucleus.
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=4;g=23be18e1-49ab-4eb0-b06e-ded5696289a9 - OBJECTTYPE Node
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING_GUID(4, UA_GUID("23be18e1-49ab-4eb0-b06e-ded5696289a9")), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_OBJECTTYPE;
        node_desc->attributes.SetBrowseName(1, "FolderType_test");
        node_desc->attributes.SetDisplayName("en", "FolderType_test");
        node_desc->attributes.SetDescription("en", "Description FolderType_test");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=58");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=45");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseObjectType");
        node_desc->references.SetBrowseName(0, "BaseObjectType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Reverse reference - In the types, it will have to be removed by the nucleus.
        node_desc->references.SetNodeId("ns=2;i=200");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "temperature");
        node_desc->references.SetBrowseName(0, "temperature");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;g=879317f1-992b-4d94-841d-bc7510b3922d - VIEW must be ignored
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING_GUID(2, UA_GUID("879317f1-992b-4d94-841d-bc7510b3922d")), UA_TYPES_EXPANDEDNODEID); // Игнорируемый
        node_desc->node_class = UA_NODECLASS_VIEW;
        node_desc->attributes.SetBrowseName(1, "View test");
        node_desc->attributes.SetDisplayName("en", "View test");
        node_desc->attributes.SetDescription("en", "Description View test");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        // Ref1 - Reverse reference
        node_desc->references.SetNodeId("ns=2;i=100");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1");
        node_desc->references.SetBrowseName(0, "vPLC1");
        node_desc->references.AddReferenceToVector(true);
        nodes_description[nodes_ids.back()] = *node_desc;


        // --- private cases of processing ----
        // node vplc1.test_node_2.browSepathtest - for some nodes you need to generate only forward references. BrowsePath test. Works only with string NodeID
        // VPLC1, although it is a numerical NodeId, it does not matter, since the parent will be selected as vplc1.test_node_2, and this is a text NodeID.
        // The space of the names should also coincide.
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_OBJECT;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2.BrowsePathTest");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2.BrowsePathTest");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=61");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "FolderType");
        node_desc->references.SetBrowseName(0, "FolderType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=46");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->references.AddReferenceToVector();
        // Ref3 - Forward reference
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=46");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // Node vPLC1.Test_node_2.BrowsePathTest.Browse2 - BrowsePath test.
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.Browse2"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueScalar("Test node 2 text");
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(200);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // Test test of ignoring KEP system nodes, where nodeid begins with the symbol "_" + generation of reverse reference.
        // Node ns=2;s=vPLC1.Test_node_2._Statistics
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "ns=2;s=vPLC1.Test_node_2._Statistics"), UA_TYPES_EXPANDEDNODEID);
        node_desc->node_class = UA_NODECLASS_OBJECT;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2._Statistics");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2._Statistics");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2._Statistics");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=61");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "FolderType");
        node_desc->references.SetBrowseName(0, "FolderType");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // Node vPLC1.Test_node_2.BrowsePathTest.Browse3 - Verification of the node with which will be HasTypeDefenition = i=62 [Basevariabletype]
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.Browse3"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueScalar("Test node 2 text");
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(200);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - Type
        node_desc->references.SetNodeId("i=62");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseVariableType");
        node_desc->references.SetBrowseName(0, "BaseVariableType");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // Node ns=2;s=vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix - check for a node that will have backlinks of type HasTypeDefinition and there will be more than one of them
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_OBJECT;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetIsAbstract(false);
        // Ref1 - type
        node_desc->references.SetNodeId("i=61");
        node_desc->references.SetIsForward(false); // У такой ссылки всегда должен быть True
        node_desc->references.SetReferenceTypeId("i=40"); // HasTypeDefinition
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "FolderType");
        node_desc->references.SetBrowseName(0, "FolderType");
        node_desc->references.AddReferenceToVector();
        // Ref2 - type (which should not exist. There should always be one type)
        node_desc->references.SetNodeId("vPLC1.Test_node_2.BrowsePathTest.Browse2");
        node_desc->references.SetIsForward(false); // У такой ссылки всегда должен быть True
        node_desc->references.SetReferenceTypeId("i=40"); // HasTypeDefinition
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "Incorrect");
        node_desc->references.SetBrowseName(0, "Incorrect");
        node_desc->references.AddReferenceToVector(true);
        // Ref3 - type (which should not exist. There should always be one type)
        node_desc->references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest.Browse3");
        node_desc->references.SetIsForward(false); // У такой ссылки всегда должен быть True
        node_desc->references.SetReferenceTypeId("i=40"); // HasTypeDefinition
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "Incorrect");
        node_desc->references.SetBrowseName(0, "Incorrect");
        node_desc->references.AddReferenceToVector(true);
        // Ref4 - type (which should not exist. There should always be one type)
#ifdef OPEN62541_VER_1_4
        node_desc->references.SetNodeId("ns=4;b=Qnl0ZVN0cmluZ05vZGU=");
#elif defined(OPEN62541_VER_1_3)
        node_desc->references.SetNodeId("ns=4;s=ByteStringNode");
#endif
        node_desc->references.SetIsForward(false); // У такой ссылки всегда должен быть True
        node_desc->references.SetReferenceTypeId("i=40"); // HasTypeDefinition
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECTTYPE);
        node_desc->references.SetDisplayName("en", "Incorrect");
        node_desc->references.SetBrowseName(0, "Incorrect");
        node_desc->references.AddReferenceToVector(true);
        // Ref 5 - Backlink to parent node
        node_desc->references.SetNodeId("ns=2;i=100");
        node_desc->references.SetIsForward(false);
        node_desc->references.SetReferenceTypeId("i=47");
        node_desc->references.SetNodeClass(UA_NODECLASS_OBJECT);
        node_desc->references.SetDisplayName("en", "vPLC1");
        node_desc->references.SetBrowseName(0, "vPLC1");
        node_desc->references.AddReferenceToVector();
        // Ref5 - Direct link
        node_desc->references.SetNodeId("vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->references.SetIsForward(true);
        node_desc->references.SetReferenceTypeId("i=46");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLE);
        node_desc->references.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->references.SetBrowseName(0, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        // NODE ns=2;s=vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable - Node with only a back reference of type HasTypeDefinition
        node_desc = std::make_unique<NodeDescription>();
        nodes_ids.emplace_back(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable"), UA_TYPES_EXPANDEDNODEID);
        valid_node_id.insert(nodes_ids.back());
        node_desc->node_class = UA_NODECLASS_VARIABLE;
        node_desc->attributes.SetBrowseName(1, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->attributes.SetDisplayName("en", "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->attributes.SetDescription("en", "Description vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable");
        node_desc->attributes.SetAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
        node_desc->attributes.SetUserAccessLevel(UA_ACCESSLEVELMASK_READ);
        node_desc->attributes.SetWriteMask(UA_WRITEMASK_ACCESSLEVELEX);
        node_desc->attributes.SetUserWriteMask(UA_WRITEMASK_USERACCESSLEVEL);
        node_desc->attributes.SetEventNotifier(UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
        node_desc->attributes.SetDataType("i=12");
        node_desc->attributes.SetValueScalar("Test message");
        node_desc->attributes.SetValueRank(UA_VALUERANK_SCALAR);
        node_desc->attributes.SetArrayDimmension(std::vector<uint32_t>());
        node_desc->attributes.SetMinimumSamplingInterval(100);
        node_desc->attributes.SetHistorizing(false);
        // Ref1 - type
        node_desc->references.SetNodeId("i=63");
        node_desc->references.SetIsForward(false); // Such a link must always be True
        node_desc->references.SetReferenceTypeId("i=40");
        node_desc->references.SetNodeClass(UA_NODECLASS_VARIABLETYPE);
        node_desc->references.SetDisplayName("en", "BaseDataVariableType");
        node_desc->references.SetBrowseName(0, "BaseDataVariableType");
        node_desc->references.AddReferenceToVector();
        nodes_description[nodes_ids.back()] = *node_desc;

        number_of_valid_class_nodes_to_export = valid_node_id.size();

        // I add expected changes at the output to the model
        cmp_ref_descr = nodes_description;
        // Removing links and elements that we will not get at the output
        for (auto& node : cmp_ref_descr)
        {
            std::vector<UATypesContainer<UA_ReferenceDescription>> new_refs;
            for (size_t index = 0; index < node.second.references.GetReferences().size(); ++index)
            {
                if (!node.second.references.GetListOfUsageRef()[index])
                {
                    new_refs.push_back(node.second.references.GetReferences()[index]);
                }
            }
            // I replace the old list with a new one with "remote" references, which are assumed that the exporter's core will remove
            node.second.references.SetReferences(std::move(new_refs));

            // For nodes in which there are no reverse references and they are added through BrowsePath, you need to add such references manually to the test
            // sample, as the Will Appear at the output of the core.
            // In the process, the algorithm in the GetNodesData method will find that there are no reverse references and add its option based on forming from BrowsePath.
            const UATypesContainer<UA_ExpandedNodeId> br_path_test1(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest"), UA_TYPES_EXPANDEDNODEID);
            const UATypesContainer<UA_ExpandedNodeId> br_path_test2(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.Browse2"), UA_TYPES_EXPANDEDNODEID);
            // For the node vplc1.test_node_2.BrowsePathTest.browse3, you need to replace in the link type HasTypeDefinition = i=62 [Basevariabletype] on i=63
            const UATypesContainer<UA_ExpandedNodeId> br_path_test3(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.Browse3"), UA_TYPES_EXPANDEDNODEID);
            // For the vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix node, you need to replace the HasTypeDefinition backlink with a regular one
            const UATypesContainer<UA_ExpandedNodeId> br_path_test4(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix"), UA_TYPES_EXPANDEDNODEID);
            // For the vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable node, you need to replace the HasTypeDefinition backlink with a regular one and add a backlink to the parent
            const UATypesContainer<UA_ExpandedNodeId> br_path_test5(UA_EXPANDEDNODEID_STRING(2, "vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix.Variable"), UA_TYPES_EXPANDEDNODEID);

            if (node.first == br_path_test1)
            {
                // As, how is the simulation of KEP references, then I will add a link that the core will add to the sample of comparative data reference.
                node.second.references.SetNodeId("ns=2;s=vPLC1.Test_node_2");
                node.second.references.SetIsForward(false);
                node.second.references.SetReferenceTypeId("i=47");
                node.second.references.AddReferenceToVector();
            }
            if (node.first == br_path_test2)
            {
                // As, how is the simulation of KEP references, then I will add a reference that the core will add to the sample of comparative data references.
                node.second.references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest");
                node.second.references.SetIsForward(false);
                node.second.references.SetReferenceTypeId("i=47");
                node.second.references.AddReferenceToVector();
            }
            if (node.first == br_path_test3)
            {
                // As, how is the simulation of KEP references, then I will add a reference that the core will add to the sample of comparative data references.
                node.second.references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest");
                node.second.references.SetIsForward(false);
                node.second.references.SetReferenceTypeId("i=47");
                node.second.references.AddReferenceToVector();
                // I will change the referenced node in the reference, since the kernel will replace the reference
                auto refs = node.second.references.GetReferences();
                refs[0].GetRef().nodeId = UA_EXPANDEDNODEID("i=63");
                node.second.references.SetReferences(std::move(refs));
            }
            if (node.first == br_path_test4)
            {
                // Изменю в ссылке HasTypeDefinition направленность, так-как это сделает ядро
                auto refs = node.second.references.GetReferences();
                refs[0].GetRef().isForward = true;
                node.second.references.SetReferences(std::move(refs));
            }
            if (node.first == br_path_test5)
            {
                // Since this is a simulation of adding backlinks, I will add a link that the kernel will add to the selection of comparative link data
                node.second.references.SetNodeId("ns=2;s=vPLC1.Test_node_2.BrowsePathTest.HasTypeDefinitionFix");
                node.second.references.SetIsForward(false);
                node.second.references.SetReferenceTypeId("i=47");
                node.second.references.AddReferenceToVector();
                // Change the direction of the HasTypeDefinition link, just like the kernel does
                auto refs = node.second.references.GetReferences();
                refs[0].GetRef().isForward = true;
                node.second.references.SetReferences(std::move(refs));
            }
        }

        Logger logger("test");
        logger.SetLevel(LogLevel::Debug);

        MockOpen62541 open(logger);
        MockEncoder encoder(logger, "nodeset");

#pragma region MockEncoder - exporting nodes
        // In any case, these methods can be called many times in a loop.
        REQUIRE_CALL(encoder, AddNodeObject(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_OBJECT)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeVariable(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeObjectType(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_OBJECTTYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeVariableType(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_VARIABLETYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeReferenceType(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_REFERENCETYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
            .WITH(_1.GetAttributes().empty() == false)
            .LR_WITH(valid_node_id.contains(_1.GetExpNodeId()) == true)
            .LR_SIDE_EFFECT(number_of_add_nodes_to_export++)
            .RETURN(StatusResults::Good)
            .TIMES(AT_LEAST(1));

        REQUIRE_CALL(encoder, AddNodeDataType(_))
            .WITH(_1.GetExpNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetExpNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetParentNodeId().GetType() == UA_TYPES_EXPANDEDNODEID)
            .LR_WITH(UA_ExpandedNodeId_equal(&_1.GetParentNodeId().GetRef(), &exp_nodeid_null) == false)
            .WITH(_1.GetNodeClass() == UA_NodeClass::UA_NODECLASS_DATATYPE)
            .WITH(_1.GetNodeReferences().empty() == false)
            .LR_SIDE_EFFECT(CheckRefferenceDescriptions(cmp_ref_descr.at(_1.GetExpNodeId()).references.GetReferences(), _1.GetNodeReferences()))
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
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeClassesRequestResponse& ncs : _1) { ncs.node_class = nodes_description.at(ncs.exp_node_id).node_class; })
                .RETURN(StatusResults::Good)
                .IN_SEQUENCE(seq);

            REQUIRE_CALL(open, ReadNodesAttributes(_, _))
                .WITH(_1.empty() == false)
                .SIDE_EFFECT(for (MockOpen62541::NodeAttributesRequestResponse& narr : _1) {
                    MESSAGE("NodeAttributesRequestResponse nodeID: ", narr.exp_node_id.get().ToString());
                    for (auto& attr : narr.attrs)
                    {
                        try
                        {
                            MESSAGE("Attr: ", attr.first, ", data: ", VariantsOfAttrToString(nodes_description.at(narr.exp_node_id).attributes.GetWrappAttr(attr.first)));
                            attr.second.emplace(nodes_description.at(narr.exp_node_id).attributes.GetWrappAttr(attr.first));
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
                .LR_SIDE_EFFECT(for (MockOpen62541::NodeReferencesRequestResponse& nrrr : _1) { nrrr.references = nodes_description.at(nrrr.exp_node_id).references.GetReferences(); })
                .RETURN(StatusResults::Good)
                .IN_SEQUENCE(seq);

            REQUIRE_CALL(encoder, AddAliases(_)).WITH(_1.empty() == false).RETURN(StatusResults::Good).IN_SEQUENCE(seq);
            REQUIRE_CALL(encoder, End()).RETURN(StatusResults::Good).IN_SEQUENCE(seq);

            NodesetExporterLoop exporter_loop(
                std::map<std::string, std::vector<UATypesContainer<UA_ExpandedNodeId>>>{{nodes_ids[0].ToString(), nodes_ids}},
                open,
                encoder,
                logger,
                {.is_perf_timer_enable = false,
                 .ns0_custom_nodes_ready_to_work = false,
                 .flat_list_of_nodes = {.is_enable = false, .create_missing_start_node = false, .allow_abstract_variable = false},
                 .parent_start_node_replacer = parent_start_node_replacer});
            auto status_result = StatusResults(StatusResults::Fail);
            CHECK_NOTHROW(status_result = exporter_loop.StartExport());
            // The number of nodes suitable for export classes must be equal to the number of nodes that will actually be transferred for export
            REQUIRE_EQ(number_of_valid_class_nodes_to_export, number_of_add_nodes_to_export);
            CHECK_EQ(status_result.GetStatus(), StatusResults::Good);
            MESSAGE("Number of nodes: ", nodes_ids.size(), ", number of nodes to be exported under incoming classes: ", number_of_add_nodes_to_export);
        }
    }
}