//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     project_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ProjectPanel chooses the data directory, project, subject and paradigm of a recording and its timer.
 */

#ifndef VIZ_PROJECT_PANEL_H
#define VIZ_PROJECT_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of a recording: the data directory with its projects and subjects as directories, the
 * paradigm, the resulting file name "<directory>/<project>/<subject>/<YYMMDD_HHMMSS>_<subject>[_<paradigm>]_raw.fif"
 * and the recording timer.
 *
 * New projects and subjects are created as directories. Programmatic changes are not reported; the user's are.
 * Nothing is stored.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ProjectPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(int milliseconds)> recording_time_changed;       /**< The recording time changed. */
        std::function<void(bool on)> recording_timer_changed;               /**< The recording timer was switched. */
        std::function<void(const std::string& project)> project_changed;    /**< Another project was chosen. */
        std::function<void(const std::string& subject)> subject_changed;    /**< Another subject was chosen. */
        std::function<void(const std::string& paradigm)> paradigm_changed;  /**< The paradigm changed. */
        std::function<void(const std::string& fileName)> file_name_changed; /**< The file name changed. */
    };

    //=========================================================================================================
    /**
     * @param[in] dataDirectory   Directory holding the projects.
     * @param[in] project         Project chosen at start.
     * @param[in] subject         Subject chosen at start.
     * @param[in] paradigm        Paradigm at start; empty for none.
     */
    explicit ProjectPanel(const std::filesystem::path& dataDirectory = "/TestData", const std::string& project = "TestProject",
                          const std::string& subject = "TestSubject", const std::string& paradigm = "UnknownParadigm");

    ~ProjectPanel() override;

    /** @param[in] callbacks   What the panel reports. */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Shows the time passed since the recording started.
     *
     * @param[in] milliseconds   Elapsed time; within half a second of the recording time it shows the recording time.
     */
    void set_recording_elapsed_time(int milliseconds);

    //=========================================================================================================
    /**
     * Updates the file name with the current time.
     *
     * @return The file name.
     */
    std::string current_file_name();

    //=========================================================================================================
    /**
     * Shows or hides the file name.
     *
     * @param[in] visible   Whether the file name is shown.
     */
    void set_file_name_visible(bool visible);

    //=========================================================================================================
    /**
     * Shows or hides the paradigm.
     *
     * @param[in] visible   Whether the paradigm is shown.
     */
    void set_paradigm_visible(bool visible);

    //=========================================================================================================
    /**
     * Creates a project directory and chooses it, as the user does with "Add project".
     *
     * @param[in] name   Name of the project.
     *
     * @return Whether the project exists afterwards.
     */
    bool add_project(const std::string& name);

    //=========================================================================================================
    /**
     * Creates a subject directory in the current project and chooses it, as the user does with "Add subject".
     *
     * @param[in] name   Name of the subject.
     *
     * @return Whether the subject exists afterwards.
     */
    bool add_subject(const std::string& name);

    /** @return The directory holding the projects. */
    std::filesystem::path data_directory() const;

    /** @return The projects found in the data directory, sorted by name ignoring case. */
    std::vector<std::string> projects() const;

    /** @return The subjects found in the current project, sorted by name ignoring case. */
    std::vector<std::string> subjects() const;

    /** @return The current project. */
    const std::string& project() const
    {
        return m_project;
    }

    /** @return The current subject. */
    const std::string& subject() const
    {
        return m_subject;
    }

    /** @return The paradigm; empty for none. */
    std::string paradigm() const;

    /** @return The file name as last updated. */
    const std::string& file_name() const
    {
        return m_fileName;
    }

    /** @return The recording time in milliseconds. */
    int recording_time() const;

    /** @return Whether the recording timer is on. */
    bool recording_timer() const;

private:
    void changed(const std::string& key);
    void set_data_directory(const std::filesystem::path& directory);
    void scan_projects();
    bool scan_subjects();
    void update_file_name(bool notify);
    void report_project();
    void report_subject();

    Callbacks m_callbacks;   /**< What to call on changes. */
    std::string m_project;   /**< Current project. */
    std::string m_subject;   /**< Current subject. */
    std::string m_fileName;  /**< Current file name. */
    int m_listener = 0;      /**< Listener on the form. */
    bool m_updating = false; /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_PROJECT_PANEL_H
