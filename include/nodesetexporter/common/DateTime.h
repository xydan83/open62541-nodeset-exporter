//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_COMMON_DATETIME_H
#define NODESETEXPORTER_COMMON_DATETIME_H

#include <chrono>

namespace nodesetexporter::common
{

class DateTime final
{
public:
    /**
     * @brief Getting a date and time in UTC format. Not streamlined.
     * @return The pointer to the static internal object STD :: TM in case of success or zero pointer otherwise.
     * The structure can be used jointly std::gmtime, std::localtime and std::ctime and can be rewritten at every call.
     * @remark Note 2: The C++20 calendar types are supported since 11.1, time zones and UTC are supported since 13.1, and chrono::parse is supported since 14.1.
     *         https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2020
     */
    static tm const* DateTimeUTCNow();

    /**
     * @brief Obtaining a date and time in a local format. Not streamlined.
     * @return The pointer to the static internal object STD :: TM in case of success or zero pointer otherwise.
     * The structure can be used jointly std::gmtime, std::localtime and std::ctime and can be rewritten at every call.
     * @remark Note 2: The C++20 calendar types are supported since 11.1, time zones and UTC are supported since 13.1, and chrono::parse is supported since 14.1.
     *         https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2020
     */
    static tm const* DateTimeLocalNow();

    /**
     * @brief The method converts the date in the text format into C structure tm.
     * @param date_time_str Date and time in text format.
     * @param format The format of the presented date and time.
     * @return C the structure of the date and time.
     * @idlexcept If parsing from the text fails, an exception to the type Runtime_error will be excluded.
     */
    static tm GetDateTimeFromString(const std::string& date_time_str, const std::string& format);

    /**
     * @brief The method converts the date in the structure of tm into a text form of format.
     * @param time The completed structure of the description of the date-time.
     * @param format The format of the text description of the date-time. By default - Sun Oct 17 04:41:13 2010 (Locale Dependent)
     * @return Date and time in text format.
     */
    static std::string GetDateTimeToString(const tm& time, const std::string& format = "%c");

    /**
     * @brief Translation of time from seconds from the beginning of the era in the current time zone in a line in a given format
     * @param time Time in seconds from the beginning of the epoch.
     * @param format The format of the text description of the date-time. By default - Sun Oct 17 04:41:13 2010 (Locale Dependent)
     * @return Date and time in text format.
     */
    static std::string GetTimeLocalToString(time_t time, const std::string& format = "%c");
};

} // namespace nodesetexporter::common

#endif // NODESETEXPORTER_COMMON_DATETIME_H
