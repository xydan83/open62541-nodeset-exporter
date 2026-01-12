//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_STATUSES_H
#define NODESETEXPORTER_COMMON_STATUSES_H

#include <cstdint>

namespace nodesetexporter::common::statuses
{

/**
 * @brief Statuses of the results of method execution. Can be expanded if necessary.
 */
template <class TReserveCodeType = int64_t>
class StatusResults
{
public:
    enum Status : uint8_t
    {
        Good,
        Fail
    };

    // todo Detail the list on problems
    enum SubStatus : uint8_t
    {
        No,
        FailedCheckNs0StartNodes, // An error in checking the starting assemblies for the ability to work with NS = 0 space.
        EmptyNodeIdList, // An empty list of nodes
        GetAliasesFail, // Error in obtaining alias of nodes
        ExportNodesFail, // Error unloading of export of nodes
        GetNodesDataFail, // Error in obtaining these attributes of nodes
        GetNodeClassesFail, // Error in obtaining classes of nodes
        ExportAliasesFail, // Alias export error
        EndFail, // Error in completing the formation of export unloading
        BeginFail, // Error forming an unloading title
        GetNamespacesFail, // Error in obtaining nodes spaces
        ExportNamespacesFail // Error for the formation of export unloading of nodes spaces
    };

    StatusResults(Status status) // NOLINT(google-explicit-constructor)
        : m_status(status)
    {
    }

    StatusResults(Status status, SubStatus sub_status)
        : m_status(status)
        , m_sub_status(sub_status)
    {
    }

    StatusResults(Status status, SubStatus sub_status, TReserveCodeType reserve_code)
        : m_status(status)
        , m_sub_status(sub_status)
        , m_reserve_code(reserve_code)
    {
    }

    bool operator==(Status status)
    {
        return m_status == status;
    }

    bool operator!=(Status status)
    {
        return m_status != status;
    }

    bool operator==(const StatusResults& stat_res) const = default;
    bool operator!=(const StatusResults& stat_res) const = default;

    [[nodiscard]] Status GetStatus() const
    {
        return m_status;
    }

    [[nodiscard]] SubStatus GetSubStatus() const
    {
        return m_sub_status;
    }

    [[nodiscard]] TReserveCodeType GetReserveCode() const
    {
        return m_reserve_code;
    }

private:
    Status m_status = Status::Fail;
    SubStatus m_sub_status = SubStatus::No;
    TReserveCodeType m_reserve_code = 0;
};

} // namespace nodesetexporter::common::statuses

#endif // NODESETEXPORTER_COMMON_STATUSES_H
