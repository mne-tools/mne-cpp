//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_path.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BIDSPath — entity-based construction and matching of BIDS-compliant paths and sidecar siblings.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_path.h"
#include "bids_const.h"

#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <format>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BIDSPath::BIDSPath(std::filesystem::path sRoot,
                   std::string sSubject,
                   std::string sSession,
                   std::string sTask,
                   std::string sDatatype,
                   std::string sSuffix,
                   std::string sExtension)
: m_sRoot(std::move(sRoot))
, m_sSubject(std::move(sSubject))
, m_sSession(std::move(sSession))
, m_sTask(std::move(sTask))
, m_sDatatype(std::move(sDatatype))
, m_sSuffix(std::move(sSuffix))
, m_sExtension(std::move(sExtension))
{
}

//=============================================================================================================
// Setters
//=============================================================================================================

void BIDSPath::set_root(std::filesystem::path sRoot)
{
    m_sRoot = std::move(sRoot);
}
void BIDSPath::set_subject(std::string sSubject)
{
    m_sSubject = std::move(sSubject);
}
void BIDSPath::set_session(std::string sSession)
{
    m_sSession = std::move(sSession);
}
void BIDSPath::set_task(std::string sTask)
{
    m_sTask = std::move(sTask);
}
void BIDSPath::set_acquisition(std::string sAcquisition)
{
    m_sAcquisition = std::move(sAcquisition);
}
void BIDSPath::set_run(std::string sRun)
{
    m_sRun = zero_pad(std::move(sRun));
}
void BIDSPath::set_processing(std::string sProcessing)
{
    m_sProcessing = std::move(sProcessing);
}
void BIDSPath::set_space(std::string sSpace)
{
    m_sSpace = std::move(sSpace);
}
void BIDSPath::set_recording(std::string sRecording)
{
    m_sRecording = std::move(sRecording);
}
void BIDSPath::set_split(std::string sSplit)
{
    m_sSplit = zero_pad(std::move(sSplit));
}
void BIDSPath::set_description(std::string sDescription)
{
    m_sDescription = std::move(sDescription);
}
void BIDSPath::set_datatype(std::string sDatatype)
{
    m_sDatatype = std::move(sDatatype);
}
void BIDSPath::set_suffix(std::string sSuffix)
{
    m_sSuffix = std::move(sSuffix);
}
void BIDSPath::set_extension(std::string sExtension)
{
    m_sExtension = std::move(sExtension);
}

//=============================================================================================================
// Getters
//=============================================================================================================

const std::filesystem::path& BIDSPath::root() const
{
    return m_sRoot;
}
const std::string& BIDSPath::subject() const
{
    return m_sSubject;
}
const std::string& BIDSPath::session() const
{
    return m_sSession;
}
const std::string& BIDSPath::task() const
{
    return m_sTask;
}
const std::string& BIDSPath::acquisition() const
{
    return m_sAcquisition;
}
const std::string& BIDSPath::run() const
{
    return m_sRun;
}
const std::string& BIDSPath::processing() const
{
    return m_sProcessing;
}
const std::string& BIDSPath::space() const
{
    return m_sSpace;
}
const std::string& BIDSPath::recording() const
{
    return m_sRecording;
}
const std::string& BIDSPath::split() const
{
    return m_sSplit;
}
const std::string& BIDSPath::description() const
{
    return m_sDescription;
}
const std::string& BIDSPath::datatype() const
{
    return m_sDatatype;
}
const std::string& BIDSPath::suffix() const
{
    return m_sSuffix;
}
const std::string& BIDSPath::extension() const
{
    return m_sExtension;
}

//=============================================================================================================
// Path construction
//=============================================================================================================

std::string BIDSPath::basename() const
{
    const std::array<std::pair<std::string_view, const std::string*>, 10> entities{{
        {"sub-", &m_sSubject},
        {"ses-", &m_sSession},
        {"task-", &m_sTask},
        {"acq-", &m_sAcquisition},
        {"run-", &m_sRun},
        {"proc-", &m_sProcessing},
        {"space-", &m_sSpace},
        {"recording-", &m_sRecording},
        {"split-", &m_sSplit},
        {"desc-", &m_sDescription},
    }};

    std::string name;
    const auto append = [&name](std::string_view part) {
        if (!name.empty())
            name += '_';
        name += part;
    };
    for (const auto& [key, value] : entities) {
        if (!value->empty())
            append(std::string(key) + *value);
    }
    if (!m_sSuffix.empty())
        append(m_sSuffix);

    return name + m_sExtension;
}

//=============================================================================================================

std::filesystem::path BIDSPath::directory() const
{
    std::filesystem::path dir = m_sRoot;

    if (!m_sSubject.empty())
        dir /= BASELIB::pathFromUtf8("sub-" + m_sSubject);

    if (!m_sSession.empty())
        dir /= BASELIB::pathFromUtf8("ses-" + m_sSession);

    if (!m_sDatatype.empty())
        dir /= BASELIB::pathFromUtf8(m_sDatatype);

    return dir;
}

//=============================================================================================================

std::filesystem::path BIDSPath::file_path() const
{
    return directory() / BASELIB::pathFromUtf8(basename());
}

//=============================================================================================================
// Convenience methods
//=============================================================================================================

