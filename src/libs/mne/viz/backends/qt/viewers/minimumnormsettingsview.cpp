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
#include "settingspanel.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QSettings>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimumNormSettingsView::MinimumNormSettingsView(const QString& sSettingsPath,
                                                 const QString& sMethod,
                                                 QWidget* parent,
                                                 Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<viz::SettingsForm>("InvMinimumNorm Settings"))
{
    m_sSettingsPath = sSettingsPath;
    m_sMethod = sMethod;

    m_pForm->add_flag("methodEditable", true);
    m_pForm->add_flag("isCmne");

    viz::SettingField method;
    method.key = "method";
    method.kind = viz::SettingField::Kind::Choice;
    method.label = "Method:";
    method.choices = {{"MNE", "MNE"}, {"dSPM", "dSPM"}, {"sLORETA", "sLORETA"}, {"CMNE", "CMNE"}};
    method.default_value = std::string("MNE");
    method.enabled_if = "methodEditable";
    m_pForm->add(method);

    m_pForm->add_choice("triggerType", "Trigger type:", {}, {});

    viz::SettingField timePoint;
    timePoint.key = "timePoint";
    timePoint.kind = viz::SettingField::Kind::Int;
    timePoint.label = "Time point (incl. pre stim):";
    timePoint.default_value = 1;
    timePoint.minimum = 1;
    timePoint.maximum = 10000;
    timePoint.decimals = 0;
    timePoint.unit = "ms";
    timePoint.commit = viz::SettingField::Commit::WhenFinished;
    m_pForm->add(timePoint);

    viz::SettingField checkpoint;
    checkpoint.key = "modelCheckpoint";
    checkpoint.kind = viz::SettingField::Kind::Path;
    checkpoint.label = "Model checkpoint:";
    checkpoint.tooltip = "Path to .onnx LSTM checkpoint";
    checkpoint.default_value = std::string();
    checkpoint.file_filter = "ONNX model (*.onnx);;All files (*)";
    checkpoint.commit = viz::SettingField::Commit::WhenFinished;
    checkpoint.visible_if = "isCmne";
    checkpoint.persistent = false;
    m_pForm->add(checkpoint);

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (key == "method") {
            onMethodChanged(QString::fromStdString(m_pForm->get<std::string>("method")));
        } else if (key == "triggerType") {
            onTriggerTypeChanged(QString::fromStdString(m_pForm->get<std::string>("triggerType")));
        } else if (key == "timePoint") {
            onTimePointValueChanged();
        } else if (key == "modelCheckpoint") {
            setModelCheckpoint(mne::path_from_utf8(m_pForm->get<std::string>("modelCheckpoint")));
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new SettingsPanel(m_pForm, this));

    if (!m_sMethod.isEmpty()) {
        m_pForm->set_value("method", m_sMethod.toStdString());
        m_pForm->set_value("methodEditable", false);
    }

    updateCmneWidgetVisibility(QString::fromStdString(m_pForm->get<std::string>("method")));

    this->setWindowTitle("InvMinimumNorm Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
}

//=============================================================================================================

MinimumNormSettingsView::~MinimumNormSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void MinimumNormSettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    std::vector<viz::SettingChoice> choices = m_pForm->field("triggerType")->choices;
    const std::size_t count = choices.size();
    for (const QString& sTriggerType : lTriggerTypes) {
        const std::string type = sTriggerType.toStdString();
        if (std::ranges::none_of(choices, [&type](const viz::SettingChoice& choice) { return choice.value == type; })) {
            choices.push_back({type, type});
        }
    }
    if (choices.size() != count) {
        m_pForm->set_choices("triggerType", std::move(choices));
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
    m_pForm->set_value("modelCheckpoint", mne::utf8_from_path(sPath));
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
    m_pForm->set_value("isCmne", method == QLatin1String("CMNE"));
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
    emit timePointChanged(m_pForm->get<int>("timePoint"));
}

//=============================================================================================================

void MinimumNormSettingsView::clearView()
{
}
