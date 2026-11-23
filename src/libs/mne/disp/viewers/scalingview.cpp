//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     scalingview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Wayne F. Mead <isk@imsorrykun.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ScalingView per-modality scaling panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scalingview.h"
#include "ui_scalingview.h"
#include <mne/fiff/fiff_constants.h>
#include "helpers/scalecontrol.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QKeyEvent>
#include <QCheckBox>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE GLOBAL mne_disp METHODS
//=============================================================================================================

/**
 * Default scales for each channel by type.
 */
const static float m_fScaleMAG = 1e-12f;  /**< Default scale for channel kind and unit of MAG */
const static float m_fScaleGRAD = 1e-15f; /**< Default scale for channel kind and unit of GRAD */
const static float m_fScaleEEG = 1e-5f;   /**< Default scale for channel kind and unit of EEG */
const static float m_fScaleEOG = 1e-6f;   /**< Default scale for channel kind and unit of EOG */
const static float m_fScaleECG = 1e-2f;   /**< Default scale for channel kind and unit of ECG */
const static float m_fScaleSTIM = 1e-3f;  /**< Default scale for channel kind and unit of STIM */
const static float m_fScaleMISC = 1e-3f;  /**< Default scale for channel kind and unit of MISC */
const static float m_fScaleEMG = 1e-3f;   /**< Default scale for channel kind and unit of EMG */
const static int MAG_TO_GRAD_LINK = 31337;

float mne::disp::getDefaultScalingValue(int iChannelKind,
                                        int iChannelUnit)
{
    float fMaxScale(1e-9f);

    switch (iChannelKind) {
        case FIFFV_MEG_CH: {
            if (iChannelUnit == FIFF_UNIT_T_M) { //Gradiometers
                fMaxScale = m_fScaleGRAD;
            } else if (iChannelUnit == FIFF_UNIT_T) { //Magnetometers
                fMaxScale = m_fScaleMAG;
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {
            fMaxScale = m_fScaleMAG;
            break;
        }

        case FIFFV_EEG_CH: { //EEG Channels
            fMaxScale = m_fScaleEEG;
            break;
        }

        case FIFFV_ECG_CH: { //ECG Channels
            fMaxScale = m_fScaleECG;
            break;
        }
        case FIFFV_EMG_CH: { //EMG Channels
            fMaxScale = m_fScaleEMG;
            break;
        }
        case FIFFV_EOG_CH: { //EOG Channels
            fMaxScale = m_fScaleEOG;
            break;
        }

        case FIFFV_STIM_CH: { //STIM Channels
            fMaxScale = m_fScaleSTIM;
            break;
        }

        case FIFFV_MISC_CH: { //MISC Channels
            fMaxScale = m_fScaleMISC;
            break;
        }
    }

    return fMaxScale;
}

//=============================================================================================================

float mne::disp::getScalingValue(const std::map<int, float>& qMapChScaling,
                                 int iChannelKind,
                                 int iChannelUnit)
{
    const auto valueOf = [&qMapChScaling](int key) {
        const auto it = qMapChScaling.find(key);
        return it != qMapChScaling.end() ? it->second : 0.0f;
    };
    float fMaxScale = valueOf(iChannelKind);

    if (iChannelKind == FIFFV_MEG_CH) {
        fMaxScale = valueOf(iChannelUnit);
    }

    if (std::isnan(fMaxScale) || fMaxScale == 0) {
        fMaxScale = mne::disp::getDefaultScalingValue(iChannelKind, iChannelUnit);
    }
    return fMaxScale;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScalingView::ScalingView(const QString& sSettingsPath,
                         QWidget* parent,
                         Qt::WindowFlags f,
                         const std::vector<std::string>& lChannelsToShow)
: AbstractView(parent, f)
, m_lChannelTypesToShow(lChannelsToShow)
, m_pUi(std::make_unique<Ui::ScalingViewWidget>())
, m_bLinkMAGtoGRAD(false)
, m_bIsShiftKeyPressed(false)
, m_bManagingSpinBoxChange(false)
, m_bManagingSliderChange(false)
, m_bManagingLinkMagToGrad(false)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Scaling");
    this->setMinimumWidth(230);

    loadSettings();
    createScaleControls();
    drawScalingGUI();
}

//=============================================================================================================

ScalingView::~ScalingView()
{
    saveSettings();
}

//=============================================================================================================

void ScalingView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_bIsShiftKeyPressed = false;
    }
    QWidget::keyReleaseEvent(event);
}

//=============================================================================================================

void ScalingView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_bIsShiftKeyPressed = true;
    }
    QWidget::keyPressEvent(event);
}