BIDSPath BIDSPath::with_suffix(std::string sSuffix, std::string sExtension) const
{
    BIDSPath result(*this);
    result.m_sSuffix = std::move(sSuffix);
    result.m_sExtension = std::move(sExtension);
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::channels_tsv_path() const
{
    return with_suffix("channels", ".tsv");
}

//=============================================================================================================

BIDSPath BIDSPath::electrodes_tsv_path() const
{
    // Electrodes file typically doesn't include task entity
    BIDSPath result = with_suffix("electrodes", ".tsv");
    result.m_sTask.clear();
    result.m_sRun.clear();
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::coordsystem_json_path() const
{
    // Coordsystem file typically doesn't include task entity
    BIDSPath result = with_suffix("coordsystem", ".json");
    result.m_sTask.clear();
    result.m_sRun.clear();
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::events_tsv_path() const
{
    return with_suffix("events", ".tsv");
}

//=============================================================================================================

BIDSPath BIDSPath::sidecar_json_path() const
{
    return with_suffix(m_sSuffix.empty() ? m_sDatatype : m_sSuffix, ".json");
}

//=============================================================================================================

bool BIDSPath::exists() const
{
    std::error_code error;
    return std::filesystem::exists(file_path(), error);
}

//=============================================================================================================

bool BIDSPath::mkdirs() const
{
    std::error_code error;
    const std::filesystem::path dir = directory();
    std::filesystem::create_directories(dir, error);
    return std::filesystem::is_directory(dir, error);
}

//=============================================================================================================

std::vector<BIDSPath> BIDSPath::match() const
{
    // Like mne_bids.BIDSPath.match: every set entity must equal the file's, unset ones match anything
    std::vector<BIDSPath> results;
    std::error_code error;
    if (m_sRoot.empty() || !std::filesystem::is_directory(m_sRoot, error))
        return results;

    for (auto it = std::filesystem::recursive_directory_iterator(m_sRoot, error);
         it != std::filesystem::recursive_directory_iterator();
         it.increment(error)) {
        const std::string name = BASELIB::utf8FromPath(it->path().filename());
        if (name.starts_with('.')) {
            if (it->is_directory(error))
                it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(error))
            continue;
        const std::size_t dot = name.find('.');
        if (dot == std::string::npos)
            continue;

        BIDSPath p;
        p.m_sRoot = m_sRoot;
        p.m_sExtension = name.substr(dot);
        const std::string dirName = BASELIB::utf8FromPath(it->path().parent_path().filename());
        if (!dirName.starts_with("sub-") && !dirName.starts_with("ses-"))
            p.m_sDatatype = dirName;

        bool valid = true;
        const BASELIB::StringList parts = BASELIB::split(std::string_view(name).substr(0, dot), '_');
        for (std::size_t i = 0; i < parts.size() && valid; ++i) {
            const std::size_t dash = parts[i].find('-');
            if (dash == std::string::npos) {
                // Only the last part may be the suffix
                valid = (i == parts.size() - 1);
                p.m_sSuffix = parts[i];
                continue;
            }
            const std::string_view key = std::string_view(parts[i]).substr(0, dash);
            std::string val = parts[i].substr(dash + 1);
            // Labels are kept as on disk (the run and split setters would pad), so the path names the file found
            if (key == "sub")
                p.m_sSubject = std::move(val);
            else if (key == "ses")
                p.m_sSession = std::move(val);
            else if (key == "task")
                p.m_sTask = std::move(val);
            else if (key == "acq")
                p.m_sAcquisition = std::move(val);
            else if (key == "run")
                p.m_sRun = std::move(val);
            else if (key == "proc")
                p.m_sProcessing = std::move(val);
            else if (key == "space")
                p.m_sSpace = std::move(val);
            else if (key == "recording")
                p.m_sRecording = std::move(val);
            else if (key == "split")
                p.m_sSplit = std::move(val);
            else if (key == "desc")
                p.m_sDescription = std::move(val);
        }
        if (!valid || p.m_sSubject.empty())
            continue;

        const std::array<std::pair<const std::string*, const std::string*>, 13> wanted{{
            {&m_sSubject, &p.m_sSubject},
            {&m_sSession, &p.m_sSession},
            {&m_sTask, &p.m_sTask},
            {&m_sAcquisition, &p.m_sAcquisition},
            {&m_sRun, &p.m_sRun},
            {&m_sProcessing, &p.m_sProcessing},
            {&m_sSpace, &p.m_sSpace},
            {&m_sRecording, &p.m_sRecording},
            {&m_sSplit, &p.m_sSplit},
            {&m_sDescription, &p.m_sDescription},
            {&m_sDatatype, &p.m_sDatatype},
            {&m_sSuffix, &p.m_sSuffix},
            {&m_sExtension, &p.m_sExtension},
        }};
        if (std::ranges::all_of(wanted, [](const auto& w) { return w.first->empty() || *w.first == *w.second; }))
            results.push_back(std::move(p));
    }

    std::ranges::sort(results, {}, &BIDSPath::file_path);
    return results;
}

//=============================================================================================================
// Validation
//=============================================================================================================

bool BIDSPath::is_valid_entity_value(std::string_view sValue)
{
    // BIDS labels are alphanumeric ASCII
    return std::ranges::all_of(sValue, [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    });
}

//=============================================================================================================
// Static helpers
//=============================================================================================================

std::string BIDSPath::zero_pad(std::string sValue)
{
    if (const std::optional<int> num = BASELIB::toInt(sValue))
        return std::format("{:02}", *num);
    return sValue;
}
