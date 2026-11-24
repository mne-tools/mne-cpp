//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     fiffanonymizer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
 * @date     August, 2019
 * @brief    FiffAnonymizer class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"

#include <mne/base/base_global.h>
#include <mne/base/byte_stream.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_dir_entry.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <format>
#include <regex>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr std::int64_t kSecsPerDay = 86400;
constexpr std::int64_t kJulianDayOfEpoch = 2440588; // Julian day of 1970-01-01

// QDate's range is much wider than year_month_day's; dates outside it are not shown, as QDate would show them oddly
constexpr std::int64_t kMaxShownDays = 11'000'000;

template<typename Function, typename... Args>
void notify(const Function& function, Args&&... args)
{
    if (function) {
        function(std::forward<Args>(args)...);
    }
}

std::optional<std::tm> localTm(std::int64_t secs)
{
    const auto time = static_cast<std::time_t>(secs);
    std::tm tm{};
#ifdef _WIN32
    if (localtime_s(&tm, &time) != 0) {
        return std::nullopt;
    }
#else
    if (localtime_r(&time, &tm) == nullptr) {
        return std::nullopt;
    }
#endif
    return tm;
}

std::int64_t daysFromCivil(int year, int month, int day)
{
    using namespace std::chrono;
    const sys_days first{std::chrono::year{year} / std::chrono::month{static_cast<unsigned>(month)} / 1};
    return first.time_since_epoch().count() + day - 1;
}

// Wall-clock seconds of a local time, as if the local time were UTC
std::int64_t wallSecs(const std::tm& tm)
{
    return daysFromCivil(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday) * kSecsPerDay + tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
}

// Seconds the local time zone is ahead of UTC at @p secs; before 1970, where Windows has no local time, that of 1970
std::int64_t utcOffset(std::int64_t secs)
{
    std::optional<std::tm> tm = localTm(secs);
    if (!tm) {
        secs = 0;
        tm = localTm(secs);
    }
    return tm ? wallSecs(*tm) - secs : 0;
}

// Seconds since the epoch of a local wall-clock time, given as if it were UTC
std::int64_t secsFromWall(std::int64_t wall)
{
    std::int64_t secs = wall - utcOffset(wall);
    secs = wall - utcOffset(secs);
    return secs;
}

// Local date and time like QDateTime::toString("dd.MM.yyyy hh:mm:ss.zzz t")
std::string formatLocalDateTime(std::int64_t secs)
{
    const std::optional<std::tm> tm = localTm(secs);
    if (!tm) {
        return {};
    }
    std::array<char, 64> zone{};
    const std::size_t length = std::strftime(zone.data(), zone.size(), "%Z", &*tm);
    return std::format("{:02}.{:02}.{:04} {:02}:{:02}:{:02}.000 {}",
                       tm->tm_mday,
                       tm->tm_mon + 1,
                       tm->tm_year + 1900,
                       tm->tm_hour,
                       tm->tm_min,
                       tm->tm_sec,
                       std::string_view(zone.data(), length));
}

// A date like QDate::toString("dd.MM.yyyy"); empty for a date QDate would hold invalid
std::string formatJulianDay(std::optional<std::int64_t> julianDay)
{
    if (!julianDay) {
        return {};
    }
    const std::int64_t days = *julianDay - kJulianDayOfEpoch;
    if (days < -kMaxShownDays || days > kMaxShownDays) {
        return {};
    }
    const std::chrono::year_month_day date{std::chrono::sys_days{std::chrono::days{days}}};
    return std::format("{:02}.{:02}.{:04}",
                       static_cast<unsigned>(date.day()),
                       static_cast<unsigned>(date.month()),
                       static_cast<int>(date.year()));
}

// Valid like a QDate, which has no year 0
bool isValidDate(const std::chrono::year_month_day& date)
{
    return date.ok() && date.year() != std::chrono::year{0};
}

std::optional<std::int64_t> julianDay(const std::chrono::year_month_day& date)
{
    if (!isValidDate(date)) {
        return std::nullopt;
    }
    return std::chrono::sys_days{date}.time_since_epoch().count() + kJulianDayOfEpoch;
}

