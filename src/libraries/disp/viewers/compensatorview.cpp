//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     compensatorview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the CompensatorView gradient-compensation selector.
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