//=============================================================================================================

std::map<int, float> ScalingView::getScaleMap() const
{
    return m_qMapChScaling;
}

//=============================================================================================================

void ScalingView::setScaleMap(const std::map<int, float>& qMapChScaling)
{
    m_qMapChScaling = qMapChScaling;

    createScaleControls();
}

//=============================================================================================================

void ScalingView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    if (m_qMapChScaling.contains(FIFF_UNIT_T)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleMAG"), m_qMapChScaling[FIFF_UNIT_T]);
    }

    if (m_qMapChScaling.contains(FIFF_UNIT_T_M)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleGRAD"), m_qMapChScaling[FIFF_UNIT_T_M]);
    }

    if (m_qMapChScaling.contains(FIFFV_EEG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleEEG"), m_qMapChScaling[FIFFV_EEG_CH]);
    }

    if (m_qMapChScaling.contains(FIFFV_EOG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleEOG"), m_qMapChScaling[FIFFV_EOG_CH]);
    }

    if (m_qMapChScaling.contains(FIFFV_ECG_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleECG"), m_qMapChScaling[FIFFV_ECG_CH]);
    }

    if (m_qMapChScaling.contains(FIFFV_STIM_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleSTIM"), m_qMapChScaling[FIFFV_STIM_CH]);
    }

    if (m_qMapChScaling.contains(FIFFV_MISC_CH)) {
        settings.setValue(m_sSettingsPath + QString("/ScalingView/scaleMISC"), m_qMapChScaling[FIFFV_MISC_CH]);
    }
}

//=============================================================================================================

void ScalingView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    float val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleMAG"), m_fScaleMAG * 3).toFloat();
    m_qMapChScaling.insert_or_assign(FIFF_UNIT_T, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleGRAD"), m_fScaleGRAD * 300 * 100 /*convert cm to m*/).toFloat();
    m_qMapChScaling.insert_or_assign(FIFF_UNIT_T_M, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleEEG"), m_fScaleEEG * 10).toFloat();
    m_qMapChScaling.insert_or_assign(FIFFV_EEG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleEOG"), m_fScaleEOG).toFloat();
    m_qMapChScaling.insert_or_assign(FIFFV_EOG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleECG"), m_fScaleECG).toFloat();
    m_qMapChScaling.insert_or_assign(FIFFV_ECG_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleSTIM"), m_fScaleSTIM).toFloat();
    m_qMapChScaling.insert_or_assign(FIFFV_STIM_CH, val);

    val = settings.value(m_sSettingsPath + QString("/ScalingView/scaleMISC"), m_fScaleMISC).toFloat();
    m_qMapChScaling.insert_or_assign(FIFFV_MISC_CH, val);
}

//=============================================================================================================

void ScalingView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ScalingView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ScalingView::processScalingChange()
{
    emit scalingChanged(m_qMapChScaling);
    saveSettings();
}

//=============================================================================================================

void ScalingView::updateMAGScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFF_UNIT_T, dScale * m_fScaleMAG);
    linkMagToGrad();
    processScalingChange();
}

//=============================================================================================================

void ScalingView::linkMagToGrad()
{
    if (m_bLinkMAGtoGRAD && !m_bManagingLinkMagToGrad) {
        m_bManagingLinkMagToGrad = true;
        m_qMapScaleControls[FIFF_UNIT_T_M]->setValue(m_qMapScaleControls[FIFF_UNIT_T]->value() / (m_qMapScaleControls[MAG_TO_GRAD_LINK]->value() / 100.f));
        m_bManagingLinkMagToGrad = false;
    }
}

//=============================================================================================================

void ScalingView::linkGradToMag()
{
    if (m_bLinkMAGtoGRAD && !m_bManagingLinkMagToGrad) {
        m_bManagingLinkMagToGrad = true;
        m_qMapScaleControls[FIFF_UNIT_T]->setValue(m_qMapScaleControls[FIFF_UNIT_T_M]->value() * (m_qMapScaleControls[MAG_TO_GRAD_LINK]->value() / 100.0f));
        m_bManagingLinkMagToGrad = false;
    }
}

//=============================================================================================================

void ScalingView::updateGRADScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFF_UNIT_T_M, dScale * m_fScaleGRAD * 100.0); //*100 because we have data in fT/cm and we want it in ft/m.
    linkGradToMag();
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateEEGScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_EEG_CH, dScale * m_fScaleEEG);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateEOGScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_EOG_CH, dScale * m_fScaleEOG);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateEMGScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_EMG_CH, dScale * m_fScaleEMG);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateECGScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_ECG_CH, dScale * m_fScaleECG);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateMISCScale(double dScale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_MISC_CH, dScale * m_fScaleMISC);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateSTIMScale(double scale)
{
    m_qMapChScaling.insert_or_assign(FIFFV_STIM_CH, scale * m_fScaleSTIM);
    processScalingChange();
}

