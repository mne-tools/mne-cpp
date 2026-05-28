//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     modalityselectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ModalitySelectionView modality checkbox panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "modalityselectionview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QMapIterator>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ModalitySelectionView::ModalitySelectionView(const QList<FIFFLIB::FiffChInfo>& lChannelList,
                                             const QString &sSettingsPath,
                                             QWidget *parent,
                                             Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Modality Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    // Specify which channel types are needed
    for(int i = 0; i < lChannelList.size(); ++i) {
        if(lChannelList.at(i).unit == FIFF_UNIT_T && !m_lChannelTypeList.contains("MAG")) {
            m_lChannelTypeList << "MAG";
        }
        if(lChannelList.at(i).unit == FIFF_UNIT_T_M && !m_lChannelTypeList.contains("GRAD")) {
            m_lChannelTypeList << "GRAD";
        }
        if(lChannelList.at(i).kind == FIFFV_EEG_CH && !m_lChannelTypeList.contains("EEG")) {
            m_lChannelTypeList << "EEG";
        }
        if(lChannelList.at(i).kind == FIFFV_EOG_CH && !m_lChannelTypeList.contains("EOG")) {
            m_lChannelTypeList << "EOG";
        }
        if(lChannelList.at(i).kind == FIFFV_STIM_CH && !m_lChannelTypeList.contains("STIM")) {
            m_lChannelTypeList << "STIM";
        }
        if(lChannelList.at(i).kind == FIFFV_MISC_CH && !m_lChannelTypeList.contains("MISC")) {
            m_lChannelTypeList << "MISC";
        }
    }

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

ModalitySelectionView::~ModalitySelectionView()
{
    saveSettings();
}

//=============================================================================================================

QMap<QString, bool> ModalitySelectionView::getModalityMap()
{
    return m_modalityMap;
}

//=============================================================================================================

void ModalitySelectionView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    if(m_modalityMap.contains("MAG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityMAG"), m_modalityMap["MAG"]);
    }
    if(m_modalityMap.contains("GRAD")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityGRAD"), m_modalityMap["GRAD"]);
    }
    if(m_modalityMap.contains("EEG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityEEG"), m_modalityMap["EEG"]);
    }
    if(m_modalityMap.contains("EOG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityEOG"), m_modalityMap["EOG"]);
    }
    if(m_modalityMap.contains("STIM")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalitySTIM"), m_modalityMap["STIM"]);
    }
    if(m_modalityMap.contains("MISC")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityMISC"), m_modalityMap["MISC"]);
    }
}

//=============================================================================================================

void ModalitySelectionView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    bool flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityMAG"), true).toBool();
    m_modalityMap.insert("MAG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityGRAD"), true).toBool();
    m_modalityMap.insert("GRAD", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityEEG"), true).toBool();
    m_modalityMap.insert("EEG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityEOG"), true).toBool();
    m_modalityMap.insert("EOG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalitySTIM"), true).toBool();
    m_modalityMap.insert("STIM", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityMISC"), true).toBool();
    m_modalityMap.insert("MISC", flag);
}

//=============================================================================================================

void ModalitySelectionView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ModalitySelectionView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ModalitySelectionView::setModalityMap(const QMap<QString, bool> &modalityMap)
{
    m_modalityMap = modalityMap;

    redrawGUI();
}

//=============================================================================================================

void ModalitySelectionView::redrawGUI()
{
    m_qListModalityCheckBox.clear();

    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    QMapIterator<QString, bool> i(m_modalityMap);

    int count = 0;
    while (i.hasNext()) {
        i.next();

        if(m_lChannelTypeList.contains(i.key(), Qt::CaseInsensitive)) {
            QCheckBox* t_pCheckBoxModality = new QCheckBox(i.key());
            t_pCheckBoxModality->setChecked(i.value());
            m_qListModalityCheckBox << t_pCheckBoxModality;
            connect(t_pCheckBoxModality,&QCheckBox::checkStateChanged,
                    this, &ModalitySelectionView::onUpdateModalityCheckbox);
            topLayout->addWidget(t_pCheckBoxModality,count,0);
            count++;
        }
    }

    //Find Modalities tab and add current layout
    this->setLayout(topLayout);
}

//=============================================================================================================

void ModalitySelectionView::onUpdateModalityCheckbox(Qt::CheckState state)
{
    Q_UNUSED(state)

    for(qint32 i = 0; i < m_qListModalityCheckBox.size(); ++i) {
        m_modalityMap[m_qListModalityCheckBox.at(i)->text()] = m_qListModalityCheckBox.at(i)->isChecked();
    }

    emit modalitiesChanged(m_modalityMap);

    saveSettings();
}

//=============================================================================================================

void ModalitySelectionView::clearView()
{

}
