//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_UATYPESCONTAINER_H
#define NODESETEXPORTER_OPEN62541_UATYPESCONTAINER_H

#include <open62541/util.h>

#include <iostream>
#include <memory>

namespace nodesetexporter::open62541
{

/**
 * @brief Container class for standard C-objects of the Open62541 library providing object lifetime management in C++11 style.
 * @tparam TOpen62541Type Types of Open62541 library objects.
 * todo Create a map in the form of a method where the parameter should be a type, and the output should be a numeric type identifier,
 *   this way you can get rid of the extra ua_type parameter in the constructor. I couldn't find such a ready-made function in the library.
 */
template <typename TOpen62541Type = std::nullptr_t>
class UATypesContainer final
{
public:
    using Type = TOpen62541Type;

    UATypesContainer() = delete;

    /**
     * @brief Creates an empty object of the specified type inside the wrapper.
     * @param ua_type Type of object created.
     */
    explicit UATypesContainer(u_int32_t ua_type)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(ua_type)
        , m_is_empty_object(true)
    {
        UA_init(m_ua_object, &UA_TYPES[ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    /**
     * @brief Creates an object and makes a deep copying of an object of type TOpen62541Type.
     * @param ua_type_obj Link to a copied object type TOpen62541Type.
     * @param ua_type Type of object TOpen62541Type.
     */
    explicit UATypesContainer(const TOpen62541Type& ua_type_obj, u_int32_t ua_type)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(ua_type)
    {
        UA_copy(&ua_type_obj, m_ua_object, &UA_TYPES[ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    /**
     * @brief Creates the object of the wrapper and retains the pointer to the object TOpen62541Type without taking it under control of the management of the life cycle, but allows
     *        Use all the functionality of the wrapper. Zero copying of the object.
     * @warning As a wrap does not control the life cycle of an internal object, it is necessary to ensure that the object itself exist when calling
     * any functions of the wrapper. There is no guarantee of a non-change of object of the TOpen62541Type type.
     * @param ua_type_obj A pointer to an object of the type TOpen62541Type.
     * @param ua_type Type of object TOpen62541Type.
     */
    explicit UATypesContainer(TOpen62541Type* const ua_type_obj, u_int32_t ua_type)
        : m_ua_type(ua_type)
        , m_ua_object(ua_type_obj)
        , m_is_weak_ref(true)
    {
    }

    ~UATypesContainer()
    {
        if (m_ua_object != nullptr && !m_is_weak_ref)
        {
            UA_delete(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    };

    UATypesContainer(const UATypesContainer& obj)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[obj.m_ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(obj.m_ua_type)
    {
        UA_copy(obj.m_ua_object, m_ua_object, &UA_TYPES[obj.m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    UATypesContainer& operator=(const UATypesContainer& obj)
    {
        if (this != &obj)
        {
            UA_delete(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            m_ua_type = obj.m_ua_type;
            m_ua_object = static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[m_ua_type])); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            UA_copy(obj.m_ua_object, m_ua_object, &UA_TYPES[obj.m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        return *this;
    };

    UATypesContainer(UATypesContainer&& obj) noexcept
        : m_ua_object(nullptr)
        , m_ua_type(0)
    {
        *this = std::move(obj);
    };

    UATypesContainer& operator=(UATypesContainer&& obj) noexcept
    {
        if (this != &obj)
        {
            UA_delete(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            m_ua_object = obj.m_ua_object;
            m_ua_type = obj.m_ua_type;
            obj.m_ua_object = nullptr;
            obj.m_ua_type = 0;
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& o_stream, const UATypesContainer& obj)
    {
        o_stream << obj.ToString();
        return o_stream;
    }

    bool operator==(const UATypesContainer<UA_NodeId>& obj) const
    {
        static_assert(std::is_same_v<TOpen62541Type, UA_NodeId>);
        return UA_NodeId_equal(m_ua_object, &obj.GetRef());
    }

    bool operator==(const UATypesContainer<UA_ExpandedNodeId>& obj) const
    {
        static_assert(std::is_same_v<TOpen62541Type, UA_ExpandedNodeId>);
        return UA_ExpandedNodeId_equal(m_ua_object, &obj.GetRef());
    }

    /**
     * @brief Get a constant reference to TOpen62541Type.
     * @return Constant reference to TOpen62541Type.
     */
    [[nodiscard]] const TOpen62541Type& GetRef() const noexcept
    {
        return *m_ua_object;
    }

    /**
     * @brief Get a reference to TOpen62541Type with the ability to modify the object.
     * @return Reference to TOpen62541Type.
     */
    [[maybe_unused]] TOpen62541Type& GetRef() noexcept
    {
        return *m_ua_object;
    }

    /**
     * @brief Get the content type of the UA Open62541 object in the container.
     * @return The numeric representation of the object type. Can be found in the types_generated.h file of the Open62541 library.
     */
    [[nodiscard]] u_int32_t GetType() const noexcept
    {
        return m_ua_type;
    }

    /**
     * @brief Output the object's contents as text. The content is encoded in JSON format.
     * @param write_json_quote If it is true, then complex elements will be framed in quotation marks.
     * @return Contents as std::string
     */
    [[nodiscard]] std::string ToString() const
    {
        std::unique_ptr<UA_String, void (*)(UA_String*)> out(UA_String_new(), UA_String_delete);
        UA_String_init(out.get());
        if (UA_print(m_ua_object, &UA_TYPES[m_ua_type], out.get()) != UA_STATUSCODE_GOOD) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        {
            return std::string{"ToString() error"};
        }
#ifdef OPEN62541_UAPRINT_WITH_QUOTES
        // Since in the Open62541 library some JSON elements began to be framed in quotes, for compatibility
        // I will remove them if the definition is activated.
        return std::string{static_cast<char*>(static_cast<void*>(out->data)), out->length}.substr(1, out->length - 2); // NOLINT(bugprone-casting-through-void)
#else
        // Fix for 1.3.x versions with quotation marks for UA_String.
        if(m_ua_type == UA_TYPES_STRING)
        {
            return std::string{static_cast<char*>(static_cast<void*>(out->data)), out->length}.substr(1, out->length - 2); // NOLINT(bugprone-casting-through-void)
        }
        return std::string{static_cast<char*>(static_cast<void*>(out->data)), out->length};
#endif
    }

    /**
     * @brief nodeid initialization method through xml notation 'ns=<namespaceIndex>;<identifiertype>=<identifier>'
     * Example: 'ns=2;s=MyTemperature'
     */
    template <typename T = TOpen62541Type>
    void SetParamFromString(std::string_view node_id)
        requires(std::is_same_v<T, UA_NodeId>)
    {
        ClearNodeIDObject();
        UA_NodeId_parse(m_ua_object, UA_STRING(const_cast<char*>(node_id.data()))); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        m_is_empty_object = false;
    }

    /**
     * @brief nodeid initialization method through XML Notation'ns=<namespaceIndex>;<identifiertype>=<identifier>'
     * Example: 'ns=2;s=MyTemperature'
     */
    template <typename T = TOpen62541Type>
    void SetParamFromString(std::string&& node_id)
        requires(std::is_same_v<T, UA_NodeId>)
    {
        ClearNodeIDObject();
        auto node_id_tmp = std::move(node_id);
        UA_NodeId_parse(m_ua_object, UA_STRING(node_id_tmp.data()));
        m_is_empty_object = false;
    }

    /**
     * @brief ExpandedNodeId object initialization method through XML Notation'ns=<namespaceIndex>;<identifiertype>=<identifier>'
     * Example: 'ns=2;s=MyTemperature', 'nsu=http://test.org/UA/Data/;s=some.node.id'
     */
    template <typename T = TOpen62541Type>
    void SetParamFromString(std::string_view exp_node_id)
        requires(std::is_same_v<T, UA_ExpandedNodeId>)
    {
        ClearNodeIDObject();
        UA_ExpandedNodeId_parse(m_ua_object, UA_STRING(const_cast<char*>(exp_node_id.data()))); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        m_is_empty_object = false;
    }

    /**
     * @brief ExpandedNodeId object initialization method through XML Notation 'ns=<namespaceIndex>;<identifiertype>=<identifier>'
     * Example: 'ns=2;s=MyTemperature', 'nsu=http://test.org/UA/Data/;s=some.node.id'
     */
    template <typename T = TOpen62541Type>
    void SetParamFromString(std::string&& exp_node_id)
        requires(std::is_same_v<T, UA_ExpandedNodeId>)
    {
        ClearNodeIDObject();
        auto exp_node_id_tmp = std::move(exp_node_id);
        UA_ExpandedNodeId_parse(m_ua_object, UA_STRING(exp_node_id_tmp.data()));
        m_is_empty_object = false;
    }

    /**
     * @brief Method for shallow copying an object and then taking ownership.
     * If the internal object of this class is not empty, the contents will undergo deep clearing, then a shallow copy
     * of the external object is performed (for example, structure fields by value and pointers are copied, but not data by pointers).
     * Then, by convention, the object is taken ownership, since during the process of deleting the UATypesContainer object itself,
     * a deep clearing of the copy of the external ua_type_obj object will be performed. Therefore, after calling this function
     * on the ua_type_obj object, it cannot be deep cleared in the external environment using individual Open62541 library methods.
     * @warning TOpen62541Type objects that do not contain pointers will be copied in their entirety. Such objects are safe to copy and use, but this function is meaningless.
     * To copy such objects, use the deep copy constructor: UATypesContainer(const TOpen62541Type& ua_type_obj, u_int32_t ua_type).
     * @warning It is recommended to create a TOpen62541Type object on the stack to automatically, shallowly delete the primary copy of ua_type_obj.
     * @warning A copied TOpen62541Type data object cannot be deep deleted.
     * @warning Use with extreme caution to avoid double-deleting the object or using the original object with pointers inside after using this function.
     * @param ua_type_obj : any TOpen62541Type from the Open62541 library.
     */
    void ShallowCopyingAndOwnership(const TOpen62541Type& ua_type_obj)
    {
        if (!m_is_empty_object && !m_is_weak_ref)
        {
            UA_clear(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
        *m_ua_object = ua_type_obj; // Поверхностное копирование без копирования других полей по указателям. Копируются только сами указатели структуры.
    }

private:
    /**
     * @brief The method cleansing internal objects of the library Open62541 with verification, such as NodeID, ExpandedNodeID.
     */
    void ClearNodeIDObject()
    {
        if constexpr (std::is_same_v<TOpen62541Type, UA_NodeId>)
        {
            if (!UA_NodeId_isNull(m_ua_object))
            {
                UA_clear(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            }
        }
        if constexpr (std::is_same_v<TOpen62541Type, UA_ExpandedNodeId>)
        {
            if (!UA_NodeId_isNull(&m_ua_object->nodeId))
            {
                UA_clear(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            }
        }
    }

private:
    u_int32_t m_ua_type;
    TOpen62541Type* m_ua_object;
    bool m_is_weak_ref = false;
    bool m_is_empty_object = false;
};

} // namespace nodesetexporter::open62541


namespace std
{

using nodesetexporter::open62541::UATypesContainer;

/**
 * Structure with a method for comparing objects of the Open62541 library, where such objects can act as keys.
 * The structure supports two types: UA_NodeId, UA_ExpandedNodeId.
 * Required to support sorting. Used in containers such as std::map, std::set, etc.
 */
template <typename TUA_Struct>
struct less<UATypesContainer<TUA_Struct>>
{
    bool operator()(const UATypesContainer<UA_ExpandedNodeId>& first, const UATypesContainer<UA_ExpandedNodeId>& second) const
    {
        return (UA_ExpandedNodeId_order(&first.GetRef(), &second.GetRef()) == UA_ORDER_LESS);
    }

    bool operator()(const UATypesContainer<UA_NodeId>& first, const UATypesContainer<UA_NodeId>& second) const
    {
        return (UA_NodeId_order(&first.GetRef(), &second.GetRef()) == UA_ORDER_LESS);
    }
};

/**
 * @brief Structure with methods for calculating the hash objects Open62541.
 * The structure supports two types: UA_NodeId, UA_ExpandedNodeId.
 */
template <typename TUA_Struct>
struct hash<UATypesContainer<TUA_Struct>>
{
    std::size_t operator()(const UATypesContainer<UA_ExpandedNodeId>& exp_nodeid) const
    {

        return UA_ExpandedNodeId_hash(&exp_nodeid.GetRef());
    }

    std::size_t operator()(const UATypesContainer<UA_NodeId>& nodeid) const
    {
        return UA_NodeId_hash(&nodeid.GetRef());
    }
};
} // namespace std

#endif // NODESETEXPORTER_OPEN62541_UATYPESCONTAINER_H