//=============================================================================================================

void ScalingView::updateMAGtoGRADlink(double dScale)
{
    Q_UNUSED(dScale)

    linkMagToGrad();
}

//=============================================================================================================

void ScalingView::drawScalingGUI()
{
    int i(0);
    int controls[] = {FIFF_UNIT_T,
                      MAG_TO_GRAD_LINK,
                      FIFF_UNIT_T_M,
                      FIFFV_EEG_CH,
                      FIFFV_EOG_CH,
                      FIFFV_ECG_CH,
                      FIFFV_STIM_CH,
                      FIFFV_MISC_CH};
    for (int c : controls) {
        if (m_qMapScaleControls.contains(c)) {
            m_pUi->verticalLayout->insertWidget(i++, m_qMapScaleControls[c].data());
        }
    }
    createLinkMagGradCheckBox();
}

//=============================================================================================================

void ScalingView::setMagGradLink(Qt::CheckState l)
{
    if (m_qMapScaleControls.contains(MAG_TO_GRAD_LINK)) {
        m_bLinkMAGtoGRAD = (l == Qt::Checked);
    } else {
        m_bLinkMAGtoGRAD = false;
    }
    if (m_bLinkMAGtoGRAD) {
        m_qMapScaleControls[MAG_TO_GRAD_LINK]->setValue(100 * m_qMapScaleControls[FIFF_UNIT_T]->value() / m_qMapScaleControls[FIFF_UNIT_T_M]->value());
    }
    showLinkControl();
}

//=============================================================================================================

void ScalingView::showLinkControl()
{
    if (m_qMapScaleControls.contains(MAG_TO_GRAD_LINK)) {
        m_qMapScaleControls[MAG_TO_GRAD_LINK]->setVisible(m_bLinkMAGtoGRAD);
    }
}

//=============================================================================================================

