//=============================================================================================================
/**
 * @file     coregsettingsview.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    CoregSettingsView class definition.
 *
 */

# define M_PI           3.14159265358979323846  /* pi */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregsettingsview.h"
#include "ui_coregsettingsview.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================
#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CoregSettingsView::CoregSettingsView(const QString& sSettingsPath,
                     QWidget *parent,
                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::CoregSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    loadSettings();

    // Connect File loading and saving
    connect(m_pUi->m_qPushButton_LoadFid, &QPushButton::released,
            this, &CoregSettingsView::onLoadFidFile);
    connect(m_pUi->m_qPushButton_StoreFid, &QPushButton::released,
            this, &CoregSettingsView::onStoreFidFile);
    connect(m_pUi->m_qPushButton_LoadDig, &QPushButton::released,
            this, &CoregSettingsView::onLoadDigFile);
//    connect(m_pUi->m_qPushButton_LoadTrans &QPushButton::released,
//            this, &CoregSettingsView::onLoadTrans);
//    connect(m_pUi->m_qPushButton_StoreTrans &QPushButton::released,
//            this, &CoregSettingsView::onStoreTrans);
    connect(m_pUi->m_qComboBox_BemItems, &QComboBox::currentTextChanged,
            this, &CoregSettingsView::changeSelectedBem, Qt::UniqueConnection);

    m_pUi->m_qGroupBox_StoreTrans->hide();


    // connect icp settings
    connect(m_pUi->m_qPushButton_FitFiducials, &QPushButton::released,
            this, &CoregSettingsView::fitFiducials);
    connect(m_pUi->m_qPushButton_FitICP, &QPushButton::released,
            this, &CoregSettingsView::fitICP);

    // connect adjustment settings
    connect(m_pUi->m_qDoubleSpinBox_X, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_Y, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_Z, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_RotX, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_RotY, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_RotZ, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_TransX, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_TransY, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_TransZ, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
}

//=============================================================================================================

CoregSettingsView::~CoregSettingsView()
{
    saveSettings();
    delete m_pUi;
}

//=============================================================================================================

void CoregSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

}

//=============================================================================================================

void CoregSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

}

//=============================================================================================================

void CoregSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
    case GuiMode::Clinical:
        break;
    default: // default is research mode
        break;
    }
}

//=============================================================================================================

void CoregSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
    case ProcessingMode::Offline:
        break;
    default: // default is realtime mode
        break;
    }
}

//=============================================================================================================

void CoregSettingsView::onLoadFidFile()
{
    QString sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select fiducials"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    if (sFileName.isEmpty()) {
        return;
    } else {
        std::cout << sFileName.toUtf8().constData() << std::endl;
        emit fidFileChanged(sFileName);
    }
}

//=============================================================================================================

void CoregSettingsView::onStoreFidFile()
{
    QString sFileName = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                     tr("Save Fiducials"), "",
                                                     tr("Fif file (*fiducials.fif)"));

    if (sFileName.isEmpty()) {
        return;
    } else {
        emit fidStoreFileChanged(sFileName);
    }
}

//=============================================================================================================

void CoregSettingsView::onLoadDigFile()
{
    QString sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select digitizer file"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    if (sFileName.isEmpty()) {
        return;
    } else {
        emit digFileChanged(sFileName);
    }
}

//=============================================================================================================

void CoregSettingsView::onLoadTrans()
{
    QString sFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Select Transformation file"),
                                                       QString(),
                                                       tr("Fif Files (*.fif)"));

    if (sFileName.isEmpty()) {
        return;
    } else {
        emit loadTrans(sFileName);
    }
}

//=============================================================================================================

void CoregSettingsView::onStoreTrans()
{
    QString sFileName = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                     tr("Save Transformation"), "",
                                                     tr("Fif file (*-trans.fif)"));

    if (sFileName.isEmpty()) {
        return;
    } else{
        emit storeTrans(sFileName);
    }
}

//=============================================================================================================

int CoregSettingsView::getMaxIter()
{
    return m_pUi->m_qSpinBox_MaxIter->value();
}

//=============================================================================================================

float CoregSettingsView::getConvergence()
{
    return m_pUi->m_qDoubleSpinBox_Converge->value()/1000;
}

//=============================================================================================================

bool CoregSettingsView::getAutoScale()
{
    return m_pUi->m_qCheckBox_AutoScale->isChecked();
}

//=============================================================================================================

