//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/common/DateTime.h"
#include "nodesetexporter/common/Strings.h"

#include <cassert>
#include <iomanip>

namespace nodesetexporter::common
{

namespace
{
std::time_t GetSystemClockNow()
{
    const std::chrono::time_point now{std::chrono::system_clock::now()};
    return std::chrono::system_clock::to_time_t(now);
}
} // namespace

const tm* common::DateTime::DateTimeUTCNow()
{
    const std::time_t hms = GetSystemClockNow();
    return gmtime(&hms);
}

const tm* common::DateTime::DateTimeLocalNow()
{
    const std::time_t hms = GetSystemClockNow();
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

// NOLINTBEGIN

// C-style code...
uint8_t DateTime::XmlEncodePrintNumber(int32_t n, char* pos, uint8_t min_digits)
{
    char digits[10];
    uint8_t len = 0;
    /* Handle negative values */
    if (n < 0)
    {
        pos[len++] = '-';
        n = -n;
    }

    /* Extract the digits */
    uint8_t i = 0;
    for (; i < min_digits || n > 0; i++)
    {
        digits[i] = (char)((n % 10) + '0');
        n /= 10;
    }

    /* Print in reverse order and return */
    for (; i > 0; i--)
        pos[len++] = digits[i - 1];
    return len;
}

/* Have some slack at the end. E.g. for negative and very long years. */
constexpr auto UA_XML_DATETIME_LENGTH = 40;

std::string DateTime::UADateTimeToString(UA_DateTime date_time)
{
    UA_DateTimeStruct tSt = UA_DateTime_toStruct(date_time);

    /* Format: -yyyy-MM-dd'T'HH:mm:ss.SSSSSSSSS'Z' is used. max 31 bytes.
     * Note the optional minus for negative years. */
    char buffer[UA_XML_DATETIME_LENGTH];
    char* pos = buffer;
    pos += XmlEncodePrintNumber(tSt.year, pos, 4);
    *(pos++) = '-';
    pos += XmlEncodePrintNumber(tSt.month, pos, 2);
    *(pos++) = '-';
    pos += XmlEncodePrintNumber(tSt.day, pos, 2);
    *(pos++) = 'T';
    pos += XmlEncodePrintNumber(tSt.hour, pos, 2);
    *(pos++) = ':';
    pos += XmlEncodePrintNumber(tSt.min, pos, 2);
    *(pos++) = ':';
    pos += XmlEncodePrintNumber(tSt.sec, pos, 2);
    *(pos++) = '.';
    pos += XmlEncodePrintNumber(tSt.milliSec, pos, 3);
    pos += XmlEncodePrintNumber(tSt.microSec, pos, 3);
    pos += XmlEncodePrintNumber(tSt.nanoSec, pos, 3);

    assert(pos <= &buffer[UA_XML_DATETIME_LENGTH]);

    /* Remove trailing zeros */
    pos--;
    while (*pos == '0')
        pos--;
    if (*pos == '.')
        pos--;

    *(++pos) = 'Z';
    UA_String str = {((uintptr_t)pos - (uintptr_t)buffer) + 1, (UA_Byte*)buffer};

    return UaStringToStdString(str);
}
// NOLINTEND
} // namespace nodesetexporter::common