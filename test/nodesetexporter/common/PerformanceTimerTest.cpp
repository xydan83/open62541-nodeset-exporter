//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/common/PerformanceTimer.h"

#include <doctest/doctest.h>
#ifdef USING_FMT
#include <fmt/format.h>
#endif

#include <unistd.h>

using PerformanceTimer = nodesetexporter::common::PerformanceTimer;

namespace
{
#ifdef USING_FMT
template <typename... TArgs>
void CheckToStdFormatEn(fmt::format_string<TArgs...> fmt, TArgs&&... args)
{
    auto text = fmt::format(fmt, std::forward<TArgs>(args)...);
    CHECK_NE(text, "");
    MESSAGE(text);
};

template <typename... TArgs>
void CheckToStdFormatDis(fmt::format_string<TArgs...> fmt, TArgs&&... args)
{
    auto text = fmt::format(fmt, std::forward<TArgs>(args)...);
    CHECK_EQ(text, "");
    MESSAGE(text);
};
#endif
} // namespace

TEST_SUITE("nodesetexporter::common")
{
    TEST_CASE("nodesetexporter::common::PerformanceTimer")
    {
        SUBCASE("Start-Stop-Reset")
        {
            SUBCASE("0:0:0.250")
            {
                PerformanceTimer timer;
                usleep(250000);
                std::chrono::milliseconds result;
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                std::string str_result;
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);

                CHECK_NOTHROW(timer.Reset());
                usleep(250000);
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);
            }
            SUBCASE("0:0:1.0")
            {
                PerformanceTimer timer;
                sleep(1);
                std::chrono::milliseconds result;
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                std::string str_result;
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);

                CHECK_NOTHROW(timer.Reset());
                sleep(1);
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);
            }
            SUBCASE("0:0:1.500")
            {
                PerformanceTimer timer;
                usleep(1500000);
                std::chrono::milliseconds result;
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                std::string str_result;
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);

                CHECK_NOTHROW(timer.Reset());
                usleep(1500000);
                CHECK_NOTHROW(result = timer.GetTimeElapsed());
                CHECK_NOTHROW(str_result = timer.TimeToString(result));
                CHECK_NE(str_result, "");
                MESSAGE(str_result);
            }
        }
#ifdef PERFORMANCE_TIMER_ENABLED
        SUBCASE("Macro Test")
        {
            SUBCASE("The timer is activated.")
            {
                auto timer = PREPARE_TIMER(true);
                usleep(1231000);
                auto result = GET_TIME_ELAPSED(timer);
                CHECK_NE(result, "");
                MESSAGE(result);

                RESET_TIMER(timer);
                usleep(31000);
                result = GET_TIME_ELAPSED(timer);
                CHECK_NE(result, "");
                MESSAGE(result);

                RESET_TIMER(timer);
                usleep(741000);
                result = GET_TIME_ELAPSED(timer);
                CHECK_NE(result, "");
                MESSAGE(result);
            }

            SUBCASE("The timer is deactivated.")
            {
                auto timer = PREPARE_TIMER(false);
                RESET_TIMER(timer);
                usleep(1231000);
                auto result = GET_TIME_ELAPSED(timer);
                CHECK_EQ(result, "");
                MESSAGE(result);

                RESET_TIMER(timer);
                usleep(31000);
                result = GET_TIME_ELAPSED(timer);
                CHECK_EQ(result, "");
                MESSAGE(result);

                RESET_TIMER(timer);
                usleep(51000);
                result = GET_TIME_ELAPSED(timer);
                CHECK_EQ(result, "");
                MESSAGE(result);
            }

            SUBCASE("Lots of different timers.")
            {
                auto timer = PREPARE_TIMER(true);
                usleep(231000);
                auto result1 = GET_TIME_ELAPSED(timer);
                CHECK_NE(result1, "");
                MESSAGE(result1);

                auto timer2 = PREPARE_TIMER(true);
                usleep(342142);
                auto result2 = GET_TIME_ELAPSED(timer2);
                CHECK_NE(result2, "");
                MESSAGE(result2);

                auto timer3 = PREPARE_TIMER(true);
                usleep(1412142);
                auto result3 = GET_TIME_ELAPSED(timer3);
                CHECK_NE(result3, "");
                MESSAGE(result3);
            }

#ifdef USING_FMT
            SUBCASE("Macro Test for FMT")
            {
                auto timer = PREPARE_TIMER(true);
                usleep(1231000);
                GET_TIME_ELAPSED_FMT_FORMAT(timer, CheckToStdFormatEn, "Time: ", " sec. 1");

                RESET_TIMER(timer);
                usleep(31000);
                GET_TIME_ELAPSED_FMT_FORMAT(timer, CheckToStdFormatEn, "Time: ", " sec. 2");

                RESET_TIMER(timer);
                usleep(1462423);
                GET_TIME_ELAPSED_FMT_FORMAT(timer, CheckToStdFormatEn, "Time: ", " sec. 3");

                timer = PREPARE_TIMER(false);
                usleep(532423);
                GET_TIME_ELAPSED_FMT_FORMAT(timer, CheckToStdFormatDis, "Time: ", " sec. 4");
            }
#endif
        }
#endif
    }
}