// A date written DDMMYYYY, like QDate::fromString(text, "ddMMyyyy"); not ok() if it is none
std::chrono::year_month_day parseDate(const std::string& text)
{
    static const std::regex pattern("^(\\d{2})(\\d{2})(\\d{4})$");
    std::smatch match;
    if (!std::regex_match(text, match, pattern)) {
        return std::chrono::year_month_day{std::chrono::year{0}, std::chrono::month{0}, std::chrono::day{0}};
    }
    return std::chrono::year_month_day{std::chrono::year{std::stoi(match[3].str())},
                                       std::chrono::month{static_cast<unsigned>(std::stoi(match[2].str()))},
                                       std::chrono::day{static_cast<unsigned>(std::stoi(match[1].str()))}};
}

// UTF-8 with every byte that does not start a valid sequence replaced by U+FFFD, as QString::fromUtf8 decodes it
std::string sanitizedUtf8(std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = 0;
        std::uint32_t minimum = 0;
        if (lead < 0x80) {
            length = 1;
        } else if (lead >= 0xC2 && lead <= 0xDF) {
            length = 2;
            minimum = 0x80;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            length = 3;
            minimum = 0x800;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            length = 4;
            minimum = 0x10000;
        }
        bool valid = length > 0 && i + length <= text.size();
        std::uint32_t codePoint = length == 1 ? lead : (lead & (0x7Fu >> length));
        for (std::size_t k = 1; valid && k < length; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            valid = (next & 0xC0) == 0x80;
            codePoint = (codePoint << 6) | (next & 0x3Fu);
        }
        valid = valid && codePoint >= minimum && codePoint <= 0x10FFFF && (codePoint < 0xD800 || codePoint > 0xDFFF);
        if (valid) {
            result.append(text.substr(i, length));
            i += length;
        } else {
            result.append("\xEF\xBF\xBD");
            ++i;
        }
    }
    return result;
}

