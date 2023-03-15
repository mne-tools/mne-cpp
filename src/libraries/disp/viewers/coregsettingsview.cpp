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
#include <QVector3D>

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
    connect(m_pUi->m_qPushButton_LoadTrans, &QPushButton::released,
            this, &CoregSettingsView::onLoadTrans);
    connect(m_pUi->m_qPushButton_StoreTrans, &QPushButton::released,
            this, &CoregSettingsView::onStoreTrans);
    connect(m_pUi->m_qComboBox_BemItems, &QComboBox::currentTextChanged,
            this, &CoregSettingsView::changeSelectedBem, Qt::UniqueConnection);

    connect(m_pUi->m_qComboBox_BemItems, &QComboBox::currentTextChanged,
            this, &CoregSettingsView::changeSelectedBem, Qt::UniqueConnection);
    connect(m_pUi->m_qComboBox_BemItems, &QComboBox::currentTextChanged,
            this, &CoregSettingsView::changeSelectedBem, Qt::UniqueConnection);

    // Connect Fiducial Pickings
    connect(m_pUi->m_qCheckBox_PickFiducials, &QCheckBox::stateChanged,
            this, &CoregSettingsView::onPickingStatus);
    connect(m_pUi->m_qRadioButton_LPA, &QCheckBox::toggled,
            this, &CoregSettingsView::onFiducialChanged);
    connect(m_pUi->m_qRadioButton_NAS, &QCheckBox::toggled,
            this, &CoregSettingsView::onFiducialChanged);
    connect(m_pUi->m_qRadioButton_RPA, &QCheckBox::toggled,
            this, &CoregSettingsView::onFiducialChanged);
    onPickingStatus();

    // connect icp settings
    connect(m_pUi->m_qPushButton_FitFiducials, &QPushButton::released,
            this, &CoregSettingsView::onFitFiducials);
    connect(m_pUi->m_qPushButton_FitICP, &QPushButton::released,
            this, &CoregSettingsView::onFitICP);

    // connect adjustment settings
    connect(m_pUi->m_qDoubleSpinBox_ScalingX, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_ScalingY, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CoregSettingsView::transParamChanged);
    connect(m_pUi->m_qDoubleSpinBox_ScalingZ, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
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
    connect(m_pUi->m_qComboBox_ScalingMode, &QComboBox::currentTextChanged,
            this, &CoregSettingsView::onScalingModeChanges, Qt::UniqueConnection);
    onScalingModeChanges();

    // set button infos
    setToolTipInfo();
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

void CoregSettingsView::setToolTipInfo()
{
    m_pUi->m_qComboBox_BemItems->setToolTip("Select the Bem to use for coregistration. Load the Bem via: File->Open");
    m_pUi->m_qPushButton_LoadFid->setToolTip("Optional. Load the fiducials from file.");
    m_pUi->m_qPushButton_StoreFid->setToolTip("Store the fiducials to file.");
    m_pUi->m_qPushButton_LoadDig->setToolTip("Load the digitizers from file.");
    m_pUi->m_qPushButton_LoadTrans->setToolTip("Optional. Load the coordinate transformation from file.");
    m_pUi->m_qPushButton_StoreTrans->setToolTip("Store the coordinate transformation to file.");

    m_pUi->m_qDoubleSpinBox_WeightRpa->setToolTip("The weight for the RPA.");
    m_pUi->m_qDoubleSpinBox_WeightLpa->setToolTip("The weight for the LPA.");
    m_pUi->m_qDoubleSpinBox_WeightNas->setToolTip("The weight for the Nasion.");
    m_pUi->m_qDoubleSpinBox_WeightEEG->setToolTip("The weight for the EEG points.");
    m_pUi->m_qDoubleSpinBox_WeightHPI->setToolTip("The weight for the HPI points.");
    m_pUi->m_qDoubleSpinBox_WeightHSP->setToolTip("The weight for the HSP points. HSP = Head Shape Points");
    m_pUi->m_qCheckBox_HSP->setToolTip("Wheater to use the HSP points for the Coregistration. HSP = Head Shape Points");
    m_pUi->m_qCheckBox_EEG->setToolTip("Wheater to use the EEG points for the Coregistration.");
    m_pUi->m_qCheckBox_HPI->setToolTip("Wheater to use the HPI points for the Coregistration.");
    m_pUi->m_qSpinBox_MaxDist->setToolTip("The maximum allowed distace between head surface and digitizer cloud. This is used to discard outliers.");

    m_pUi->m_qCheckBox_AutoScale->setToolTip("Wheater to use automatic scaling for the fiducial alignment.");
    m_pUi->m_qDoubleSpinBox_Converge->setToolTip("The convergence limit for the ICP algorithm.");
    m_pUi->m_qSpinBox_MaxIter->setToolTip("The maximum number of iterations for the ICP algorithm.");
    m_pUi->m_qPushButton_FitFiducials->setToolTip("Fiducial alignment. Apply this step before using the ICP algorithm to get a better first guess.");
    m_pUi->m_qPushButton_FitICP->setToolTip("Co-Registration with the ICP algorithm.");
    m_pUi->m_qLabel_RMSE->setToolTip("The Root-Mean-Square-Error of the distance between closest point and digigizer in mm");
    m_pUi->m_qComboBox_ScalingMode->setToolTip("The scaling Mode. None - No scaling is applied; Uniform - same scaling for x,y,z-axis; 3-Axis - scaling on each axis.");
    m_pUi->m_qDoubleSpinBox_ScalingX->setToolTip("Scaling to apply in x-direction.");
    m_pUi->m_qDoubleSpinBox_ScalingY->setToolTip("Scaling to apply in y-direction.");
    m_pUi->m_qDoubleSpinBox_ScalingZ->setToolTip("Scaling to apply in z-direction.");

    m_pUi->m_qDoubleSpinBox_RotX->setToolTip("Rotation arround x-axis.");
    m_pUi->m_qDoubleSpinBox_RotY->setToolTip("Rotation arround y-axis.");
    m_pUi->m_qDoubleSpinBox_RotZ->setToolTip("Rotation arround z-axis.");

    m_pUi->m_qDoubleSpinBox_TransX->setToolTip("Translation to apply in x-direction.");
    m_pUi->m_qDoubleSpinBox_TransY->setToolTip("Translation to apply in y-direction.");
    m_pUi->m_qDoubleSpinBox_TransZ->setToolTip("Translation to apply in z-direction.");

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
        emit fidFileChanged(sFileName);
    }
}

