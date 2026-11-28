//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     project_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ProjectPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "project_panel.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kDataDirectory = "dataDirectory";
constexpr const char* kProject = "project";
constexpr const char* kNewProject = "newProject";
constexpr const char* kAddProject = "addProject";
constexpr const char* kSubject = "subject";
constexpr const char* kNewSubject = "newSubject";
constexpr const char* kAddSubject = "addSubject";
constexpr const char* kParadigm = "paradigm";
constexpr const char* kFileName = "fileName";
constexpr const char* kShowParadigm = "showParadigm";
constexpr const char* kShowFileName = "showFileName";
constexpr const char* kUseTimer = "useRecordingTimer";
constexpr const char* kHours = "hours";
constexpr const char* kMinutes = "minutes";
constexpr const char* kSeconds = "seconds";
constexpr const char* kPassedTime = "passedTime";

constexpr const char* kSetup = "Project File Setup";
constexpr const char* kRecording = "Recording settings";

SettingField unsaved(const char* key, SettingField::Kind kind, const std::string& label, SettingValue value, const char* group)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.default_value = std::move(value);
    field.group = group;
    // The recording plugin decides afresh each session
    field.persistent = false;
    return field;
}

SettingField time_field(const char* key, const std::string& label, int value, const std::string& unit)
{
    SettingField field = unsaved(key, SettingField::Kind::Int, label, value, kRecording);
    field.minimum = 0;
    field.maximum = 1000;
    field.unit = unit;
    return field;
}

/** The visible subdirectories of @p directory, sorted by name ignoring case. */
std::vector<std::string> subdirectories(const std::filesystem::path& directory)
{
    std::vector<std::string> names;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
        std::string name = mne::utf8_from_path(entry.path().filename());
        if (entry.is_directory(error) && !name.starts_with('.')) {
            names.push_back(std::move(name));
        }
    }
    const auto lower = [](unsigned char c) {
        return std::tolower(c);
    };
    std::ranges::sort(names, [&lower](const std::string& a, const std::string& b) {
        return std::ranges::lexicographical_compare(a, b, {}, lower, lower);
    });
    return names;
}

std::vector<SettingChoice> plain_choices(const std::vector<std::string>& names)
{
    std::vector<SettingChoice> choices;
    choices.reserve(names.size());
    for (const std::string& name : names) {
        choices.push_back({name, name});
    }
    return choices;
}

std::string clock_text(int milliseconds)
{
    const int seconds = std::max(milliseconds, 0) / 1000;
    std::array<char, 16> text{};
    std::snprintf(text.data(), text.size(), "%02d:%02d:%02d", (seconds / 3600) % 24, (seconds / 60) % 60, seconds % 60);
    return text.data();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectPanel::ProjectPanel(const std::filesystem::path& dataDirectory, const std::string& project, const std::string& subject,
                           const std::string& paradigm)
: Panel("ProjectSettingsView", "Project Settings")
, m_project(project)
, m_subject(subject)
{
    SettingsForm& f = form();
    f.add_flag(kShowParadigm, true);
    f.add_flag(kShowFileName, true);
    SettingField directory = unsaved(kDataDirectory, SettingField::Kind::Directory, "Directory:", utf8_from_path(dataDirectory), kSetup);
    directory.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(directory));
    f.add(unsaved(kProject, SettingField::Kind::Choice, "Project:", project, kSetup));
    f.add(unsaved(kNewProject, SettingField::Kind::Text, "New project:", std::string(), kSetup));
    f.add(unsaved(kAddProject, SettingField::Kind::Action, "Add project", false, kSetup));
    f.add(unsaved(kSubject, SettingField::Kind::Choice, "Subject:", subject, kSetup));
    f.add(unsaved(kNewSubject, SettingField::Kind::Text, "New subject:", std::string(), kSetup));
    f.add(unsaved(kAddSubject, SettingField::Kind::Action, "Add subject", false, kSetup));
    SettingField paradigmField = unsaved(kParadigm, SettingField::Kind::Text, "Paradigm:", paradigm, kSetup);
    paradigmField.visible_if = kShowParadigm;
    f.add(std::move(paradigmField));
    SettingField fileField = unsaved(kFileName, SettingField::Kind::Info, "File:", std::string(), kSetup);
    fileField.visible_if = kShowFileName;
    f.add(std::move(fileField));

    f.add(unsaved(kUseTimer, SettingField::Kind::Bool, "Use recording timer", false, kRecording));
    f.add(time_field(kHours, "Recording hours:", 0, " h"));
    f.add(time_field(kMinutes, "Recording minutes:", 5, " min"));
    f.add(time_field(kSeconds, "Recording seconds:", 0, " sec"));
    f.add(unsaved(kPassedTime, SettingField::Kind::Info, "Passed time:", std::string("00:00:00"), kRecording));

    scan_projects();
    scan_subjects();
    update_file_name(false);

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kAddProject || key == kAddSubject)) {
            changed(key);
        }
    });
}

//=============================================================================================================

