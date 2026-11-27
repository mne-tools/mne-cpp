//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     triggerdetectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the TriggerDetectionView trigger-detection panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggerdetectionview.h"
#include "../detail/scoped_flag.h"
#include "../kit/settingspanel.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColorDialog>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** A field shown in real-time mode only. */
SettingField realTimeField(const std::string& key, SettingField::Kind kind, const std::string& label)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.visible_if = "realTime";
    return field;
}

/** Shows a color on a color button. */
void showColor(QPushButton* button, const QColor& color)
{
    if (!button) {
        return;
    }
    QPalette palette = button->palette();
    palette.setColor(QPalette::Button, color);
    button->setPalette(palette);
    button->update();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerDetectionView::TriggerDetectionView(const QString& sSettingsPath,
                                           QWidget* parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<viz::SettingsForm>("Trigger Detection Settings"))
{
    m_sSettingsPath = sSettingsPath;

    m_pForm->add_flag("realTime", true);

    SettingField activate = realTimeField("activateTriggerDetection", SettingField::Kind::Bool, "Activate trigger detection");
    activate.default_value = false;
    m_pForm->add(activate);

    m_pForm->add_choice("triggerChannel", "Trigger channel:", {}, {});

    m_pForm->add(realTimeField("triggerColorType", SettingField::Kind::Choice, "Type color:"));
    m_pForm->add(realTimeField("triggerColor", SettingField::Kind::Custom, {}));

    m_pForm->add_double("detectionThresholdFirst", "Threshold:", 0.1, -1000.0, 1000.0, 0.1, 2);
    SettingField exponent;
    exponent.key = "detectionThresholdSecond";
    exponent.kind = SettingField::Kind::Int;
    exponent.default_value = -1;
    exponent.minimum = -100;
    exponent.maximum = 100;
    exponent.decimals = 0;
    exponent.prefix = "e^";
    m_pForm->add(exponent);

    m_pForm->add(realTimeField("numberDetectedTriggers", SettingField::Kind::Custom, "Detected trigger:"));
    m_pForm->add(realTimeField("resetNumberTriggers", SettingField::Kind::Action, "Reset"));

    SettingField detect;
    detect.key = "detectTriggers";
    detect.kind = SettingField::Kind::Action;
    detect.label = "Detect Triggers";
    detect.visible_if = "!realTime";
    m_pForm->add(detect);

    // The controls react once init() connected the view to measurement info
    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (!m_bInitialized || m_bSilent) {
            return;
        }
        if (key == "activateTriggerDetection" || key == "triggerChannel" || key.starts_with("detectionThreshold")) {
            onTriggerInfoChanged();
        } else if (key == "triggerColorType") {
            onRealTimeTriggerColorTypeChanged(QString::fromStdString(m_pForm->get<std::string>("triggerColorType")));
        } else if (key == "resetNumberTriggers") {
            onResetTriggerNumbers();
        } else if (key == "detectTriggers") {
            onDetectTriggers();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* pPanel = new SettingsPanel(m_pForm, this);
    layout->addWidget(pPanel);

    if (auto* pColor = pPanel->customWidget("triggerColor")) {
        m_pTriggerColorButton = new QPushButton(QStringLiteral("Click to change"), pColor);
        m_pTriggerColorButton->setObjectName(QStringLiteral("triggerColorButton"));
        pColor->layout()->addWidget(m_pTriggerColorButton);
        connect(m_pTriggerColorButton, &QPushButton::clicked, this, [this](bool state) {
            if (m_bInitialized) {
                onRealTimeTriggerColorChanged(state);
            }
        });
    }
    if (auto* pCount = pPanel->customWidget("numberDetectedTriggers")) {
        m_pNumberDetectedTriggers = new QLabel(QStringLiteral("0"), pCount);
        m_pNumberDetectedTriggers->setObjectName(QStringLiteral("numberDetectedTriggersLabel"));
        pCount->layout()->addWidget(m_pNumberDetectedTriggers);
    }

    this->setWindowTitle("Trigger Detection Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    updateProcessingMode(RealTime);

    loadSettings();
}

//=============================================================================================================

TriggerDetectionView::~TriggerDetectionView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void TriggerDetectionView::init(const std::shared_ptr<FiffInfo>& pFiffInfo)
{
    if (pFiffInfo) {
        m_pFiffInfo = pFiffInfo;

        std::vector<SettingChoice> channels;
        for (int i = 0; i < std::ssize(m_pFiffInfo->chs); i++) {
            if (m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH) {
                channels.push_back({m_pFiffInfo->chs[i].ch_name, m_pFiffInfo->chs[i].ch_name});
            }
        }
        m_pForm->set_choices("triggerChannel", std::move(channels));

        m_bInitialized = true;

        if (m_pTriggerColorButton) {
            m_pTriggerColorButton->setAutoFillBackground(true);
            m_pTriggerColorButton->setFlat(true);
        }
        showColor(m_pTriggerColorButton, QColor(177, 0, 0));

        loadSettings();
    }
}

//=============================================================================================================

void TriggerDetectionView::setNumberDetectedTriggersAndTypes(int numberDetections, const std::map<int, std::vector<std::pair<int, double>>>& mapDetectedTriggers)
{
    if (m_pNumberDetectedTriggers) {
        m_pNumberDetectedTriggers->setText(QString("%1").arg(numberDetections));
    }

    //Set trigger types
    std::vector<SettingChoice> types = m_pForm->field("triggerColorType")->choices;
    const std::size_t count = types.size();
    for (const auto& [channel, triggers] : mapDetectedTriggers) {
        for (const auto& [sample, value] : triggers) {
            const std::string type = QString::number(value).toStdString();
            if (std::ranges::none_of(types, [&type](const SettingChoice& choice) { return choice.value == type; })) {
                types.push_back({type, type});
            }
        }
    }
    if (types.size() != count) {
        m_pForm->set_choices("triggerColorType", std::move(types));
    }
}

//=============================================================================================================

void TriggerDetectionView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    const std::vector<SettingChoice>& channels = m_pForm->field("triggerChannel")->choices;
    const std::string channel = m_pForm->get<std::string>("triggerChannel");
    const auto current = std::ranges::find(channels, channel, &SettingChoice::value);
    const int channelIndex = current == channels.end() ? -1 : static_cast<int>(std::distance(channels.begin(), current));

    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), m_pForm->get<bool>("activateTriggerDetection"));
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), channelIndex);
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), m_pForm->get<double>("detectionThresholdFirst"));
    settings.setValue(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), m_pForm->get<int>("detectionThresholdSecond"));

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    std::map<double, QColor>::const_iterator i = m_qMapTriggerColor.cbegin();
    while (i != m_qMapTriggerColor.cend()) {
        settings.setValue(QString::number(i->first), i->second);
        ++i;
    }
    settings.endGroup();
}

