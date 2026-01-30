//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_TYPEALIASES_H
#define NODESETEXPORTER_OPEN62541_TYPEALIASES_H

#include "nodesetexporter/open62541/UATypesContainer.h"

#include <fmt/format.h>
#include <optional>
#include <variant>
#include <vector>

namespace nodesetexporter::open62541::typealiases
{
using ::nodesetexporter::open62541::UATypesContainer;

/**
 * @brief An anchor class to allow compile-time methods (such as is_base_of_v, ...) to determine if the data contains a MultidimensionalArray structure without specifying nested <T> types.
 */
class MultidimensionalArrayAnchor : std::in_place_t
{
};

/**
 * @brief A class describing an array supporting multiple dimensions. Comparable to the UA_Variant structure in terms of data description,
 * but only with respect to array data storage.
 * The descriptions of the fields for identifying array dimensions comply with the OPCUA standard https://reference.opcfoundation.org/Core/Part6/v104/docs/5.2.2.16
 * The ArrayLength field is absent, as it is equivalent to the data.size() call and is returned by the ArrayLength() function.
 * Single-dimensional arrays are not encoded in any way.
 * Multidimensional arrays are encoded as a single-dimensional array; the array length determines the total number of elements. The original array can be reconstructed from the dimensions, which are
 * encoded after the value field. Dimensions of higher rank are serialized first. For example, an array with dimensions [2,2,2] is written in the following order: [0,0,0], [0,0,1], [0,1,0], [0,1,1],
 * [1,0,0], [1,0,1], [1,1,0], [1,1,1] Multidimensional arrays are defined as having data in the array returned by the GetArray function where ArrayLength > 0 and ArrayDimensionsLength > 1. A
 * one-dimensional array is defined as having data in the array returned by the GetArray function where ArrayLength > 0 and ArrayDimensionsLength = 0. The GetArrayDimensions method returns a populated
 * array only if the number of array dimensions is 2 or more (multidimensional array).
 * @warning Not used for regular scalar values.
 * @tparam T The type of the stored data.
 */
template <typename T>
class MultidimensionalArray : MultidimensionalArrayAnchor
{
public:
    MultidimensionalArray() = default;
    /**
     * @brief Method for initializing an array of varying depths.
     * @param data The data array.
     * @param array_dimensions An array specifying the number of dimensions and the number of elements in each dimension of the data array.
     * Only populated if the number of array dimensions is 2 or more (multidimensional array).
     * For a single-dimensional array, pass an empty array.
     */
    explicit MultidimensionalArray(const std::vector<T>& data, const std::vector<UA_UInt32>& array_dimensions = {})
    {
        SetArray(data, array_dimensions);
    }
    /**
     * @brief Method for initializing an array of varying depths.
     * @param data The data array.
     * @param array_dimensions An array specifying the number of dimensions and the number of elements in each dimension of the data array.
     * Only populated if the number of array dimensions is 2 or more (multidimensional array).
     * For a single-dimensional array, pass an empty array.
     */
    explicit MultidimensionalArray(std::vector<T>&& data, std::vector<UA_UInt32>&& array_dimensions = {})
    {
        SetArray(std::move(data), std::move(array_dimensions));
    }; // Так-же для inplace
    ~MultidimensionalArray() = default;
    MultidimensionalArray(MultidimensionalArray& m_array)
        : m_array_dimensions(m_array.m_array_dimensions)
        , m_data(m_array.m_data)
    {
    }
    MultidimensionalArray(const MultidimensionalArray& m_array) = default;
    MultidimensionalArray& operator=(const MultidimensionalArray& m_array) = default;
    MultidimensionalArray(MultidimensionalArray&&) noexcept = default;
    MultidimensionalArray& operator=(MultidimensionalArray&& obj) noexcept = default;

    friend std::ostream& operator<<(std::ostream& o_stream, const MultidimensionalArray& obj)
    {
        o_stream << obj.ToString();
        return o_stream;
    }

    /**
     * @brief Method for initializing an array of varying depths.
     * @param data The data array.
     * @param array_dimensions An array specifying the number of dimensions and the number of elements in each dimension of the data array.
     * Only populated if the number of array dimensions is 2 or more (multidimensional array).
     * For a single-dimensional array, pass an empty array.
     */
    void SetArray(std::vector<T>&& data, std::vector<UA_UInt32>&& array_dimensions = {})
    {
        CheckLength(data, array_dimensions);
        m_array_dimensions = std::move(array_dimensions);
        m_data = std::move(data);
    }

    /**
     * @brief Method for initializing an array of varying depths.
     * @param data The data array.
     * @param array_dimensions An array specifying the number of dimensions and the number of elements in each dimension of the data array.
     * Only populated if the number of array dimensions is 2 or more (multidimensional array).
     * For a single-dimensional array, pass an empty array.
     */
    void SetArray(const std::vector<T>& data, const std::vector<UA_UInt32>& array_dimensions = {})
    {
        CheckLength(data, array_dimensions);
        m_array_dimensions = array_dimensions;
        m_data = data;
    }

