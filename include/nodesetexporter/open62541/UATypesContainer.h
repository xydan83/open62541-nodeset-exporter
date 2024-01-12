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
    UATypesContainer() = delete;

    explicit UATypesContainer(u_int32_t ua_type)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(ua_type)
    {
        UA_init(m_ua_object, &UA_TYPES[ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    /**
     * @brief Creates an object and makes a deep copy to itself of an object of type TOpen62541Type.
     */
    explicit UATypesContainer(const TOpen62541Type& ua_type_obj, u_int32_t ua_type)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(ua_type)
    {
        UA_copy(&ua_type_obj, m_ua_object, &UA_TYPES[ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    ~UATypesContainer()
    {
        if (m_ua_object != nullptr)
        {
            UA_delete(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    };

    UATypesContainer(const UATypesContainer& obj)
        : m_ua_object(static_cast<TOpen62541Type*>(UA_new(&UA_TYPES[obj.m_ua_type]))) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        , m_ua_type(obj.m_ua_type)
        , m_status_code(obj.m_status_code)
    {
        UA_copy(obj.m_ua_object, m_ua_object, &UA_TYPES[obj.m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    };

    UATypesContainer& operator=(const UATypesContainer& obj)
    {
        if (this != &obj)
        {
            UA_delete(m_ua_object, &UA_TYPES[m_ua_type]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            m_ua_type = obj.m_ua_type;
            m_status_code = obj.m_status_code;
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
            m_status_code = obj.m_status_code;
            obj.m_ua_object = nullptr;
            obj.m_ua_type = 0;
            obj.m_status_code = UA_STATUSCODE_GOOD;
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& o_stream, const UATypesContainer& obj)
    {
        o_stream << obj.ToString();
        return o_stream;
    }

    /**
     * @brief Set the status code of the internal object. (Optional)
     *        If an OPC UA object is placed in a container in which it is necessary to comply with the rule for the sequence of objects (for example, a list of objects), a status code is returned,
     *        even if the object was not read for some reason. In this case the object will be empty and will be returned
     *        Fail-StatusCode, by which you can determine whether an object is valid or not and its status.
     *        In the OPC UA standard, all attributes are returned according to the principle - they sent a list of NodeIDs, received back a list of attributes and a list of statuses in the same
     *        sequence as they sent.
     * https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2
     * @param status_code Integer value of the object's status code. Codes for standard 1.04 - http://www.opcfoundation.org/UA/schemas/StatusCode.csv
     */
    [[maybe_unused]] void SetStatusCode(UA_StatusCode status_code)
    {
        m_status_code = status_code;
    }

    /**
     * @brief Get the status code of an internal object. (Optional)
     * @return The integer value of the object's status code. Codes for standard 1.04 - http://www.opcfoundation.org/UA/schemas/StatusCode.csv
     */
    [[maybe_unused]] [[nodiscard]] UA_StatusCode GetStatusCode() const
    {
        return m_status_code;
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
     * @return Contents as std::string
     */
    [[nodiscard]] std::string ToString() const
    {
        UA_String out = UA_STRING_NULL;
        if (UA_print(m_ua_object, &UA_TYPES[m_ua_type], &out) != UA_STATUSCODE_GOOD) // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        {
            return std::string{"ToString() error"};
        }

#ifdef OPEN62541_UAPRINT_WITH_QUOTES
        // Since in the Open62541 library some JSON elements began to be framed in quotes, for compatibility
        // I will remove them if the definition is activated.
        return std::string{static_cast<char*>(static_cast<void*>(out.data)), out.length}.substr(1, out.length - 2);
#else
        return std::string{static_cast<char*>(static_cast<void*>(out.data)), out.length};
#endif
    }

private:
    u_int32_t m_ua_type;
    // By default, all objects created inside the container will have a positive status code. Optionally, you can set the desired status code.
    UA_StatusCode m_status_code = UA_STATUSCODE_GOOD;
    TOpen62541Type* m_ua_object;
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
} // namespace std

#endif // NODESETEXPORTER_OPEN62541_UATYPESCONTAINER_H