float CoregSettingsView::getWeightLPA()
{
    return m_pUi->m_qDoubleSpinBox_WeightLpa->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getWeightRPA()
{
    return m_pUi->m_qDoubleSpinBox_WeightRpa->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getWeightNAS()
{
    return m_pUi->m_qDoubleSpinBox_WeightNas->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getWeightEEG()
{
    return m_pUi->m_qDoubleSpinBox_WeightEEG->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getWeightHPI()
{
    return m_pUi->m_qDoubleSpinBox_WeightHPI->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getWeightHSP()
{
    return m_pUi->m_qDoubleSpinBox_WeightHSP->text().toFloat();
}

//=============================================================================================================

float CoregSettingsView::getOmmitDistance()
{
    return static_cast<float>(m_pUi->m_qSpinBox_MaxDist->value())/1000.0;
}

//=============================================================================================================

QList<int> CoregSettingsView::getDigitizerCheckState()
{
    QList<int> lPicks({FIFFV_POINT_CARDINAL});
    if(m_pUi->m_qCheckBox_EEG) {
        lPicks << FIFFV_POINT_EEG;
    }
    if(m_pUi->m_qCheckBox_HPI) {
        lPicks << FIFFV_POINT_HPI;
    }
    if(m_pUi->m_qCheckBox_HSP) {
        lPicks << FIFFV_POINT_EXTRA;
    }
    return lPicks;
}

//=============================================================================================================

void CoregSettingsView::clearSelectionBem()
{
    m_pUi->m_qComboBox_BemItems->clear();
}

//=============================================================================================================

void CoregSettingsView::addSelectionBem(const QString& sBemName)
{
    m_pUi->m_qComboBox_BemItems->addItem(sBemName);
}

//=============================================================================================================

QString CoregSettingsView::getCurrentSelectedBem()
{
    return m_pUi->m_qComboBox_BemItems->currentText();
}

//=============================================================================================================

void CoregSettingsView::setOmittedPoints(const int iN)
{
    m_pUi->m_qLabel_NOmitted->setText(QString::number(iN));
}

//=============================================================================================================

void CoregSettingsView::setTransParams(const Vector3f& vecTrans,
                                       const Vector3f& vecRot,
                                       const Vector3f& vecScale)
{
    QSignalBlocker blockerTransX(m_pUi->m_qDoubleSpinBox_TransX);
    QSignalBlocker blockerTransY(m_pUi->m_qDoubleSpinBox_TransY);
    QSignalBlocker blockerTransZ(m_pUi->m_qDoubleSpinBox_TransZ);
    QSignalBlocker blockerRotX(m_pUi->m_qDoubleSpinBox_RotX);
    QSignalBlocker blockerRotY(m_pUi->m_qDoubleSpinBox_RotY);
    QSignalBlocker blockerRotZ(m_pUi->m_qDoubleSpinBox_RotZ);
    QSignalBlocker blockerScaleX(m_pUi->m_qDoubleSpinBox_X);
    QSignalBlocker blockerScaleY(m_pUi->m_qDoubleSpinBox_Y);
    QSignalBlocker blockerScaleZ(m_pUi->m_qDoubleSpinBox_Z);

    m_pUi->m_qDoubleSpinBox_TransX->setValue(vecTrans(0)*1000);
    m_pUi->m_qDoubleSpinBox_TransY->setValue(vecTrans(1)*1000);
    m_pUi->m_qDoubleSpinBox_TransZ->setValue(vecTrans(2)*1000);

    m_pUi->m_qDoubleSpinBox_RotX->setValue(vecRot(0)*180/M_PI);
    m_pUi->m_qDoubleSpinBox_RotY->setValue(vecRot(1)*180/M_PI);
    m_pUi->m_qDoubleSpinBox_RotZ->setValue(vecRot(2)*180/M_PI);

    m_pUi->m_qDoubleSpinBox_X->setValue(vecScale(0));
    m_pUi->m_qDoubleSpinBox_Y->setValue(vecScale(1));
    m_pUi->m_qDoubleSpinBox_Z->setValue(vecScale(2));
}

//=============================================================================================================

void CoregSettingsView::getTransParams(Vector3f& vecRot,
                                       Vector3f& vecTrans,
                                       Vector3f& vecScale)
{
    vecTrans(0) = m_pUi->m_qDoubleSpinBox_TransX->value()/1000.0;
    vecTrans(1) = m_pUi->m_qDoubleSpinBox_TransY->value()/1000.0;
    vecTrans(2) = m_pUi->m_qDoubleSpinBox_TransZ->value()/1000.0;

    vecRot(0) = m_pUi->m_qDoubleSpinBox_RotX->value() * M_PI/180.0;
    vecRot(1) = m_pUi->m_qDoubleSpinBox_RotY->value() * M_PI/180.0;
    vecRot(2) = m_pUi->m_qDoubleSpinBox_RotZ->value() * M_PI/180.0;

    // ToDo implement scaling modes
    vecScale(0) = m_pUi->m_qDoubleSpinBox_X->value();
    vecScale(1) = m_pUi->m_qDoubleSpinBox_Y->value();
    vecScale(2) = m_pUi->m_qDoubleSpinBox_Z->value();
    qDebug() << "it is happenig";
    return;
}
