//=============================================================================================================
/**
 * @file     compensatorview.cpp
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
 * @brief    Definition of the CompensatorView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "compensatorview.h"

#include <fiff/fiff_ctf_comp.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QGridLayout>
#include <QSignalMapper>
#include <QSettings>
#include <QDebug>

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

CompensatorView::CompensatorView(const QString& sSettingsPath,
                                 QWidget *parent,
                                 Qt::WindowFlags f)
: AbstractView(parent, f)
, m_iLastTo(0)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Compensators");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

CompensatorView::~CompensatorView()
{
    saveSettings();
}

//=============================================================================================================

QList<FIFFLIB::FiffCtfComp> CompensatorView::getCompensators() const
{
    return m_pComps;
}

//=============================================================================================================

void CompensatorView::setCompensators(const QList<FIFFLIB::FiffCtfComp>& comps)
{
    m_pComps = comps;

    for(int i = 0; i < m_pComps.size(); ++i) {
        if(!m_mapCompActive.contains(m_pComps.at(i).kind)) {
            m_mapCompActive.insert(m_pComps.at(i).kind, false);
        }
    }

    redrawGUI();
}

//=============================================================================================================

void CompensatorView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/CompensatorView/compensatorActive"));

    QMap<int,bool>::const_iterator iComp = m_mapCompActive.constBegin();
    while (iComp != m_mapCompActive.constEnd()) {
         settings.setValue(QString::number(iComp.key()), iComp.value());
         ++iComp;
    }

    settings.endGroup();
}

//=============================================================================================================

void CompensatorView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/CompensatorView/compensatorActive"));

    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
        m_mapCompActive[key.toInt()] = settings.value(key).toBool();
    }

    settings.endGroup();
}

//=============================================================================================================

void CompensatorView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void CompensatorView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

int CompensatorView::getLastTo() const
{
    return m_iLastTo;
}

//=============================================================================================================

void CompensatorView::redrawGUI()
{
    if(m_pComps.isEmpty()) {
        return;
    }

    m_qListCompCheckBox.clear();

    // Compensation Selection
    QGridLayout *topLayout = new QGridLayout;

    for(int i = 0; i < m_pComps.size(); ++i) {
        QString numStr;
        QCheckBox* checkBox = new QCheckBox(numStr.setNum(m_pComps[i].kind));

        m_qListCompCheckBox.append(checkBox);

        connect(checkBox, &QCheckBox::toggled,
                this, &CompensatorView::onCheckCompStatusChanged);

        checkBox->setChecked(m_mapCompActive[m_pComps.at(i).kind]);

        topLayout->addWidget(checkBox, i, 0);
    }

    //Find Comp tab and add current layout
    this->setLayout(topLayout);
}

//=============================================================================================================

void CompensatorView::onCheckCompStatusChanged()
{
   if(QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender())) {
        bool currentState = false;
        QString compName = pCheckBox->text();

        for(int i = 0; i < m_qListCompCheckBox.size(); ++i) {
            if(m_qListCompCheckBox[i]->text() != compName) {
                m_qListCompCheckBox[i]->setChecked(false);
                m_mapCompActive[compName.toInt()] = false;
            } else {
                currentState = m_qListCompCheckBox[i]->isChecked();
                m_mapCompActive[compName.toInt()] = currentState;
            }
        }

        if(currentState) {
            emit compSelectionChanged(compName.toInt());
            m_iLastTo = compName.toInt();
        } else { //If none selected
            emit compSelectionChanged(0);
            m_iLastTo = 0;
        }
   }

   saveSettings();
}

//=============================================================================================================

void CompensatorView::clearView()
{

}
