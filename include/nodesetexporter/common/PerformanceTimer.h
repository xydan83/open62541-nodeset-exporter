//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_PERFORMANCETIMER_H
#define NODESETEXPORTER_COMMON_PERFORMANCETIMER_H

#ifdef PERFORMANCE_TIMER_ENABLED // NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define PREPARE_TIMER(is_enabled) ((is_enabled) ? std::make_unique<PerformanceTimer>() : std::unique_ptr<PerformanceTimer>())

#define RESET_TIMER(perf_timer)                                                                                                                                                                        \
    ({                                                                                                                                                                                                 \
        if (perf_timer)                                                                                                                                                                                \
        {                                                                                                                                                                                              \
            (perf_timer)->Reset();                                                                                                                                                                     \
        }                                                                                                                                                                                              \
    })
#define GET_TIME_ELAPSED(perf_timer) ((perf_timer) ? PerformanceTimer::TimeToString((perf_timer)->GetTimeElapsed()) : "")

#ifdef USING_FMT
#define GET_TIME_ELAPSED_FMT_FORMAT(perf_timer, func, front_text, back_text)                                                                                                                           \
    ({                                                                                                                                                                                                 \
        if (perf_timer)                                                                                                                                                                                \
        {                                                                                                                                                                                              \
            (func)("{}{}{}", (front_text), PerformanceTimer::TimeToString((perf_timer)->GetTimeElapsed()), back_text);                                                                                 \
        }                                                                                                                                                                                              \
    })
#else
#define GET_TIME_ELAPSED_FMT_FORMAT(...) (void())
#endif

#else
#define PREPARE_TIMER(...) (nullptr)
#define RESET_TIMER(...) ("")
#define GET_TIME_ELAPSED(...) (void())
#define GET_TIME_ELAPSED_FMT_FORMAT(...) (void())
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

#include <array>
#include <chrono>
#include <iostream>
#include <memory>

namespace nodesetexporter::common
{

/**
 * A simple productivity timer. Resolution is milliseconds.
 */
class PerformanceTimer final
{
private:
    using Clock = std::chrono::steady_clock;
    using MilliSec = std::chrono::milliseconds;
    using StadyClockPoint = std::chrono::time_point<std::chrono::steady_clock>;

public:
    /**
     * @brief Creates a timer object and marks the starting time point.
     */
    PerformanceTimer()
    {
        m_time_start_point = Clock::now();
    }

    /**
     * @brief Restarts the timer.
     */
    [[maybe_unused]] void Reset()
    {
        m_time_start_point = Clock::now();
    }

    /**
     * @brief Stops the timer.
     * @return Returns the operating time of the tested section in milliseconds.
     */
    [[nodiscard]] MilliSec GetTimeElapsed()
    {
        return std::chrono::duration_cast<MilliSec>(Clock::now() - m_time_start_point);
    }

    /**
     * @brief Helper method that converts an object to ms. to the format string HH:MM:SS.MS
     * @param milli_sec An object representing time in ms.
     * @return Text representation of time in HH:MM:SS.MS format.
     */
    static std::string TimeToString(MilliSec milli_sec)
    {
        // todo Add as an option a function implemented via fmt::format
        std::string ch_buf;
        ch_buf.resize(buffer_chars);
        auto resize = std::sprintf( // NOLINT(cppcoreguidelines-pro-type-vararg)
            ch_buf.data(),
            "%.2ld:%.2ld:%.2ld.%.3ld",
            std::chrono::hh_mm_ss(milli_sec).hours().count(),
            std::chrono::hh_mm_ss(milli_sec).minutes().count(),
            std::chrono::hh_mm_ss(milli_sec).seconds().count(),
            std::chrono::hh_mm_ss(milli_sec).subseconds().count());
        ch_buf.resize(resize);
        return ch_buf;
    }

private:
    StadyClockPoint m_time_start_point;
    static constexpr auto buffer_chars = 16;
};

} // namespace nodesetexporter::common

#endif // NODESETEXPORTER_COMMON_PERFORMANCETIMER_H