    /**
     * @brief Returns a serialized array of data based on the number of dimensions.
     * @warning Single-dimensional arrays are not encoded in any way.
     * Multidimensional arrays are encoded as a single-dimensional array, and this field indicates the total number of elements.
     * The original array can be reconstructed from the dimensions, which are encoded after the value field.
     * Dimensions of higher rank are serialized first. For example, an array with dimensions [2,2,2] is written in the following order:
     * [0,0,0], [0,0,1], [0,1,0], [0,1,1], [1,0,0], [1,0,1], [1,1,0], [1,1,1]
     */
    [[nodiscard]] std::vector<T> GetArray() const
    {
        return m_data;
    }

    /**
     * @brief Returns an array containing the number of elements in each dimension of the data array.
     * @warning GetArrayDimensions returns a non-empty array only if the number of dimensions is 2 or greater and all dimensions have a length greater than 0.
     */
    [[nodiscard]] std::vector<UA_UInt32> GetArrayDimensions() const
    {
        return m_array_dimensions;
    }

    /**
     * @brief The number of dimensions in the data array.
     * @warning ArrayDimensionsLength returns nonzero values only if the number of dimensions is 2 or greater and all dimensions have a length greater than 0.
     */
    [[nodiscard]] size_t ArrayDimensionsLength() const
    {
        return m_array_dimensions.size();
    }

    /**
     * @brief Number of elements in the array
     * @warning In the case of a one-dimensional array, the function returns the number of elements in the one-dimensional array.
     * In the case of a multidimensional array, the total length of the array will return the number of elements in all dimensions, where the number of elements is equal to the product of all the
     * lengths of the dimensions.
     */
    [[nodiscard]] size_t ArrayLength() const
    {
        return m_data.size();
    }

    /**
     * @brief Display the contents of arrays in text form.
     */
    [[nodiscard]] std::string ToString() const
    {
        return fmt::format(
            "MultidimensionalArray\r\nType: {}\r\nArray dim size: {}\r\nArray dimensions: {}\r\nVector data: {}",
            typeid(T).name(),
            m_array_dimensions.size(),
            VectorToString<UA_UInt32>(m_array_dimensions),
            VectorToString<T>(m_data));
    }

private:
    /**
     * @brief Map the contents of the array to text.
     */
    template <typename VecType>
    [[nodiscard]] std::string VectorToString(const std::vector<VecType>& vect) const;

    /**
     * @brief The function checks the length of the data array and the number of elements in the array, which conveys the depth
     * and the number of elements in each dimension of the data array.
     * The check is only performed for the specified depth > 1 measurement.
     * @param data Array of data.
     * @param array_dimensions The array of dimensions of the data array.
     * @idlexcept In case the check compares the length of the data array and the specified number of elements in each dimension
     * do not converge - a std::runtime_error exception is thrown with a description of the error.
     * If array_dimensions specifies the depth of one dimension, a std::runtime_error exception is thrown with a description of the error.
     */
    void CheckLength(const std::vector<T>& data, const std::vector<UA_UInt32>& array_dimensions) const
    {
        if (array_dimensions.size() > 1)
        {
            size_t calculated_array_size = 1; // Будем выполнять умножение, поэтому единица.
            for (const auto& arr_dim : array_dimensions)
            {
                if (arr_dim == 0)
                {
                    throw std::runtime_error("The length of dimension must be greater than 0");
                }
                calculated_array_size *= arr_dim;
            }
            if (data.size() != calculated_array_size)
            {
                throw std::runtime_error("The length of the array must be equal to the product of the lengths of the dimensions");
            }
        }
        if (array_dimensions.size() == 1)
        {
            throw std::runtime_error("The ArrayDimensions field shall only be present if the number of dimensions is 2 or greater and all dimensions have a length greater than 0.");
        }
    }

private:
    std::vector<UA_UInt32> m_array_dimensions; // Depth of each dimension.  Dimensions with lower rank appear first in the array. All dimensions must be specified and must be greater than zero.
                                               // array_dimensions_size is the size() of m_array_dimensions.
    std::vector<T> m_data; // Array data
};

/**
 * @brief Since the status code is transmitted using the UA_UInt32 type, to highlight its essence I pack it into a separate structure.
 */
struct StatusCode
{
    StatusCode() = default;
    explicit StatusCode(UA_StatusCode stat_code)
        : m_status_code(stat_code)
    {
    }

    UA_StatusCode m_status_code = UA_STATUSCODE_GOOD;