ProjectPanel::~ProjectPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void ProjectPanel::changed(const std::string& key)
{
    if (key == kDataDirectory) {
        set_data_directory(data_directory());
    } else if (key == kProject) {
        m_project = form().get<std::string>(kProject);
        report_project();
        if (scan_subjects()) {
            report_subject();
        }
        update_file_name(true);
    } else if (key == kSubject) {
        m_subject = form().get<std::string>(kSubject);
        report_subject();
        update_file_name(true);
    } else if (key == kParadigm) {
        if (m_callbacks.paradigm_changed) {
            m_callbacks.paradigm_changed(paradigm());
        }
        update_file_name(true);
    } else if (key == kAddProject || key == kAddSubject) {
        const char* nameKey = key == kAddProject ? kNewProject : kNewSubject;
        const std::string name = form().get<std::string>(nameKey);
        if (!name.empty()) {
            const bool added = key == kAddProject ? add_project(name) : add_subject(name);
            if (added) {
                const bool updating = std::exchange(m_updating, true);
                form().set_value(nameKey, std::string());
                m_updating = updating;
            }
        }
    } else if (key == kHours || key == kMinutes || key == kSeconds) {
        if (m_callbacks.recording_time_changed) {
            m_callbacks.recording_time_changed(recording_time());
        }
    } else if (key == kUseTimer) {
        if (m_callbacks.recording_timer_changed) {
            m_callbacks.recording_timer_changed(recording_timer());
        }
    }
}

//=============================================================================================================

void ProjectPanel::set_data_directory(const std::filesystem::path& directory)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kDataDirectory, utf8_from_path(directory));
    m_updating = updating;
    const std::string project = m_project;
    scan_projects();
    if (m_project != project) {
        report_project();
    }
    if (scan_subjects()) {
        report_subject();
    }
    update_file_name(true);
}

//=============================================================================================================

void ProjectPanel::scan_projects()
{
    const std::vector<std::string> names = projects();
    // An unknown project becomes the first one; without any the name stays for the file name
    if (!names.empty() && std::ranges::find(names, m_project) == names.end()) {
        m_project = names.front();
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(kProject, plain_choices(names));
    form().set_value(kProject, m_project);
    m_updating = updating;
}

//=============================================================================================================

bool ProjectPanel::scan_subjects()
{
    const std::vector<std::string> names = subjects();
    const std::string subject = m_subject;
    if (std::ranges::find(names, m_subject) == names.end()) {
        m_subject = names.empty() ? std::string() : names.front();
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(kSubject, plain_choices(names));
    form().set_value(kSubject, m_subject);
    m_updating = updating;
    return m_subject != subject;
}

//=============================================================================================================

void ProjectPanel::update_file_name(bool notify)
{
    const std::time_t seconds = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    std::array<char, 32> stamp{};
    std::strftime(stamp.data(), stamp.size(), "%y%m%d_%H%M%S", &local);

    const std::string paradigmName = paradigm();
    m_fileName = utf8_from_path(data_directory()) + "/" + m_project + "/" + m_subject + "/" + stamp.data() + "_" + m_subject;
    if (!paradigmName.empty()) {
        m_fileName += "_" + paradigmName;
    }
    m_fileName += "_raw.fif";

    const bool updating = std::exchange(m_updating, true);
    form().set_value(kFileName, m_fileName);
    m_updating = updating;
    if (notify && m_callbacks.file_name_changed) {
        m_callbacks.file_name_changed(m_fileName);
    }
}

//=============================================================================================================

void ProjectPanel::report_project()
{
    if (m_callbacks.project_changed) {
        m_callbacks.project_changed(m_project);
    }
}

//=============================================================================================================

void ProjectPanel::report_subject()
{
    if (m_callbacks.subject_changed) {
        m_callbacks.subject_changed(m_subject);
    }
}

//=============================================================================================================

void ProjectPanel::set_recording_elapsed_time(int milliseconds)
{
    // Timers fire a little early, so the end of the recording shows the full recording time
    const int recordingTime = recording_time();
    const int shown = recordingTime - milliseconds < 500 ? recordingTime : milliseconds;
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kPassedTime, clock_text(shown));
    m_updating = updating;
}

//=============================================================================================================

std::string ProjectPanel::current_file_name()
{
    update_file_name(false);
    return m_fileName;
}

//=============================================================================================================

void ProjectPanel::set_file_name_visible(bool visible)
{
    form().set_value(kShowFileName, visible);
}

//=============================================================================================================

void ProjectPanel::set_paradigm_visible(bool visible)
{
    form().set_value(kShowParadigm, visible);
}

//=============================================================================================================

bool ProjectPanel::add_project(const std::string& name)
{
    const std::filesystem::path directory = data_directory() / path_from_utf8(name);
    std::error_code error;
    std::filesystem::create_directory(directory, error);
    if (!std::filesystem::is_directory(directory, error)) {
        return false;
    }
    m_project = name;
    report_project();
    scan_projects();
    if (scan_subjects()) {
        report_subject();
    }
    update_file_name(true);
    return true;
}

//=============================================================================================================

bool ProjectPanel::add_subject(const std::string& name)
{
    const std::filesystem::path directory = data_directory() / path_from_utf8(m_project) / path_from_utf8(name);
    std::error_code error;
    std::filesystem::create_directory(directory, error);
    if (!std::filesystem::is_directory(directory, error)) {
        return false;
    }
    m_subject = name;
    report_subject();
    scan_subjects();
    update_file_name(true);
    return true;
}

//=============================================================================================================

std::filesystem::path ProjectPanel::data_directory() const
{
    return path_from_utf8(form().get<std::string>(kDataDirectory));
}

//=============================================================================================================

std::vector<std::string> ProjectPanel::projects() const
{
    return subdirectories(data_directory());
}

//=============================================================================================================

std::vector<std::string> ProjectPanel::subjects() const
{
    return subdirectories(data_directory() / path_from_utf8(m_project));
}

//=============================================================================================================

std::string ProjectPanel::paradigm() const
{
    return form().get<std::string>(kParadigm);
}

//=============================================================================================================

int ProjectPanel::recording_time() const
{
    return ((form().get<int>(kHours) * 60 + form().get<int>(kMinutes)) * 60 + form().get<int>(kSeconds)) * 1000;
}

//=============================================================================================================

bool ProjectPanel::recording_timer() const
{
    return form().get<bool>(kUseTimer);
}
