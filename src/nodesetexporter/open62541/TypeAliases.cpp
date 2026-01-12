//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/open62541/TypeAliases.h"

#include <magic_enum/magic_enum.hpp>

namespace nodesetexporter::open62541::typealiases
{

std::string VariantsOfAttrToString(const VariantsOfAttr& var)
{
    return std::visit(
        []<class T>(const T& arg) -> std::string
        {
            if constexpr (std::is_same_v<T, UA_Boolean>)
            {
                return std::to_string(static_cast<int>(arg));
            }
            else if constexpr (std::is_same_v<T, UA_NodeClass>)
            {
                return std::string(magic_enum::enum_name(arg));
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return std::to_string(arg);
            }
            else
            {
                return arg.ToString();
            }
        },
        var);
}

template <typename T>
template <typename VecType>
std::string MultidimensionalArray<T>::VectorToString(const std::vector<VecType>& vect) const
{
    if (vect.empty())
    {
        return "[]";
    }
    std::string data_t;
    for (const auto& val : vect)
    {
        if constexpr (std::is_scalar_v<VecType>)
        {
            data_t.append(fmt::format("{}, ", std::to_string(val)));
        }
        else
        {
            // Так-как ToString будет только в случае применения обертки UATypesContainer, то необходимо оборачивать в "constexpr else".
            data_t.append(fmt::format("\r\n{}, ", val.ToString()));
        }
    }
    data_t.erase(data_t.end() - 2);
    return fmt::format("[{}]", data_t);
}
namespace
{
/**
 * @brief Конвертация примитивных типов данных в Variant библиотеки Open62541 в std::optional<std::variant>.
 * @tparam T Тип данных.
 * @param variant Объект данных типа Variant библиотеки Open62541.
 * @return Объект данных типа Variant библиотеки Open62541.
 */
template <typename T>
std::optional<VariantsOfAttr> ToVariantPlainType(const UA_Variant& variant)
{
    return std::optional<VariantsOfAttr>{VariantsOfAttr(*static_cast<T*>(variant.data))};
}

/**
 * @brief Конвертация комплексных (структурных) типов дынных в Variant библиотеки Open62541 в std::optional<std::variant>.
 * @tparam T Тип данных.
 * @param variant Объект данных типа Variant библиотеки Open62541.
 * @param data_enum_type тип структуры описывающую хранимые данные в формате OPC UA.
 * @return Объект данных типа Variant библиотеки Open62541 упакованный в контейнер UATypesContainer для управлением времени жизни.
 */
template <typename T>
std::optional<VariantsOfAttr> ToVariantStructType(const UA_Variant& variant, uint16_t data_enum_type)
{
    return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<T>(*static_cast<T*>(variant.data), data_enum_type))};
}

/**
 * @brief Конвертация примитивных типов массивов дынных в Variant библиотеки Open62541 в std::optional<std::variant>.
 * @tparam T Тип данных.
 * @param variant Объект данных типа Variant библиотеки Open62541.
 * @return Объект данных типа Variant библиотеки Open62541.
 */
