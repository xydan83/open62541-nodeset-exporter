//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/logger/StdLog.h"

#include <doctest/doctest.h>

using nodesetexporter::common::LogLevel;
using nodesetexporter::logger::ConsoleLogger;

TEST_SUITE("nodesetexporter::logger")
{
    TEST_CASE("nodesetexporter::logger::ConsoleLogger")
    {
        SUBCASE("Checking for Exceptions in Output")
        {
            ConsoleLogger logger("test");
            logger.SetLevel(LogLevel::All);
            CHECK_NOTHROW(logger.Trace("Check Trace message."));
            CHECK_NOTHROW(logger.Debug("Check Debug message."));
            CHECK_NOTHROW(logger.Info("Check Info message."));
            CHECK_NOTHROW(logger.Warning("Check Warning message."));
            CHECK_NOTHROW(logger.Error("Check Error message."));
            CHECK_NOTHROW(logger.Critical("Check Critical message."));
        }
    }
}