    [[nodiscard]] std::string ToString() const
    {
        return std::to_string(m_status_code);
    }
};

/**
 * @brief Since UA_ByteString is a typedef for UA_String, to highlight its essence I pack it into a separate structure.
 */
struct ByteString
{
    ByteString() = default;
    explicit ByteString(const UA_ByteString& b_string)
        : m_byte_string(b_string)
    {
    }

    UA_ByteString m_byte_string = UA_BYTESTRING_NULL;
};


using VariantsOfAttr = std::variant<
    // Scalars
    UA_Boolean, // Used by the IsAbstract, Symmetric, ContainsNoLoops, Historizing, Executable, UserExecutable, Value attribute
    UA_SByte, // Used by the Value attribute
    UA_Byte, // Used by the EventNotifier, AccessLevel, UserAccessLevel, Value attribute
    UA_Int16, // Used by the Value attribute
    UA_UInt16, // Used by the Value attribute
    UA_Int32, // Used by the Value attribute
    UA_UInt32, // Used by the WriteMask, UserWriteMask, Value attribute
    UA_Int64, // Used by the Value attribute
    UA_UInt64, // Used by the Value attribute
    UA_Float, // Used by the Value attribute
    UA_Double, // Used by the MinimumSamplingInterval, Value attribute
    UA_NodeClass, // Used by the NodeClass attribute
    StatusCode, // Used by the Value attribute
    UATypesContainer<ByteString>, // Used by the Value attribute
    UATypesContainer<UA_DateTime>, // Used by the Value attribute (Essentially it’s an int64, but since it needs to be recognized as a DateTime, I’ll pack it into a container).
    UATypesContainer<UA_Guid>, // Used by the Value attribute
    UATypesContainer<UA_String>, // Used by the Value attribute
    UATypesContainer<UA_NodeId>, // Used by the DataType, Value attribute
    UATypesContainer<UA_ExpandedNodeId>, // Used by the Value attribute
    UATypesContainer<UA_QualifiedName>, // Used by the BrowseName, Value attribute
    UATypesContainer<UA_LocalizedText>, // Used by the DisplayName, Description, InverseName, Value attribute
    UATypesContainer<UA_Variant>, // Used by the Value attribute (not implemented)
    UATypesContainer<UA_StructureDefinition>, // Used by the DataTypeDefinition attribute (no separate function)
    UATypesContainer<UA_EnumDefinition>, // Used by the DataTypeDefinition attribute (no separate function)
    UATypesContainer<UA_DiagnosticInfo>, // Used by the Value attribute
    // Arrays
    MultidimensionalArray<UA_Boolean>, // Used by the Value attribute
    MultidimensionalArray<UA_SByte>, // Used by the Value attribute
    MultidimensionalArray<UA_Byte>, // Used by the Value attribute
    MultidimensionalArray<UA_Int16>, // Used by the Value attribute
    MultidimensionalArray<UA_UInt16>, // Used by the Value attribute
    MultidimensionalArray<UA_Int32>, // Used by the Value attribute
    MultidimensionalArray<UA_UInt32>, // Used by the ArrayDimensions attribute - carries an UInt32 array, Value
    MultidimensionalArray<UA_Int64>, // Used by the Value attribute
    MultidimensionalArray<UA_UInt64>, // Used by the Value attribute
    MultidimensionalArray<UA_Float>, // Used by the Value attribute
    MultidimensionalArray<UA_Double>, // Used by the Value attribute
    MultidimensionalArray<StatusCode>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<ByteString>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_DateTime>>, // Used by the Value attribute (Essentially this is an int64, but since it needs to be recognized as a DateTime, I will pack it into a
                                                          // container).
    MultidimensionalArray<UATypesContainer<UA_Guid>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_String>>, // Used by the Value attribute (including UA_ByteString)
    MultidimensionalArray<UATypesContainer<UA_NodeId>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_ExpandedNodeId>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_QualifiedName>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_LocalizedText>>, // Used by the Value attribute
    MultidimensionalArray<UATypesContainer<UA_Variant>>, // Used by the Value attribute (not implemented)
    MultidimensionalArray<UATypesContainer<UA_DiagnosticInfo>>>; // Used by the Value attribute

/**
 * @brief Function for converting variant variable values into a string.
 * @param var
 * @return A string representing the value of the variant variable.
 */
std::string VariantsOfAttrToString(const VariantsOfAttr& var);

/**
 * @brief Function to convert UA_Variant to std::optional<VariantsOfAttr>.
 * @param variant UA_Variant.
 * @return Returns a std::variant described as VariantsOfAttr with an optional wrapper to allow null content to be expressed (similar to a monostate).
 *         If there is no value or if there is content not supported in VariantsOfAttr, the value will be nullopt.
 */
std::optional<VariantsOfAttr> UAVariantToStdVariant(const UA_Variant& variant);

} // namespace nodesetexporter::open62541::typealiases
#endif // NODESETEXPORTER_OPEN62541_TYPEALIASES_H
