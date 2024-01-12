//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_STATUSES_H
#define NODESETEXPORTER_COMMON_STATUSES_H

namespace nodesetexporter::common::statuses
{

/**
 * @brief Statuses of the results of method execution. Can be expanded if necessary.
 */
enum StatusResults
{
    Good,
    Fail
};

} // namespace nodesetexporter::common::statuses

#endif // NODESETEXPORTER_COMMON_STATUSES_H