void ScalingView::createScaleControls()
{
    //MAG
    if (m_qMapChScaling.contains(FIFF_UNIT_T) && (std::ranges::contains(m_lChannelTypesToShow, "mag") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlMAG = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("MAG [pT])")).get();
        pControlMAG->setRange(0.2f, 20.0f);
        pControlMAG->setMaxSensitivityPoint(1.5);
        pControlMAG->setValue(m_qMapChScaling[FIFF_UNIT_T] / m_fScaleMAG);
        pControlMAG->setSensitivity(0.7f);
        pControlMAG->invertSlider(true);
        connect(pControlMAG, &ScaleControl::valueChanged, this, &ScalingView::updateMAGScale);
        m_qMapScaleControls.insert_or_assign(FIFF_UNIT_T, pControlMAG);
    }

    //GRAD
    if (m_qMapChScaling.contains(FIFF_UNIT_T_M) && (std::ranges::contains(m_lChannelTypesToShow, "grad") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlGRAD = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("GRAD [fT/cm]")).get();
        pControlGRAD->setRange(30.0f, 5000.0f);
        pControlGRAD->setMaxSensitivityPoint(100.0f);
        pControlGRAD->setValue(m_qMapChScaling[FIFF_UNIT_T_M] / m_fScaleGRAD);
        pControlGRAD->setSensitivity(0.7f);
        pControlGRAD->invertSlider(true);
        connect(pControlGRAD, &ScaleControl::valueChanged, this, &ScalingView::updateGRADScale);
        m_qMapScaleControls.insert_or_assign(FIFF_UNIT_T_M, pControlGRAD);
    }

    //MAGtoGRADlink only if we have Mags and Grads to link
    if (m_qMapScaleControls.contains(FIFF_UNIT_T) && m_qMapScaleControls.contains(FIFF_UNIT_T_M)) {
        QPointer<ScaleControl> pControlMAGtoGRADlink = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("MAG-GRAD link [cm]")).get();
        pControlMAGtoGRADlink->setRange(0.10f, 8.0f);
        pControlMAGtoGRADlink->setMaxSensitivityPoint(2.0f);
        pControlMAGtoGRADlink->setValue(2.0f);
        pControlMAGtoGRADlink->setSensitivity(0.7f);
        connect(pControlMAGtoGRADlink, &ScaleControl::valueChanged, this, &ScalingView::updateMAGtoGRADlink);
        pControlMAGtoGRADlink->setVisible(m_bLinkMAGtoGRAD);
        m_qMapScaleControls.insert_or_assign(MAG_TO_GRAD_LINK, pControlMAGtoGRADlink);
    }

    //EEG
    if (m_qMapChScaling.contains(FIFFV_EEG_CH) && (std::ranges::contains(m_lChannelTypesToShow, "eeg") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlEEG = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("EEG [uV]")).get();
        pControlEEG->setRange(3.0f, 100.0f);
        pControlEEG->setMaxSensitivityPoint(14.0f);
        pControlEEG->setValue(m_qMapChScaling[FIFFV_EEG_CH] / m_fScaleEEG);
        pControlEEG->setSensitivity(0.4f);
        pControlEEG->invertSlider(true);
        connect(pControlEEG, &ScaleControl::valueChanged, this, &ScalingView::updateEEGScale);
        m_qMapScaleControls.insert_or_assign(FIFFV_EEG_CH, pControlEEG);
    }

    //EOG
    if (m_qMapChScaling.contains(FIFFV_EOG_CH) && (std::ranges::contains(m_lChannelTypesToShow, "eog") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlEOG = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("EOG [uV]")).get();
        pControlEOG->setRange(3.0f, 100.0f);
        pControlEOG->setMaxSensitivityPoint(14.0f);
        pControlEOG->setValue(m_qMapChScaling[FIFFV_EOG_CH] / m_fScaleEOG);
        pControlEOG->setSensitivity(0.4f);
        pControlEOG->invertSlider(true);
        connect(pControlEOG, &ScaleControl::valueChanged, this, &ScalingView::updateEOGScale);
        m_qMapScaleControls.insert_or_assign(FIFFV_EOG_CH, pControlEOG);
    }

    //ECG
    if (m_qMapChScaling.contains(FIFFV_ECG_CH) && (std::ranges::contains(m_lChannelTypesToShow, "ecg") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlECG = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("ECG [uV]")).get();
        pControlECG->setRange(3.0f, 100.0f);
        pControlECG->setMaxSensitivityPoint(14.0f);
        pControlECG->setValue(m_qMapChScaling[FIFFV_ECG_CH] / m_fScaleECG);
        pControlECG->setSensitivity(0.4f);
        pControlECG->invertSlider(true);
        connect(pControlECG, &ScaleControl::valueChanged, this, &ScalingView::updateECGScale);
        m_qMapScaleControls.insert_or_assign(FIFFV_ECG_CH, pControlECG);
    }

    //STIM
    if (m_qMapChScaling.contains(FIFFV_STIM_CH) && (std::ranges::contains(m_lChannelTypesToShow, "stim") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlSTIM = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("STIM")).get();
        pControlSTIM->setRange(1.0f, 99999.0f);
        pControlSTIM->setMaxSensitivityPoint(5000.0f);
        pControlSTIM->setValue(m_qMapChScaling[FIFFV_STIM_CH] / m_fScaleSTIM);
        pControlSTIM->setSensitivity(0.8f);
        pControlSTIM->invertSlider(true);
        connect(pControlSTIM, &ScaleControl::valueChanged, this, &ScalingView::updateSTIMScale);
        m_qMapScaleControls.insert_or_assign(FIFFV_STIM_CH, pControlSTIM);
    }

    //MISC
    if (m_qMapChScaling.contains(FIFFV_MISC_CH) && (std::ranges::contains(m_lChannelTypesToShow, "misc") || std::ranges::contains(m_lChannelTypesToShow, "all"))) {
        QPointer<ScaleControl> pControlMISC = m_vecScaleControls.emplace_back(std::make_unique<ScaleControl>("MISC")).get();
        pControlMISC->setRange(3.0f, 100.0f);
        pControlMISC->setMaxSensitivityPoint(14.0f);
        pControlMISC->setValue(m_qMapChScaling[FIFFV_MISC_CH] / m_fScaleMISC);
        pControlMISC->setSensitivity(0.4f);
        pControlMISC->invertSlider(true);
        connect(pControlMISC, &ScaleControl::valueChanged, this, &ScalingView::updateMISCScale);
        m_qMapScaleControls.insert_or_assign(FIFFV_MISC_CH, pControlMISC);
    }
}

//=============================================================================================================

void ScalingView::clearView()
{
}

//=============================================================================================================

void ScalingView::createLinkMagGradCheckBox()
{
    m_pCheckBox = QPointer<QCheckBox>(new QCheckBox("Link MAGs -> GRADs", this));
    m_pCheckBox->setChecked(m_bLinkMAGtoGRAD);
    connect(m_pCheckBox, &QCheckBox::checkStateChanged, this, &ScalingView::setMagGradLink);
    m_pUi->verticalLayout->addWidget(m_pCheckBox);
}