//=============================================================================================================

void TriggerDetectionView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    {
        const detail::ScopedFlag silent(m_bSilent);
        m_pForm->set_value("activateTriggerDetection", settings.value(m_sSettingsPath + QString("/TriggerDetectionView/Activated"), false).toBool());
        const std::vector<SettingChoice>& channels = m_pForm->field("triggerChannel")->choices;
        const int channelIndex = settings.value(m_sSettingsPath + QString("/TriggerDetectionView/ChannelIndex"), 0).toInt();
        if (channelIndex >= 0 && channelIndex < std::ssize(channels)) {
            m_pForm->set_value("triggerChannel", channels[static_cast<std::size_t>(channelIndex)].value);
        }
        m_pForm->set_value("detectionThresholdFirst", settings.value(m_sSettingsPath + QString("/TriggerDetectionView/FirstThresholdValue"), 0.1).toDouble());
        m_pForm->set_value("detectionThresholdSecond", settings.value(m_sSettingsPath + QString("/TriggerDetectionView/SecondThresholdValue"), -1).toInt());
    }

    settings.beginGroup(m_sSettingsPath + QString("/TriggerDetectionView/Colors"));
    for (const QString& key : settings.childKeys()) {
        double dKey = key.toDouble();
        m_qMapTriggerColor[dKey] = settings.value(key).value<QColor>();
    }
    settings.endGroup();

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TriggerDetectionView::updateProcessingMode(ProcessingMode mode)
{
    m_pForm->set_value("realTime", mode != ProcessingMode::Offline);
}

//=============================================================================================================

void TriggerDetectionView::onTriggerInfoChanged()
{
    emit triggerInfoChanged(m_qMapTriggerColor,
                            m_pForm->get<bool>("activateTriggerDetection"),
                            getSelectedStimChannel(),
                            m_pForm->get<double>("detectionThresholdFirst") * std::pow(10, m_pForm->get<int>("detectionThresholdSecond")));

    saveSettings();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorChanged(bool state)
{
    Q_UNUSED(state);

    const double type = QString::fromStdString(m_pForm->get<std::string>("triggerColorType")).toDouble();
    const auto color = QColorDialog::getColor(m_qMapTriggerColor[type], this, "Set trigger color");
    // Cancelled: getColor() returns an invalid colour
    if (!color.isValid()) {
        return;
    }

    showColor(m_pTriggerColorButton, color);

    m_qMapTriggerColor[type] = color;

    onTriggerInfoChanged();
}

//=============================================================================================================

void TriggerDetectionView::onRealTimeTriggerColorTypeChanged(const QString& value)
{
    showColor(m_pTriggerColorButton, m_qMapTriggerColor[value.toDouble()]);
}

//=============================================================================================================

void TriggerDetectionView::onResetTriggerNumbers()
{
    if (m_pNumberDetectedTriggers) {
        m_pNumberDetectedTriggers->setText(QString("0"));
    }
    const bool hadTypes = !m_pForm->field("triggerColorType")->choices.empty();
    m_pForm->set_choices("triggerColorType", {});
    // The emptied type list selects no type, like the cleared combo box did
    if (hadTypes && m_bInitialized) {
        onRealTimeTriggerColorTypeChanged(QString());
    }

    emit resetTriggerCounter();
}

//=============================================================================================================

void TriggerDetectionView::onDetectTriggers()
{
    if (getSelectedStimChannel().empty()) {
        return;
    }
    emit detectTriggers(getSelectedStimChannel(),
                        m_pForm->get<double>("detectionThresholdFirst") * std::pow(10, m_pForm->get<int>("detectionThresholdSecond")));
}

//=============================================================================================================

std::string TriggerDetectionView::getSelectedStimChannel()
{
    return m_pForm->get<std::string>("triggerChannel");
}

//=============================================================================================================

void TriggerDetectionView::clearView()
{
}
