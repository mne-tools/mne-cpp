//=============================================================================================================
/**
 * @file     projectorsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ProjectorsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectorsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QFrame>
#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectorsView::ProjectorsView(const QString& sSettingsPath,
                               QWidget *parent,
                               Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pEnableDisableProjectors(Q_NULLPTR)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Projectors");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

ProjectorsView::~ProjectorsView()
{
    saveSettings();
}

//=============================================================================================================

QList<FIFFLIB::FiffProj> ProjectorsView::getProjectors() const
{
    return m_pProjs;
}

//=============================================================================================================

void ProjectorsView::setProjectors(const QList<FIFFLIB::FiffProj>& projs)
{
    m_pProjs = projs;

    for(int i = 0; i < m_pProjs.size(); ++i) {
        if(!m_mapProjActive.contains(m_pProjs.at(i).desc)) {
            m_mapProjActive.insert(m_pProjs.at(i).desc, m_pProjs.at(i).active);
        } else {
            m_pProjs[i].active = m_mapProjActive[m_pProjs.at(i).desc];
        }
    }

    redrawGUI();
}

//=============================================================================================================

void ProjectorsView::redrawGUI()
{
    if(m_pProjs.isEmpty()) {
        return;
    }

    m_qListProjCheckBox.clear();

    // Projection Selection
    QGridLayout *topLayout = new QGridLayout;

    bool bAllActivated = true;

    qint32 i = 0;

    for(i; i < m_pProjs.size(); ++i) {
        QCheckBox* checkBox = new QCheckBox(m_pProjs.at(i).desc);

        if(m_pProjs.at(i).active == false) {
            bAllActivated = false;
        }

        m_qListProjCheckBox.append(checkBox);

        connect(checkBox, &QCheckBox::toggled,
                this, &ProjectorsView::onCheckProjStatusChanged);

        checkBox->setChecked(m_pProjs.at(i).active);

        topLayout->addWidget(checkBox, i, 0);
    }

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    topLayout->addWidget(line, i+1, 0);

    m_pEnableDisableProjectors = new QCheckBox("Enable all");
    m_pEnableDisableProjectors->setChecked(bAllActivated);
    topLayout->addWidget(m_pEnableDisableProjectors, i+2, 0);
    connect(m_pEnableDisableProjectors, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
        this, &ProjectorsView::onEnableDisableAllProj);

    this->setLayout(topLayout);

    onCheckProjStatusChanged();
}

//=============================================================================================================

void ProjectorsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/ProjectorsView/projectorsActive"));

    QMap<QString,bool>::const_iterator iProj = m_mapProjActive.constBegin();
    while (iProj != m_mapProjActive.constEnd()) {
         settings.setValue(iProj.key(), iProj.value());
         ++iProj;
    }

    settings.endGroup();
}

//=============================================================================================================

void ProjectorsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/ProjectorsView/projectorsActive"));

    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
        m_mapProjActive[key] = settings.value(key).toBool();
    }
    settings.endGroup();
}

//=============================================================================================================

void ProjectorsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ProjectorsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ProjectorsView::onEnableDisableAllProj(bool status)
{
    //Set all checkboxes to status
    for(int i = 0; i<m_qListProjCheckBox.size(); i++) {
        m_qListProjCheckBox.at(i)->setChecked(status);
    }

    //Set all projection activation states to status
    for(int i = 0; i < m_pProjs.size(); ++i) {
        m_pProjs[i].active = status;
        m_mapProjActive[m_pProjs.at(i).desc] = status;

    }

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(status);
    }

    emit projSelectionChanged(m_pProjs);

    saveSettings();
}

//=============================================================================================================

void ProjectorsView::onCheckProjStatusChanged()
{
    bool bAllActivated = true;

    for(qint32 i = 0; i < m_qListProjCheckBox.size(); ++i) {
        if(m_qListProjCheckBox.at(i)->isChecked() == false) {
            bAllActivated = false;
        }

        m_pProjs[i].active = m_qListProjCheckBox.at(i)->isChecked();
        m_mapProjActive[m_pProjs.at(i).desc] = m_qListProjCheckBox.at(i)->isChecked();
    }

    if(m_pEnableDisableProjectors) {
        m_pEnableDisableProjectors->setChecked(bAllActivated);
    }

    emit projSelectionChanged(m_pProjs);

    saveSettings();
}

//=============================================================================================================

void ProjectorsView::clearView()
{

}
