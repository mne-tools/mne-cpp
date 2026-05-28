//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     projectsettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     December 2018
 * @brief    Output-path and filename-template panel for recording / export sessions.
 *
 * ProjectSettingsView lets the user pick a base directory, a project
 * name and a recording-filename template, then turns the latter into
 * the final, timestamp-resolved file path that the recording plugin
 * will write. Templates accept the usual @c %S / @c %T / @c %P
 * placeholders for subject, time and project.
 */

#ifndef PROJECTSETTINGSVIEW_H
#define PROJECTSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class ProjectSettingsViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Output-path / filename-template panel for recording and export sessions.
 *
 * Resolves user-edited templates with @c %S / @c %T / @c %P
 * placeholders into the absolute file path used by the recording
 * plugin.
 */
class DISPSHARED_EXPORT ProjectSettingsView : public AbstractView
{
    Q_OBJECT

public:
    explicit ProjectSettingsView(const QString& sSettingsPath = "",
                                 const QString& sDataPath = "/TestData",
                                 const QString& sCurrentProject = "TestProject",
                                 const QString& sCurrentSubject = "TestSubject",
                                 const QString& sCurrentParadigm = "UnknownParadigm",
                                 QWidget *parent = 0);
    ~ProjectSettingsView();

    //=========================================================================================================
    /**
     * Sets elapsed time to input parameter
     *
     * @param[in] mSecsElapsed      elapsed tim in milliseconds
     */
    void setRecordingElapsedTime(int mSecsElapsed);

    //=========================================================================================================
    /**
     * Updates and gets current file name based on project and subject.
     *
     * @return file name
     */
    QString getCurrentFileName();

    //=========================================================================================================
    /**
     * Updates file name based on current project and subject.
     */
    void triggerFileNameUpdate();

    //=========================================================================================================
    /**
     * Hides file name ui elements.
     */
    void hideFileNameUi();

    //=========================================================================================================
    /**
     * Shows file name ui elements.
     */
    void showFileNameUi();

    //=========================================================================================================
    /**
     * Hides paradigm ui elements.
     */
    void hideParadigmUi();

    //=========================================================================================================
    /**
     * Shows paradigm ui elements.
     */
    void showParadigmUi();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

private:
    void connectGui();

    void addProject();
    void addSubject();

    void deleteProject();
    void deleteSubject();

    void paradigmChanged(const QString &sNewParadigm);

    void scanForProjects();
    void scanForSubjects();

    void selectNewProject(const QString &sNewProject);
    void selectNewSubject(const QString &sNewSubject);

    void updateFileName(bool currentTime = true);

    void onTimeChanged();
    void onRecordingTimerStateChanged(bool state);

    void browseDirectories();

    Ui::ProjectSettingsViewWidget*   m_pUi;

    QStringList         m_sListProjects;
    QStringList         m_sListSubjects;

    QString             m_sDataPath;
    QString             m_sCurrentProject;
    QString             m_sCurrentSubject;
    QString             m_sCurrentParadigm;
    QString             m_sFileName;

    int                 m_iRecordingTime;       /**< recording time in ms.*/

signals:
    void timerChanged(int secs);
    void recordingTimerStateChanged(bool state);
    void newProject(const QString& sCurrentProject);
    void newSubject(const QString& sCurrentSubject);
    void newParadigm(const QString& sCurrentParadigm);
    void fileNameChanged(const QString& sCurrentFileName);
};
} // NAMESPACE

#endif // PROJECTSETTINGSVIEW_H
