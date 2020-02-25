//=============================================================================================================
/**
 * @file     projectsettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Declaration of the ProjectSettingsView Class.
 *
 */

#ifndef PROJECTSETTINGSVIEW_H
#define PROJECTSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>


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
 * The ProjectSettingsView class provides a viewer to setup and manage the file name before the acquisition starts.
 *
 * @brief The ProjectSettingsView class provides a viewer to setup and manage the file name before the acquisition starts.
 */
class DISPSHARED_EXPORT ProjectSettingsView : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectSettingsView(const QString& sDataPath = "/TestData",
                                 const QString& sCurrentProject = "TestProject",
                                 const QString& sCurrentSubject = "TestSubject",
                                 const QString& sCurrentParadigm = "UnknownParadigm",
                                 QWidget *parent = 0);
    ~ProjectSettingsView();

    void setRecordingElapsedTime(int mSecsElapsed);
    QString getCurrentFileName();

private:
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

    Ui::ProjectSettingsViewWidget*   ui;

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
};

} // NAMESPACE

#endif // PROJECTSETTINGSVIEW_H