//=============================================================================================================

void CoregSettingsView::onPickingStatus()
{
    bool bState = m_pUi->m_qCheckBox_PickFiducials->isChecked();
    if(bState) {
        m_pUi->m_qWidget_ResultFiducials->setEnabled(true);
        emit pickFiducials(true);
    } else {
        m_pUi->m_qWidget_ResultFiducials->setEnabled(false);
        emit pickFiducials(false);
    }
    return;
}

//=============================================================================================================

void CoregSettingsView::setFiducials(const QVector3D vecPosition)
{
    QVector3D vecTemp;
    // store incoming vector
    if(m_pUi->m_qRadioButton_LPA->isChecked()) {
        m_vecLPA = vecPosition;

        // step to next fiducial
        m_pUi->m_qRadioButton_NAS->setChecked(true);
        vecTemp = m_vecNAS;
        onFiducialChanged();
    } else if (m_pUi->m_qRadioButton_NAS->isChecked()) {
        m_vecNAS = vecPosition;

        // step to next fiducial
        m_pUi->m_qRadioButton_RPA->setChecked(true);
        vecTemp = m_vecRPA;
        onFiducialChanged();
    } else {
        m_vecRPA = vecPosition;

        // step to next fiducial
        m_pUi->m_qRadioButton_LPA->setChecked(true);
        vecTemp = m_vecLPA;
        onFiducialChanged();
    }

    // floor(vecAxialPosition[0]*100)/100 makes sure to only take 2 decimal positions
    m_pUi->m_qLineEdit_FidX->setText(QString::number(floor(vecTemp[0]*100)/100 * 1000) + " mm" );
    m_pUi->m_qLineEdit_FidY->setText(QString::number(floor(vecTemp[1]*100)/100 * 1000) + " mm" );
    m_pUi->m_qLineEdit_FidZ->setText(QString::number(floor(vecTemp[2]*100)/100 * 1000) + " mm" );
    return;
}