template <typename T>
std::optional<VariantsOfAttr> ToVariantPlainTypeArray(const UA_Variant& variant)
{
    if (variant.data == nullptr)
    {
        return {std::nullopt};
    }

    MultidimensionalArray<T> multidimens_array;
    std::vector<UA_UInt32> arr_dim;
    arr_dim.reserve(variant.arrayDimensionsSize);
    for (size_t index = 0; index < variant.arrayDimensionsSize; ++index)
    {
        arr_dim.push_back(static_cast<UA_UInt32*>(variant.arrayDimensions)[index]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    std::vector<T> arr_data;
    arr_data.reserve(variant.arrayLength);
    for (size_t index = 0; index < variant.arrayLength; ++index)
    {
        arr_data.push_back(static_cast<T*>(variant.data)[index]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    multidimens_array.SetArray(std::move(arr_data), std::move(arr_dim));
    return std::optional<VariantsOfAttr>{VariantsOfAttr(multidimens_array)};
}


/**
 * @brief Конвертация комплексных (структурных) типов массивов дынных в Variant библиотеки Open62541 в std::optional<std::variant>.
 * @tparam T Тип данных.
 * @param variant Объект данных типа Variant библиотеки Open62541.
 * @param data_enum_type тип структуры описывающую хранимые данные в формате OPC UA.
 * @return Объект данных типа Variant библиотеки Open62541 упакованный в контейнер UATypesContainer для управлением времени жизни.
 */
template <typename T>
std::optional<VariantsOfAttr> ToVariantStructTypeArray(const UA_Variant& variant, uint16_t data_enum_type)
{
    if (variant.data == nullptr)
    {
        return {std::nullopt};
    }

    MultidimensionalArray<UATypesContainer<T>> multidimens_array;
    std::vector<UA_UInt32> arr_dim;
    arr_dim.reserve(variant.arrayDimensionsSize);
    for (size_t index = 0; index < variant.arrayDimensionsSize; ++index)
    {
        arr_dim.push_back(static_cast<UA_UInt32*>(variant.arrayDimensions)[index]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    std::vector<UATypesContainer<T>> arr_data;
    arr_data.reserve(variant.arrayLength);
    for (size_t index = 0; index < variant.arrayLength; ++index)
    {
        const auto obj = UATypesContainer<T>(static_cast<T*>(variant.data)[index], data_enum_type); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        arr_data.push_back(obj);
    }
    multidimens_array.SetArray(std::move(arr_data), std::move(arr_dim));
    return std::optional<VariantsOfAttr>{VariantsOfAttr(multidimens_array)};
}
} // namespace

std::optional<VariantsOfAttr> UAVariantToStdVariant(const UA_Variant& variant)
{
    if (UA_Variant_isEmpty(&variant))
    {
        return {std::nullopt};
    }

    // К сожалению switch нельзя использовать, так-как variant.type выдает структуру, а variant.type->typeKind - может повториться и используется исключительно внутри.
    // Типы UA_TYPES дают более детальное описание. Например, UA_TYPES_NODECLASS - идет как структура, а среди UA_DATATYPEKINDS ее нет, скорее будет опознаваться как UA_DATATYPEKIND_STRUCTURE.
    if (UA_Variant_isScalar(&variant))
    {
        // Скаляры
        // Простые типы
        if (variant.type == &UA_TYPES[UA_TYPES_BOOLEAN])
        {
            return ToVariantPlainType<UA_Boolean>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_SBYTE])
        {
            return ToVariantPlainType<UA_SByte>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_BYTE])
        {
            return ToVariantPlainType<UA_Byte>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT16])
        {
            return ToVariantPlainType<UA_Int16>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT16])
        {
            return ToVariantPlainType<UA_UInt16>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT32])
        {
            return ToVariantPlainType<UA_Int32>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT32])
        {
            return ToVariantPlainType<UA_UInt32>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT64])
        {
            return ToVariantPlainType<UA_Int64>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT64])
        {
            return ToVariantPlainType<UA_UInt64>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_FLOAT])
        {
            return ToVariantPlainType<UA_Float>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DOUBLE])
        {
            return ToVariantPlainType<UA_Double>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_NODECLASS])
        {
            return ToVariantPlainType<UA_NodeClass>(variant);
        }
        // Структурные типы
        if (variant.type == &UA_TYPES[UA_TYPES_STATUSCODE])
        {
            // Исключение, так-как кастомная структура с одним полем для идентификации типа.
            return ToVariantPlainType<StatusCode>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_BYTESTRING])
        {
            // Исключение, так-как UA_ByteString и UA_String - это алиасы, пришлось делать обертку + это структурный тип. Но на уровне типов OPC - это разные типы.
            return ToVariantStructType<ByteString>(variant, UA_TYPES_BYTESTRING);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UTCTIME])
        {
            return ToVariantStructType<UA_UtcTime>(variant, UA_TYPES_UTCTIME);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DATETIME])
        {
            return ToVariantStructType<UA_DateTime>(variant, UA_TYPES_DATETIME);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_GUID])
        {
            return ToVariantStructType<UA_Guid>(variant, UA_TYPES_GUID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_STRING])
        {
            return ToVariantStructType<UA_String>(variant, UA_TYPES_STRING);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_NODEID])
        {
            return ToVariantStructType<UA_NodeId>(variant, UA_TYPES_NODEID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_EXPANDEDNODEID])
        {
            return ToVariantStructType<UA_ExpandedNodeId>(variant, UA_TYPES_EXPANDEDNODEID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_QUALIFIEDNAME])
        {
            return ToVariantStructType<UA_QualifiedName>(variant, UA_TYPES_QUALIFIEDNAME);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
        {
            return ToVariantStructType<UA_LocalizedText>(variant, UA_TYPES_LOCALIZEDTEXT);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_VARIANT])
        {
            return ToVariantStructType<UA_Variant>(variant, UA_TYPES_VARIANT);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_StructureDefinition>(*static_cast<UA_StructureDefinition*>(variant.data), UA_TYPES_STRUCTUREDEFINITION))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION])
        {
            return std::optional<VariantsOfAttr>{VariantsOfAttr(UATypesContainer<UA_EnumDefinition>(*static_cast<UA_EnumDefinition*>(variant.data), UA_TYPES_ENUMDEFINITION))};
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DIAGNOSTICINFO])
        {
            return ToVariantStructType<UA_DiagnosticInfo>(variant, UA_TYPES_DIAGNOSTICINFO);
        }
    }
    else
    {
        // Массивы включая многомерные
        // Простые типы
        if (variant.type == &UA_TYPES[UA_TYPES_BOOLEAN])
        {
            return ToVariantPlainTypeArray<UA_Boolean>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_SBYTE])
        {
            return ToVariantPlainTypeArray<UA_SByte>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_BYTE])
        {
            return ToVariantPlainTypeArray<UA_Byte>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT16])
        {
            return ToVariantPlainTypeArray<UA_Int16>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT16])
        {
            return ToVariantPlainTypeArray<UA_UInt16>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT32])
        {
            return ToVariantPlainTypeArray<UA_Int32>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT32])
        {
            return ToVariantPlainTypeArray<UA_UInt32>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_INT64])
        {
            return ToVariantPlainTypeArray<UA_Int64>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_UINT64])
        {
            return ToVariantPlainTypeArray<UA_UInt64>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_FLOAT])
        {
            return ToVariantPlainTypeArray<UA_Float>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DOUBLE])
        {
            return ToVariantPlainTypeArray<UA_Double>(variant);
        }
        // Структурные типы
        if (variant.type == &UA_TYPES[UA_TYPES_STATUSCODE])
        {
            // Исключение, так-как кастомная структура с одним полем для идентификации типа.
            return ToVariantPlainTypeArray<StatusCode>(variant);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_BYTESTRING])
        {
            // Исключение, так-как кастомная структура с одним полем для идентификации типа.
            return ToVariantStructTypeArray<ByteString>(variant, UA_TYPES_BYTESTRING);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DATETIME])
        {
            return ToVariantStructTypeArray<UA_DateTime>(variant, UA_TYPES_DATETIME);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_GUID])
        {
            return ToVariantStructTypeArray<UA_Guid>(variant, UA_TYPES_GUID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_STRING])
        {
            return ToVariantStructTypeArray<UA_String>(variant, UA_TYPES_STRING);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_NODEID])
        {
            return ToVariantStructTypeArray<UA_NodeId>(variant, UA_TYPES_NODEID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_EXPANDEDNODEID])
        {
            return ToVariantStructTypeArray<UA_ExpandedNodeId>(variant, UA_TYPES_EXPANDEDNODEID);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_QUALIFIEDNAME])
        {
            return ToVariantStructTypeArray<UA_QualifiedName>(variant, UA_TYPES_QUALIFIEDNAME);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
        {
            return ToVariantStructTypeArray<UA_LocalizedText>(variant, UA_TYPES_LOCALIZEDTEXT);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_VARIANT])
        {
            return ToVariantStructTypeArray<UA_Variant>(variant, UA_TYPES_VARIANT);
        }
        if (variant.type == &UA_TYPES[UA_TYPES_DIAGNOSTICINFO])
        {
            return ToVariantStructTypeArray<UA_DiagnosticInfo>(variant, UA_TYPES_DIAGNOSTICINFO);
        }
    }

    return {std::nullopt};
}

} // namespace nodesetexporter::open62541::typealiases