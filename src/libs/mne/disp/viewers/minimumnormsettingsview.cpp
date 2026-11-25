//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     minimumnormsettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           johaenns <j.vorw01@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September 2018
 * @brief    Implementation of the MinimumNormSettingsView inverse-method panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "minimumnormsettingsview.h"

#include "ui_minimumnormsettingsview.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QPushButton>
#include <QSettings>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimumNormSettingsView::MinimumNormSettingsView(const QString& sSettingsPath,
                                                 const QString& sMethod,
                                                 QWidget* parent,
                                                 Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(std::make_unique<Ui::MinimumNormSettingsViewWidget>())
{
    m_sSettingsPath = sSettingsPath;
    m_sMethod = sMethod;
    m_pUi->setupUi(this);

    connect(m_pUi->m_comboBox_method, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &MinimumNormSettingsView::onMethodChanged);

    connect(m_pUi->m_comboBox_triggerType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &MinimumNormSettingsView::onTriggerTypeChanged);

    connect(m_pUi->m_spinBox_timepoint, &QSpinBox::editingFinished,
            this, &MinimumNormSettingsView::onTimePointValueChanged);

    connect(m_pUi->m_pushButton_browseModelCheckpoint, &QPushButton::clicked,
            this, &MinimumNormSettingsView::onBrowseModelCheckpointClicked);

    if (!m_sMethod.isEmpty()) {
        m_pUi->m_comboBox_method->setCurrentIndex(m_pUi->m_comboBox_method->findText(m_sMethod));
        m_pUi->m_comboBox_method->setEnabled(false);
    }

    updateCmneWidgetVisibility(m_pUi->m_comboBox_method->currentText());

    this->setWindowTitle("InvMinimumNorm Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
}

//=============================================================================================================

MinimumNormSettingsView::~MinimumNormSettingsView()
{
    saveSettings();
}

//=============================================================================================================

void MinimumNormSettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    for (const QString& sTriggerType : lTriggerTypes) {
        if (m_pUi->m_comboBox_triggerType->findText(sTriggerType) == -1) {
            m_pUi->m_comboBox_triggerType->addItem(sTriggerType);
        }
    }
}

//=============================================================================================================

void MinimumNormSettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    settings.setValue(m_sSettingsPath + QString("/MinimumNormSettingsView/modelCheckpoint"),
                      QString::fromStdString(mne::utf8_from_path(m_sModelCheckpoint)));
}

//=============================================================================================================

void MinimumNormSettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    const QString sStored = settings.value(m_sSettingsPath + QString("/MinimumNormSettingsView/modelCheckpoint"),
                                           QString())
                                .toString();
    if (!sStored.isEmpty()) {
        setModelCheckpoint(mne::path_from_utf8(sStored.toStdString()));
    }
}

//=============================================================================================================

void MinimumNormSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void MinimumNormSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void MinimumNormSettingsView::onMethodChanged(const QString& method)
{
    updateCmneWidgetVisibility(method);
    emit methodChanged(method);
}

//=============================================================================================================

void MinimumNormSettingsView::setModelCheckpoint(const std::filesystem::path& sPath)
{
    if (m_sModelCheckpoint == sPath) {
        return;
    }
    m_sModelCheckpoint = sPath;
    m_pUi->m_lineEdit_modelCheckpoint->setText(QString::fromStdString(mne::utf8_from_path(sPath)));
    saveSettings();
    emit modelCheckpointChanged(sPath);
}

//=============================================================================================================

std::filesystem::path MinimumNormSettingsView::getModelCheckpoint() const
{
    return m_sModelCheckpoint;
}

//=============================================================================================================

void MinimumNormSettingsView::updateCmneWidgetVisibility(const QString& method)
{
    const bool bIsCmne = (method == QLatin1String("CMNE"));
    m_pUi->m_label_modelCheckpoint->setVisible(bIsCmne);
    m_pUi->m_lineEdit_modelCheckpoint->setVisible(bIsCmne);
    m_pUi->m_pushButton_browseModelCheckpoint->setVisible(bIsCmne);
}

//=============================================================================================================

void MinimumNormSettingsView::onBrowseModelCheckpointClicked()
{
    std::error_code error;
    const QString sStartDir = m_sModelCheckpoint.empty()
        ? QString()
        : QString::fromStdString(mne::utf8_from_path(std::filesystem::absolute(m_sModelCheckpoint, error).parent_path()));
    const QString sFile = QFileDialog::getOpenFileName(this,
                                                       tr("Select CMNE model checkpoint"),
                                                       sStartDir,
                                                       tr("ONNX model (*.onnx);;All files (*)"));
    if (!sFile.isEmpty()) {
        setModelCheckpoint(mne::path_from_utf8(sFile.toStdString()));
    }
}

//=============================================================================================================

void MinimumNormSettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
}

//=============================================================================================================

void MinimumNormSettingsView::onTimePointValueChanged()
{
    emit timePointChanged(m_pUi->m_spinBox_timepoint->value());
}

//=============================================================================================================

void MinimumNormSettingsView::clearView()
{
}
