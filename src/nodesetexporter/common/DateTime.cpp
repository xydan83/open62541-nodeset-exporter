//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include <nodesetexporter/common/DateTime.h>

#include <iomanip>

namespace nodesetexporter::common
{

std::time_t GetSystemClockNow()
{
    const std::chrono::time_point now{std::chrono::system_clock::now()};
    return std::chrono::system_clock::to_time_t(now);
}

const tm* common::DateTime::DateTimeUTCNow()
{
    std::time_t hms = GetSystemClockNow();
    return gmtime(&hms);
}

const tm* common::DateTime::DateTimeLocalNow()
{
    std::time_t hms = GetSystemClockNow();
    return localtime(&hms);
}

tm DateTime::GetDateTimeFromString(const std::string& date_time_str, const std::string& format)
{
    std::tm date_time = {};
    std::istringstream date_time_stream(date_time_str);
    date_time_stream >> std::get_time(&date_time, format.c_str());
    if (date_time_stream.fail())
    {
        throw std::runtime_error("GetDateTimeFromString parse failed");
    }
    return date_time;
}

std::string DateTime::GetDateTimeToString(const tm& time, const std::string& format)
{
    std::ostringstream date_time_o;
    date_time_o << std::put_time(&time, format.c_str());
    return date_time_o.str();
}

std::string DateTime::GetTimeLocalToString(time_t time, const std::string& format)
{
    tm* time_info = localtime(&time);
    return GetDateTimeToString(*time_info, format);
}

} // namespace nodesetexporter::common