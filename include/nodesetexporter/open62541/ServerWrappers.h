//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_OPEN62541_SERVERWRAPPERS_H
#define NODESETEXPORTER_OPEN62541_SERVERWRAPPERS_H

#include "nodesetexporter/interfaces/IOpen62541.h"

#include <open62541/server.h>

namespace nodesetexporter::open62541
{

using nodesetexporter::interfaces::IOpen62541;
using nodesetexporter::interfaces::LoggerBase;
using nodesetexporter::interfaces::StatusResults;
using nodesetexporter::interfaces::UATypesContainer;
using nodesetexporter::interfaces::VariantsOfAttr;

class Open62541ServerWrapper final : public IOpen62541
{
public:
    explicit Open62541ServerWrapper(UA_Server& ua_server, LoggerBase& logger)
        : IOpen62541(logger)
        , m_ua_server(ua_server)
    {
        throw std::runtime_error("Not implemented. Created for future implementation.");
    }
    ~Open62541ServerWrapper() override = default;
    Open62541ServerWrapper(Open62541ServerWrapper&) = delete;
    Open62541ServerWrapper(Open62541ServerWrapper&&) = delete;
    Open62541ServerWrapper& operator=(const Open62541ServerWrapper& obj) = delete;
    Open62541ServerWrapper& operator=(Open62541ServerWrapper&& obj) = delete;

    __attribute__((noreturn)) StatusResults ReadNodeClasses(std::vector<NodeClassesRequestResponse>& /*node_class_structure_lists*/) override
    {
        throw std::runtime_error("Not implemented");
    }
    __attribute__((noreturn)) StatusResults ReadNodeReferences(std::vector<NodeReferencesRequestResponse>& /*node_references_structure_lists*/) override
    {
        throw std::runtime_error("Not implemented");
    }
    __attribute__((noreturn)) StatusResults ReadNodesAttributes(std::vector<NodeAttributesRequestResponse>& /*node_attr_structure_lists*/, size_t /*attr_sum*/) override
    {
        throw std::runtime_error("Not implemented");
    }
    __attribute__((noreturn)) StatusResults ReadNodeDataValue(const UATypesContainer<UA_ExpandedNodeId>& /*node_id*/, UATypesContainer<UA_Variant>& /*data_value*/) override
    {
        throw std::runtime_error("Not implemented");
    }
    UA_Server& m_ua_server;
};

} // namespace nodesetexporter::open62541

#endif // NODESETEXPORTER_OPEN62541_SERVERWRAPPERS_H
