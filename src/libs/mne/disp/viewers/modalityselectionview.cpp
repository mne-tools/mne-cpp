//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     modalityselectionview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Implementation of the ModalitySelectionView modality checkbox panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "modalityselectionview.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ModalitySelectionView::ModalitySelectionView(const std::vector<mne::FiffChInfo>& lChannelList,
                                             const QString& sSettingsPath,
                                             QWidget* parent,
                                             Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Modality Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    // Specify which channel types are needed
    for (int i = 0; i < std::ssize(lChannelList); ++i) {
        if (lChannelList.at(i).unit == FIFF_UNIT_T && !std::ranges::contains(m_lChannelTypeList, "MAG")) {
            m_lChannelTypeList.push_back("MAG");
        }
        if (lChannelList.at(i).unit == FIFF_UNIT_T_M && !std::ranges::contains(m_lChannelTypeList, "GRAD")) {
            m_lChannelTypeList.push_back("GRAD");
        }
        if (lChannelList.at(i).kind == FIFFV_EEG_CH && !std::ranges::contains(m_lChannelTypeList, "EEG")) {
            m_lChannelTypeList.push_back("EEG");
        }
        if (lChannelList.at(i).kind == FIFFV_EOG_CH && !std::ranges::contains(m_lChannelTypeList, "EOG")) {
            m_lChannelTypeList.push_back("EOG");
        }
        if (lChannelList.at(i).kind == FIFFV_STIM_CH && !std::ranges::contains(m_lChannelTypeList, "STIM")) {
            m_lChannelTypeList.push_back("STIM");
        }
        if (lChannelList.at(i).kind == FIFFV_MISC_CH && !std::ranges::contains(m_lChannelTypeList, "MISC")) {
            m_lChannelTypeList.push_back("MISC");
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

std::map<std::string, bool> ModalitySelectionView::getModalityMap()
{
    return m_modalityMap;
}

//=============================================================================================================

void ModalitySelectionView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    if (m_modalityMap.contains("MAG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityMAG"), m_modalityMap["MAG"]);
    }
    if (m_modalityMap.contains("GRAD")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityGRAD"), m_modalityMap["GRAD"]);
    }
    if (m_modalityMap.contains("EEG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityEEG"), m_modalityMap["EEG"]);
    }
    if (m_modalityMap.contains("EOG")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityEOG"), m_modalityMap["EOG"]);
    }
    if (m_modalityMap.contains("STIM")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalitySTIM"), m_modalityMap["STIM"]);
    }
    if (m_modalityMap.contains("MISC")) {
        settings.setValue(m_sSettingsPath + QString("/ModalitySelectionView/modalityMISC"), m_modalityMap["MISC"]);
    }
}

//=============================================================================================================

void ModalitySelectionView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    bool flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityMAG"), true).toBool();
    m_modalityMap.insert_or_assign("MAG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityGRAD"), true).toBool();
    m_modalityMap.insert_or_assign("GRAD", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityEEG"), true).toBool();
    m_modalityMap.insert_or_assign("EEG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityEOG"), true).toBool();
    m_modalityMap.insert_or_assign("EOG", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalitySTIM"), true).toBool();
    m_modalityMap.insert_or_assign("STIM", flag);

    flag = settings.value(m_sSettingsPath + QString("/ModalitySelectionView/modalityMISC"), true).toBool();
    m_modalityMap.insert_or_assign("MISC", flag);
}

//=============================================================================================================

void ModalitySelectionView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ModalitySelectionView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ModalitySelectionView::setModalityMap(const std::map<std::string, bool>& modalityMap)
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
    if (!topLayout) {
        topLayout = new QGridLayout();
    }

    // The widgets go right away: the view is redrawn synchronously and looked up by name
    while (std::unique_ptr<QLayoutItem> pItem{topLayout->takeAt(0)}) {
        std::unique_ptr<QWidget> pWidget{pItem->widget()};
    }

    int count = 0;
    for (const auto& [modality, checked] : m_modalityMap) {
        const bool listed = std::ranges::any_of(m_lChannelTypeList, [&modality](const std::string& type) { return mne::iequals(type, modality); });
        if (listed) {
            QCheckBox* t_pCheckBoxModality = new QCheckBox(QString::fromStdString(modality));
            t_pCheckBoxModality->setChecked(checked);
            m_qListModalityCheckBox.push_back(t_pCheckBoxModality);
            connect(t_pCheckBoxModality, &QCheckBox::checkStateChanged,
                    this, &ModalitySelectionView::onUpdateModalityCheckbox);
            topLayout->addWidget(t_pCheckBoxModality, count, 0);
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

    for (QCheckBox* pCheckBox : m_qListModalityCheckBox) {
        m_modalityMap[pCheckBox->text().toStdString()] = pCheckBox->isChecked();
    }

    emit modalitiesChanged(m_modalityMap);

    saveSettings();
}

//=============================================================================================================

void ModalitySelectionView::clearView()
{
}