//=============================================================================================================

void CoregSettingsView::onFiducialChanged()
{
    QVector3D vecTemp;
    if(m_pUi->m_qRadioButton_LPA->isChecked()) {
        vecTemp = m_vecLPA;
        emit fiducialChanged(FIFFV_POINT_LPA);
    } else if (m_pUi->m_qRadioButton_NAS->isChecked()) {
        vecTemp = m_vecNAS;
        emit fiducialChanged(FIFFV_POINT_NASION);
    } else {
        vecTemp = m_vecRPA;
        emit fiducialChanged(FIFFV_POINT_RPA);
    }

    // floor(vecAxialPosition[0]*100)/100 makes sure to only take 2 decimal positions
    m_pUi->m_qLineEdit_FidX->setText(QString::number(floor(vecTemp[0]*100)/100 * 1000) + " mm" );
    m_pUi->m_qLineEdit_FidY->setText(QString::number(floor(vecTemp[1]*100)/100 * 1000) + " mm" );
    m_pUi->m_qLineEdit_FidZ->setText(QString::number(floor(vecTemp[2]*100)/100 * 1000) + " mm" );
    return;
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
                                                     tr("Select Transformation"),
                                                     QString(),
                                                     tr("Fif Files (*-trans.fif)"));

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

int CoregSettingsView::getCurrentFiducial()
{
    // choose to use other points as well
    if(m_pUi->m_qRadioButton_LPA->isChecked()) {
        return FIFFV_POINT_LPA;
    } else if(m_pUi->m_qRadioButton_NAS->isChecked()) {
        return FIFFV_POINT_NASION;
    } else if(m_pUi->m_qRadioButton_RPA->isChecked()) {
        return FIFFV_POINT_RPA;
    };
    return -1;
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
    // always use cardinal points
    QList<int> lPicks({FIFFV_POINT_CARDINAL});

    // choose to use other points as well
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
    // block to avoid action loop
    QSignalBlocker blockerComboBox(m_pUi->m_qComboBox_BemItems);
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

void CoregSettingsView::setRMSE(const float fRMSE)
{
    m_pUi->m_qLabel_RMSE->setText(QString::number(fRMSE*1000) + " mm");
}

//=============================================================================================================

void CoregSettingsView::setTransParams(const Vector3f& vecTrans,
                                       const Vector3f& vecRot,
                                       const Vector3f& vecScale)
{
    // block signals to avoid action loop
    QSignalBlocker blockerTransX(m_pUi->m_qDoubleSpinBox_TransX);
    QSignalBlocker blockerTransY(m_pUi->m_qDoubleSpinBox_TransY);
    QSignalBlocker blockerTransZ(m_pUi->m_qDoubleSpinBox_TransZ);
    QSignalBlocker blockerRotX(m_pUi->m_qDoubleSpinBox_RotX);
    QSignalBlocker blockerRotY(m_pUi->m_qDoubleSpinBox_RotY);
    QSignalBlocker blockerRotZ(m_pUi->m_qDoubleSpinBox_RotZ);
    QSignalBlocker blockerScaleX(m_pUi->m_qDoubleSpinBox_ScalingX);
    QSignalBlocker blockerScaleY(m_pUi->m_qDoubleSpinBox_ScalingY);
    QSignalBlocker blockerScaleZ(m_pUi->m_qDoubleSpinBox_ScalingZ);

    m_pUi->m_qDoubleSpinBox_TransX->setValue(vecTrans(0)*1000);
    m_pUi->m_qDoubleSpinBox_TransY->setValue(vecTrans(1)*1000);
    m_pUi->m_qDoubleSpinBox_TransZ->setValue(vecTrans(2)*1000);

    // Inverted order due to euler rotation
    m_pUi->m_qDoubleSpinBox_RotX->setValue(vecRot(2)*180/M_PI);
    m_pUi->m_qDoubleSpinBox_RotY->setValue(vecRot(1)*180/M_PI);
    m_pUi->m_qDoubleSpinBox_RotZ->setValue(vecRot(0)*180/M_PI);

    m_pUi->m_qDoubleSpinBox_ScalingX->setValue(vecScale(2));
    m_pUi->m_qDoubleSpinBox_ScalingY->setValue(vecScale(1));
    m_pUi->m_qDoubleSpinBox_ScalingZ->setValue(vecScale(0));
}

//=============================================================================================================

void CoregSettingsView::getTransParams(Vector3f& vecRot,
                                       Vector3f& vecTrans,
                                       Vector3f& vecScale)
{
    vecTrans(0) = m_pUi->m_qDoubleSpinBox_TransX->value()/1000.0;
    vecTrans(1) = m_pUi->m_qDoubleSpinBox_TransY->value()/1000.0;
    vecTrans(2) = m_pUi->m_qDoubleSpinBox_TransZ->value()/1000.0;

    // Inverted order due to euler rotation
    vecRot(2) = m_pUi->m_qDoubleSpinBox_RotX->value() * M_PI/180.0;
    vecRot(1) = m_pUi->m_qDoubleSpinBox_RotY->value() * M_PI/180.0;
    vecRot(0) = m_pUi->m_qDoubleSpinBox_RotZ->value() * M_PI/180.0;

    // apply different scaling modes
    if (m_pUi->m_qComboBox_ScalingMode->currentText() == "Uniform") {
        vecScale.fill(m_pUi->m_qDoubleSpinBox_ScalingX->value());
    } else if (m_pUi->m_qComboBox_ScalingMode->currentText() == "3-Axis") {
        vecScale(0) = m_pUi->m_qDoubleSpinBox_ScalingX->value();
        vecScale(1) = m_pUi->m_qDoubleSpinBox_ScalingY->value();
        vecScale(2) = m_pUi->m_qDoubleSpinBox_ScalingZ->value();
    } else {
        vecScale.fill(1);
    }

    return;
}

//=============================================================================================================

void CoregSettingsView::onScalingModeChanges()
{
    // apply different scaling modes
    if (m_pUi->m_qComboBox_ScalingMode->currentText() == "Uniform") {
        m_pUi->m_qDoubleSpinBox_ScalingX->setEnabled(true);
        m_pUi->m_qDoubleSpinBox_ScalingY->setEnabled(false);
        m_pUi->m_qDoubleSpinBox_ScalingZ->setEnabled(false);
    } else if (m_pUi->m_qComboBox_ScalingMode->currentText() == "3-Axis") {
        m_pUi->m_qDoubleSpinBox_ScalingX->setEnabled(true);
        m_pUi->m_qDoubleSpinBox_ScalingY->setEnabled(true);
        m_pUi->m_qDoubleSpinBox_ScalingZ->setEnabled(true);
    } else {
        m_pUi->m_qDoubleSpinBox_ScalingX->setEnabled(false);
        m_pUi->m_qDoubleSpinBox_ScalingY->setEnabled(false);
        m_pUi->m_qDoubleSpinBox_ScalingZ->setEnabled(false);
    }
    return;
}

//=============================================================================================================

void CoregSettingsView::onFitFiducials()
{
    m_pUi->m_qCheckBox_PickFiducials->setChecked(false);
    emit fitFiducials();
}

//=============================================================================================================

void CoregSettingsView::onFitICP()
{
    m_pUi->m_qCheckBox_PickFiducials->setChecked(false);
    emit fitICP();
}

//=============================================================================================================

void CoregSettingsView::clearView()
{

}