std::filesystem::path absolutePath(const std::string& utf8)
{
    std::error_code error;
    const std::filesystem::path path = std::filesystem::absolute(mne::path_from_utf8(utf8), error);
    return path.lexically_normal();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnonymizer::FiffAnonymizer()
: m_pTag(mne::FiffTag{})
, m_bFileInSet(false)
, m_bFileOutSet(false)
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bMNEEnvironmentMode(false)
, m_dMaxValidFiffVerion(1.3)
, m_sDefaultString("mne_anonymize")
, m_sDefaultShortString("mne-cpp")
, m_dDefaultDate(std::chrono::year{2000} / 1 / 1)
, m_dMeasurementDate(m_dDefaultDate)
, m_iMeasurementDateOffset(0)
, m_bUseMeasurementDateOffset(false)
, m_sFiffComment(m_sDefaultString)
, m_sFiffExperimenter(m_sDefaultString)
, m_iSubjectId(0)
, m_sSubjectFirstName(m_sDefaultString)
, m_sSubjectMidName("mne")
, m_sSubjectLastName(m_sDefaultString)
, m_dSubjectBirthday(std::chrono::year{2000} / 1 / 1)
, m_iSubjectBirthdayOffset(0)
, m_bUseSubjectBirthdayOffset(false)
, m_sSubjectComment(m_sDefaultString)
, m_iSubjectSex(0)
, m_iSubjectHand(0)
, m_fSubjectWeight(0.0)
, m_fSubjectHeight(0.0)
, m_sSubjectHisId(m_sDefaultString)
, m_iProjectId(0)
, m_sProjectName(m_sDefaultString)
, m_sProjectAim(m_sDefaultString)
, m_sProjectPersons(m_sDefaultString)
, m_sProjectComment(m_sDefaultString)
, m_sMNEWorkingDir(m_sDefaultString)
, m_sMNECommand(m_sDefaultString)
{
    //MAC addresses have 6 bytes. We use 2 more here to complete 2 int32 (2bytes) reads.
    //check->sometimes MAC address is stored in the 0-5 bytes some other times it
    //is stored in the 2-7 bytes. To do -> Check why!!!
    m_BDfltMAC[0] = 0;
    m_BDfltMAC[1] = 0;
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(const FiffAnonymizer& obj)
: m_pTag(obj.m_pTag)
, m_blockTypeList(obj.m_blockTypeList)
, m_fFileIn(obj.m_fFileIn)
, m_fFileOut(obj.m_fFileOut)
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bMNEEnvironmentMode(obj.m_bMNEEnvironmentMode)
, m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sDefaultShortString(obj.m_sDefaultShortString)
, m_dDefaultDate(obj.m_dDefaultDate)
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_iMeasurementDateOffset(obj.m_iMeasurementDateOffset)
, m_bUseMeasurementDateOffset(obj.m_bUseMeasurementDateOffset)
, m_sFiffComment(obj.m_sFiffComment)
, m_sFiffExperimenter(obj.m_sFiffExperimenter)
, m_iSubjectId(obj.m_iSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_iSubjectSex(obj.m_iSubjectSex)
, m_iSubjectHand(obj.m_iSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_sMNEWorkingDir(obj.m_sDefaultString)
, m_sMNECommand(obj.m_sDefaultString)
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer&& obj) noexcept
: m_pTag(std::move(obj.m_pTag))
, m_blockTypeList(std::move(obj.m_blockTypeList))
, m_fFileIn(std::move(obj.m_fFileIn))
, m_fFileOut(std::move(obj.m_fFileOut))
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bMNEEnvironmentMode(obj.m_bMNEEnvironmentMode)
, m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sDefaultShortString(obj.m_sDefaultShortString)
, m_dDefaultDate(obj.m_dDefaultDate)
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_iMeasurementDateOffset(obj.m_iMeasurementDateOffset)
, m_bUseMeasurementDateOffset(obj.m_bUseMeasurementDateOffset)
, m_sFiffComment(obj.m_sFiffComment)
, m_sFiffExperimenter(obj.m_sFiffExperimenter)
, m_iSubjectId(obj.m_iSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_iSubjectSex(obj.m_iSubjectSex)
, m_iSubjectHand(obj.m_iSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_sMNEWorkingDir(obj.m_sDefaultString)
, m_sMNECommand(obj.m_sDefaultString)
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];
}

//=============================================================================================================

int FiffAnonymizer::anonymizeFile()
{
    if (!m_bFileInSet) {
        mne::Log::critical("Input file has not been specified.");
        return 1;
    }

    if (!m_bFileOutSet) {
        mne::Log::critical("Output file has not been specified.");
        return 1;
    }

    printIfVerbose("Max. Valid Fiff version: " + mne::format_number(m_dMaxValidFiffVerion, 'g', 6));
    printIfVerbose(std::string("Build date and time: ") + mne::build_date_time());
    printIfVerbose(" ");

    if (openInOutStreams()) {
        return 1;
    }

    printIfVerbose("Reading info in the file.");
    m_blockTypeList.clear();
    processHeaderTags();

    while ((m_pTag->next != -1) && (!m_pInStream->byte_stream().at_end())) {
        readTag();
        censorTag();
        writeTag();
    }

    closeInOutStreams();

    notify(m_callbacks.outFileReady);

    return 0;
}

//=============================================================================================================

void FiffAnonymizer::setCallbacks(FiffAnonymizerCallbacks callbacks)
{
    m_callbacks = std::move(callbacks);
}

//=============================================================================================================

std::int64_t FiffAnonymizer::censorMeasurementDate(std::int64_t inSecs) const
{
    if (m_bUseMeasurementDateOffset) {
        // Days are subtracted from the local date, keeping the local time of day like QDateTime::addDays
        const std::int64_t wall = inSecs + utcOffset(inSecs);
        return secsFromWall(wall - static_cast<std::int64_t>(m_iMeasurementDateOffset) * kSecsPerDay);
    }
    if (!isValidDate(m_dMeasurementDate)) {
        return 0;
    }
    const std::int64_t days = std::chrono::sys_days{m_dMeasurementDate}.time_since_epoch().count();
    return secsFromWall(days * kSecsPerDay + 3600 + 60);
}

//=============================================================================================================

void FiffAnonymizer::censorTag()
{
    switch (m_pTag->kind) {
        //all these 'kinds' of tags contain a fileID struct, which contains info related to
        //measurement date
        case FIFF_FILE_ID:
        case FIFF_BLOCK_ID:
        case FIFF_PARENT_FILE_ID:
        case FIFF_PARENT_BLOCK_ID:
        case FIFF_REF_FILE_ID:
        case FIFF_REF_BLOCK_ID: {
            mne::FiffId inId = m_pTag->to_fiff_id();
            const std::int64_t inMeasDate = inId.time.secs;
            notify(m_callbacks.readingIdMeasurementDate, inMeasDate);

            const std::int64_t outMeasDate = censorMeasurementDate(inMeasDate);
            const bool outMeasDateValid = m_bUseMeasurementDateOffset || isValidDate(m_dMeasurementDate);

            mne::FiffId outId(inId);
            outId.machid[0] = m_BDfltMAC[0];
            outId.machid[1] = m_BDfltMAC[1];
            outId.time.secs = static_cast<int32_t>(outMeasDate);
            outId.time.usecs = 0;

            const int fiffIdSize(sizeof(inId) / sizeof(mne::fiff_int_t));
            mne::fiff_int_t outData[fiffIdSize];
            outData[0] = outId.version;
            outData[1] = outId.machid[0];
            outData[2] = outId.machid[1];
            outData[3] = outId.time.secs;
            outData[4] = outId.time.usecs;

            m_pTag->resize(fiffIdSize * sizeof(mne::fiff_int_t));
            memcpy(m_pTag->data(), reinterpret_cast<char*>(outData), fiffIdSize * sizeof(mne::fiff_int_t));
            printIfVerbose("MAC address in ID tag changed: " + inId.toMachidString() + " -> " + outId.toMachidString());
            printIfVerbose("Measurement date in ID tag changed: " + formatLocalDateTime(inMeasDate) + " -> " + (outMeasDateValid ? formatLocalDateTime(outMeasDate) : std::string()));
            break;
        }
        case FIFF_MEAS_DATE: {
            const std::int64_t inMeasDate = *m_pTag->to_int();
            notify(m_callbacks.readingFileMeasurementDate, inMeasDate);

            const std::int64_t outMeasDate = censorMeasurementDate(inMeasDate);
            const bool outMeasDateValid = m_bUseMeasurementDateOffset || isValidDate(m_dMeasurementDate);

            mne::fiff_int_t outData[1];
            outData[0] = static_cast<int32_t>(outMeasDate);
            memcpy(m_pTag->data(), reinterpret_cast<char*>(outData), sizeof(mne::fiff_int_t));
            printIfVerbose("Measurement date changed: " + formatLocalDateTime(inMeasDate) + " -> " + (outMeasDateValid ? formatLocalDateTime(outMeasDate) : std::string()));
            break;
        }
        case FIFF_COMMENT: {
            std::string inStr(tagText());
            notify(m_callbacks.readingFileComment, inStr);

            if (!m_blockTypeList.empty() && m_blockTypeList.back() == FIFFB_MEAS_INFO) {
                std::string outStr(m_sDefaultString);
                setTagText(outStr);
                printIfVerbose("Description of the measurement block changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        case FIFF_EXPERIMENTER: {
            std::string inStr(tagText());
            notify(m_callbacks.readingFileExperimenter, inStr);

            std::string outStr(m_sDefaultString);
            setTagText(outStr);
            printIfVerbose("Experimenter changed: " + inStr + " -> " + outStr);
            break;
        }
        case FIFF_SUBJ_ID: {
            int32_t inSubjID(*m_pTag->to_int());
            notify(m_callbacks.readingSubjectId, inSubjID);
            int32_t outSubjID(m_iSubjectId);
            memcpy(m_pTag->data(), &outSubjID, sizeof(int32_t));
            printIfVerbose(std::format("Subject ID changed: {} -> {}", inSubjID, outSubjID));
            break;
        }
        case FIFF_SUBJ_FIRST_NAME: {
            std::string inFirstName(tagText());
            notify(m_callbacks.readingSubjectFirstName, inFirstName);
            std::string outFirstName(m_sSubjectFirstName);
            setTagText(outFirstName);
            printIfVerbose("Subject first name changed: " + inFirstName + " -> " + outFirstName);
            break;
        }
        case FIFF_SUBJ_MIDDLE_NAME: {
            std::string inStr(tagText());
            notify(m_callbacks.readingSubjectMiddleName, inStr);
            std::string outStr(m_sSubjectMidName);
            setTagText(outStr);
            printIfVerbose("Subject middle name changed: " + inStr + " -> " + outStr);
            break;
        }
        case FIFF_SUBJ_LAST_NAME: {
            std::string inStr(tagText());
            notify(m_callbacks.readingSubjectLastName, inStr);
            std::string outStr(m_sSubjectLastName);
            setTagText(outStr);
            printIfVerbose("Subject last name changed: " + inStr + " -> " + outStr);
            break;
        }
        case FIFF_SUBJ_BIRTH_DAY: {
            const std::int64_t inBirthday = *m_pTag->to_julian();
            notify(m_callbacks.readingSubjectBirthday, inBirthday);
            std::optional<std::int64_t> outBirthday;

            if (m_bUseSubjectBirthdayOffset) {
                outBirthday = inBirthday - m_iSubjectBirthdayOffset;
            } else {
                outBirthday = julianDay(m_dSubjectBirthday);
            }

            mne::fiff_int_t outData[1];
            outData[0] = static_cast<int32_t>(outBirthday.value_or(0));
            memcpy(m_pTag->data(), reinterpret_cast<char*>(outData), sizeof(mne::fiff_int_t));
            printIfVerbose("Subject birthday date changed: " + formatJulianDay(inBirthday) + " -> " + formatJulianDay(outBirthday));
            break;
        }
        case FIFF_SUBJ_SEX: {
            int32_t inSubjectSex(*m_pTag->to_int());
            notify(m_callbacks.readingSubjectSex, inSubjectSex);
            if (m_bBruteMode) {
                int32_t outSubjSex(m_iSubjectSex);
                memcpy(m_pTag->data(), &outSubjSex, sizeof(int32_t));
                printIfVerbose("Subject sex changed: " + subjectSexToString(inSubjectSex) + " -> " + subjectSexToString(outSubjSex));
            }
            break;
        }
        case FIFF_SUBJ_HAND: {
            int32_t inSubjectHand(*m_pTag->to_int());
            notify(m_callbacks.readingSubjectHand, inSubjectHand);
            if (m_bBruteMode) {
                int32_t newSubjHand(m_iSubjectHand);
                memcpy(m_pTag->data(), &newSubjHand, sizeof(int32_t));
                printIfVerbose("Subject handedness changed: " + subjectHandToString(inSubjectHand) + " -> " + subjectHandToString(newSubjHand));
            }
            break;
        }
        case FIFF_SUBJ_WEIGHT: {
            float inWeight(*m_pTag->to_float());
            notify(m_callbacks.readingSubjectWeight, inWeight);
            if (m_bBruteMode) {
                float outWeight(m_fSubjectWeight);
                memcpy(m_pTag->data(), &outWeight, sizeof(float));
                printIfVerbose("Subject weight changed: " + mne::format_number(static_cast<double>(inWeight), 'g', 6) + " -> " + mne::format_number(static_cast<double>(outWeight), 'g', 6));
            }
            break;
        }
        case FIFF_SUBJ_HEIGHT: {
            float inHeight(*m_pTag->to_float());
            notify(m_callbacks.readingSubjectHeight, inHeight);
            if (m_bBruteMode) {
                float outHeight(m_fSubjectHeight);
                memcpy(m_pTag->data(), &outHeight, sizeof(float));
                printIfVerbose("Subject height changed: " + mne::format_number(static_cast<double>(inHeight), 'g', 6) + " -> " + mne::format_number(static_cast<double>(outHeight), 'g', 6));
            }
            break;
        }
        case FIFF_SUBJ_COMMENT: {
            std::string inStr(tagText());
            notify(m_callbacks.readingSubjectComment, inStr);
            std::string outStr(m_sSubjectComment);
            setTagText(outStr);
            printIfVerbose("Subject comment changed: " + inStr + " -> " + outStr);
            break;
        }
        case FIFF_SUBJ_HIS_ID: {
            std::string inSubjectHisId(tagText());
            notify(m_callbacks.readingSubjectHisId, inSubjectHisId);
            std::string outSubjectHisId(m_sSubjectHisId);
            setTagText(outSubjectHisId);
            printIfVerbose("Subject Hospital-ID(His Id) changed: " + inSubjectHisId + " -> " + outSubjectHisId);
            break;
        }
        case FIFF_PROJ_ID: {
            int32_t inProjID(*m_pTag->to_int());
            notify(m_callbacks.readingProjectId, inProjID);
            if (m_bBruteMode) {
                int32_t newProjID(m_iProjectId);
                memcpy(m_pTag->data(), &newProjID, sizeof(int32_t));
                printIfVerbose(std::format("ProjectID changed: {} -> {}", inProjID, newProjID));
            }
            break;
        }
        case FIFF_PROJ_NAME: {
            std::string inStr(tagText());
            notify(m_callbacks.readingProjectName, inStr);
            if (m_bBruteMode) {
                std::string outStr(m_sProjectName);
                setTagText(outStr);
                printIfVerbose("Project name changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        case FIFF_PROJ_AIM: {
            std::string inStr(tagText());
            notify(m_callbacks.readingProjectAim, inStr);
            if (m_bBruteMode) {
                std::string outStr(m_sProjectAim);
                setTagText(outStr);
                printIfVerbose("Project aim changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        case FIFF_PROJ_PERSONS: {
            std::string inStr(tagText());
            notify(m_callbacks.readingProjectPersons, inStr);
            std::string outStr(m_sProjectPersons);
            setTagText(outStr);
            printIfVerbose("Project persons changed: " + inStr + " -> " + outStr);
            break;
        }
        case FIFF_PROJ_COMMENT: {
            std::string inStr(tagText());
            notify(m_callbacks.readingProjectComment, inStr);
            if (m_bBruteMode) {
                std::string outStr(m_sProjectComment);
                setTagText(outStr);
                printIfVerbose("Project comment changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        case FIFF_MRI_PIXEL_DATA: {
            printIfVerbose("  ");
            printIfVerbose("Warning: The input fif file contains MRI data.");
            printIfVerbose("Warning: Beware that a subject''s face can be reconstructed from it");
            printIfVerbose("Warning: This software can not anonymize MRI data, at the moment.");
            printIfVerbose("  ");
            notify(m_callbacks.mriDataFoundInFile, true);
            break;
        }
        case FIFF_MNE_ENV_WORKING_DIR: {
            std::string inStr(tagText());
            notify(m_callbacks.readingMNEWorkingDir, inStr);
            if (m_bMNEEnvironmentMode || m_bBruteMode) {
                std::string outStr(m_sMNEWorkingDir);
                setTagText(outStr);
                printIfVerbose("MNE working directory info changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        case FIFF_MNE_ENV_COMMAND_LINE: {
            std::string inStr(tagText());
            notify(m_callbacks.readingMNECommandLine, inStr);
            if (m_bMNEEnvironmentMode || m_bBruteMode) {
                std::string outStr(m_sMNECommand);
                setTagText(outStr);
                printIfVerbose("MNE command line info changed: " + inStr + " -> " + outStr);
            }
            break;
        }
        default: {
        } //default
    } //switch
}

//=============================================================================================================

void FiffAnonymizer::readTag()
{
    m_pTag = m_pInStream->read_tag(-1);
    updateBlockTypeList();
}

//=============================================================================================================

void FiffAnonymizer::writeTag()
{
    //make output tag list linear
    if (m_pTag->next > 0) {
        m_pTag->next = FIFFV_NEXT_SEQ;
    }

    m_pOutStream->write_tag(*m_pTag, -1);
}

//=============================================================================================================

void FiffAnonymizer::processHeaderTags()
{
    readTag();

    if (checkValidFiffFormatVersion()) {
        printIfVerbose("Input file compatible with this version.");
    } else {
        mne::Log::critical("This file may not be compatible with this application. First tag is not a valid ID tag.");
    }

    censorTag();
    writeTag();

    // pointer to tag directory
    readTag();

    if (m_pTag->kind != FIFF_DIR_POINTER) {
        mne::Log::critical("This file may not be compatible with this application. Second tag is not a valid Tag directory pointer tag.");
    }
    int32_t newTagDirLoc(-1);
    memcpy(m_pTag->data(), &newTagDirLoc, sizeof(int32_t));

    censorTag();
    writeTag();

    //free list
    readTag();

    if ((m_pTag->kind == FIFF_FREE_LIST) && (*m_pTag->to_int() > 0)) {
        mne::Log::warning("This file contains a Free List of tags. It will not be copied to the output file.");
    } else {
        // output this tag, whatever kind it is, to the oupput file.
        censorTag();
        writeTag();
    }
}

//=============================================================================================================

void FiffAnonymizer::updateBlockTypeList()
{
    if (m_pTag->kind == FIFF_BLOCK_START) {
        m_blockTypeList.push_back(*m_pTag->to_int());
    }

    if (m_pTag->kind == FIFF_BLOCK_END && !m_blockTypeList.empty()) {
        m_blockTypeList.pop_back();
    }
}

//=============================================================================================================

bool FiffAnonymizer::checkValidFiffFormatVersion()
{
    if (m_pTag->kind == FIFF_FILE_ID) {
        mne::FiffId fileId = m_pTag->to_fiff_id();
        int inMayorVersion = static_cast<int>((static_cast<uint32_t>(fileId.version) & 0xFFFF0000) >> 16);
        int inMinorVersion = static_cast<int>(static_cast<uint32_t>(fileId.version) & 0x0000FFFF);
        double inVersion = inMayorVersion + inMinorVersion / 10.0;
        notify(m_callbacks.readingIdFileVersion, inVersion);
        notify(m_callbacks.readingIdMac, fileId.toMachidString());

        if (inVersion > m_dMaxValidFiffVerion) {
            return false;
        }
        return true;
    }
    return false;
}

//=============================================================================================================

int FiffAnonymizer::setInFile(const std::string& sFileIn)
{
    m_fFileIn = absolutePath(sFileIn);
    m_bFileInSet = true;
    return 0;
}

//=============================================================================================================

std::string FiffAnonymizer::getFileNameIn() const
{
    return mne::utf8_from_path(m_fFileIn);
}

//=============================================================================================================

int FiffAnonymizer::setOutFile(const std::string& sFileOut)
{
    const std::filesystem::path fileOut = absolutePath(sFileOut);
    if (fileOut == m_fFileIn) {
        mne::Log::warning("Both input and output file names are the same.");
        return 1;
    } else {
        m_fFileOut = fileOut;
    }
    m_bFileOutSet = true;
    return 0;
}

//=============================================================================================================

std::string FiffAnonymizer::getFileNameOut() const
{
    return mne::utf8_from_path(m_fFileOut);
}

//=============================================================================================================

void FiffAnonymizer::setVerboseMode(bool bFlag)
{
    m_bVerboseMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getVerboseMode() const
{
    return m_bVerboseMode;
}

//=============================================================================================================

void FiffAnonymizer::setMNEEnvironmentMode(bool bFlag)
{
    m_bMNEEnvironmentMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getMNEEnvironmentMode() const
{
    return m_bMNEEnvironmentMode;
}

//=============================================================================================================

void FiffAnonymizer::setBruteMode(bool bFlag)
{
    m_bBruteMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getBruteMode() const
{
    return m_bBruteMode;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDate(const std::chrono::year_month_day& d)
{
    m_dMeasurementDate = d;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDate(const std::string& sMeasDay)
{
    m_dMeasurementDate = parseDate(sMeasDay);
}

//=============================================================================================================

std::chrono::year_month_day FiffAnonymizer::getMeasurementDate() const
{
    return m_dMeasurementDate;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDateOffset(const int iMeasDayOffset)
{
    m_iMeasurementDateOffset = iMeasDayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setUseMeasurementDateOffset(bool b)
{
    m_bUseMeasurementDateOffset = b;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const std::string& sSubjBirthday)
{
    m_dSubjectBirthday = parseDate(sSubjBirthday);
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const std::chrono::year_month_day& dSubjBirthday)
{
    m_dSubjectBirthday = dSubjBirthday;
}

//=============================================================================================================

std::chrono::year_month_day FiffAnonymizer::getSubjectBirthday() const
{
    return m_dSubjectBirthday;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthdayOffset(int iSubjBirthdayOffset)
{
    m_iSubjectBirthdayOffset = iSubjBirthdayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setUseSubjectBirthdayOffset(bool b)
{
    m_bUseSubjectBirthdayOffset = b;
}

//=============================================================================================================

int FiffAnonymizer::getSubjectBirthdayOffset() const
{
    return m_iSubjectBirthdayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectHisId(const std::string& sSubjectHisId)
{
    m_sSubjectHisId = sSubjectHisId;
}

//=============================================================================================================

int FiffAnonymizer::openInOutStreams()
{
    const std::string fileIn = getFileNameIn();
    auto in = mne::FileByteStream::open(m_fFileIn, mne::FileByteStream::Mode::Read);
    if (!in) {
        mne::Log::critical("Problem opening the input file: {}", fileIn);
        return 1;
    }
    printIfVerbose("Input file opened correctly: " + fileIn);
    m_pInStream = std::make_unique<mne::FiffStream>(std::move(in), fileIn);

    const std::string fileOut = getFileNameOut();
    auto out = mne::FileByteStream::open(m_fFileOut, mne::FileByteStream::Mode::Write);
    if (!out) {
        mne::Log::critical("Problem opening the output file: {}", fileOut);
        return 1;
    }
    printIfVerbose("Output file opened correctly: " + fileOut);
    m_pOutStream = std::make_unique<mne::FiffStream>(std::move(out), fileOut);
    return 0;
}

//=============================================================================================================

int FiffAnonymizer::closeInOutStreams()
{
    if (m_pInStream->close()) {
        printIfVerbose("Input file closed.");
    } else {
        mne::Log::critical("Problem closing the input file: {}", getFileNameIn());
        return 1;
    }

    if (m_pOutStream->close()) {
        printIfVerbose("Output file closed.");
    } else {
        mne::Log::critical("Problem closing the output file: {}", getFileNameOut());
        return 1;
    }

    return 0;
}

//=============================================================================================================

std::string FiffAnonymizer::getSubjectHisID() const
{
    return m_sSubjectHisId;
}

//=============================================================================================================

int FiffAnonymizer::getMeasurementDayOffset() const
{
    return m_iMeasurementDateOffset;
}

//=============================================================================================================

bool FiffAnonymizer::getUseMeasurementDayOffset() const
{
    return m_bUseMeasurementDateOffset;
}

//=============================================================================================================

bool FiffAnonymizer::getUseSubjectBirthdayOffset() const
{
    return m_bUseSubjectBirthdayOffset;
}

//=============================================================================================================

bool FiffAnonymizer::isFileInSet() const
{
    return m_bFileInSet;
}

//=============================================================================================================

bool FiffAnonymizer::isFileOutSet() const
{
    return m_bFileOutSet;
}

//=============================================================================================================

std::string FiffAnonymizer::tagText() const
{
    const auto* text = reinterpret_cast<const char*>(m_pTag->data());
    return sanitizedUtf8(std::string_view(text, static_cast<std::size_t>(std::find(text, text + m_pTag->size(), '\0') - text)));
}

//=============================================================================================================

void FiffAnonymizer::setTagText(const std::string& text)
{
    m_pTag->resize(text.size());
    std::memcpy(m_pTag->data(), text.data(), text.size());
}

//=============================================================================================================

std::string FiffAnonymizer::subjectSexToString(int sexCode) const
{
    static const std::array<std::string_view, 3> subjectSexRefList = {"unknown", "male", "female"};

    if (sexCode > -1 && sexCode < static_cast<int>(subjectSexRefList.size())) {
        return std::string(subjectSexRefList[static_cast<std::size_t>(sexCode)]);
    } else {
        mne::Log::critical("Invalid subject sex code. [0 = unknown, 1 = male, 2 = female]. The code of the subject is: {}", sexCode);
        return "invalid-code";
    }
}

//=============================================================================================================

std::string FiffAnonymizer::subjectHandToString(int handCode) const
{
    static const std::array<std::string_view, 4> subjectHandRefList = {"unknown", "right", "left", "ambidextrous"};

    if ((handCode > -1) && (handCode < static_cast<int>(subjectHandRefList.size()))) {
        return std::string(subjectHandRefList[static_cast<std::size_t>(handCode)]);
    } else {
        mne::Log::critical("Invalid subject handedness code. [0 = unknown, 1 = right, 2 = left, 3 = ambidextrous]. The code of the subject is: {}", handCode);
        return "invalid-code";
    }
}

//=============================================================================================================

void FiffAnonymizer::printIfVerbose(const std::string& str) const
{
    if (m_bVerboseMode) {
        mne::Log::info("{}", str);
    }
}
