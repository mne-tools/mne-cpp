//=============================================================================================================
/**
 * @file     hpisettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Ruben Dörfel. All rights reserved.
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
 * @brief    HpiSettingsView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpisettingsview.h"

#include "ui_hpisettingsview.h"

#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>

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

HpiSettingsView::HpiSettingsView(const QString& sSettingsPath,
                                 QWidget *parent,
                                 Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::HpiSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    connect(m_pUi->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &HpiSettingsView::onLoadDigitizers);
    connect(m_pUi->m_pushButton_doFreqOrder, &QPushButton::clicked,
            this, &HpiSettingsView::doFreqOrder);
    connect(m_pUi->m_pushButton_doSingleFit, &QPushButton::clicked,
            this, &HpiSettingsView::doSingleHpiFit);
    connect(m_pUi->m_tableWidget_Frequencies, &QTableWidget::cellChanged,
            this, &HpiSettingsView::onFrequencyCellChanged);
    connect(m_pUi->m_pushButton_addCoil, &QPushButton::clicked,
            this, &HpiSettingsView::onAddCoil);
    connect(m_pUi->m_pushButton_removeCoil, &QPushButton::clicked,
            this, &HpiSettingsView::onRemoveCoil);
    connect(m_pUi->m_checkBox_useSSP, &QCheckBox::clicked,
            this, &HpiSettingsView::sspStatusChanged);
    connect(m_pUi->m_checkBox_useComp, &QCheckBox::clicked,
            this, &HpiSettingsView::compStatusChanged);
    connect(m_pUi->m_checkBox_continousHPI, &QCheckBox::clicked,
            this, &HpiSettingsView::contHpiStatusChanged);
    connect(m_pUi->m_spinBox_samplesToFit, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &HpiSettingsView::fittingWindowSizeChanged);
    connect(m_pUi->m_doubleSpinBox_maxHPIContinousDist, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HpiSettingsView::allowedMeanErrorDistChanged);
    connect(m_pUi->m_doubleSpinBox_moveThreshold, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HpiSettingsView::allowedMovementChanged);
    connect(m_pUi->m_doubleSpinBox_rotThreshold, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HpiSettingsView::allowedRotationChanged);
    connect(m_pUi->comboBox_coilPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &HpiSettingsView::selectCoilPreset);

    //Init coil freqs
    m_vCoilFreqs << 293 << 307 << 314 << 321;
    qRegisterMetaTypeStreamOperators<QVector<int> >("QVector<int>");

    loadSettings();
}

//=============================================================================================================

HpiSettingsView::~HpiSettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void HpiSettingsView::setErrorLabels(const QVector<double>& vError,
                                     double dMeanErrorDist)
{
    //Update eror labels and change from m to mm
    QString sGof("0mm");

    for(int i = 0; i < vError.size(); ++i) {
        if(i < m_pUi->m_tableWidget_errors->rowCount()) {
            sGof = QString::number(vError[i]*1000,'f',2)+QString(" mm");
            m_pUi->m_tableWidget_errors->item(i, 1)->setText(sGof);
        }
    }

    m_pUi->m_label_averagedFitError->setText(QString::number(dMeanErrorDist*1000,'f',2)+QString(" mm"));

    //Update good/bad fit label
    if(dMeanErrorDist*1000 > m_pUi->m_doubleSpinBox_maxHPIContinousDist->value()) {
        m_pUi->m_label_fitFeedback->setText("Last fit: Bad");
        m_pUi->m_label_fitFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        m_pUi->m_label_fitFeedback->setText("Last fit: Good");
        m_pUi->m_label_fitFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}

//=========================================================================================================

void HpiSettingsView::setMovementResults(double dMovement,
                                         double dRotation)
{
    m_pUi->m_qLineEdit_moveResult->setText(QString::number(dMovement*1000,'f',2) + QString(" mm"));
    m_pUi->m_qLineEdit_rotResult->setText(QString::number(dRotation,'f',2) + QString(" °"));

    if(dMovement*1000 > m_pUi->m_doubleSpinBox_moveThreshold->value() || dRotation > m_pUi->m_doubleSpinBox_rotThreshold->value()) {
        m_pUi->m_label_movementFeedback->setText("Big");
        m_pUi->m_label_movementFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        m_pUi->m_label_movementFeedback->setText("Small");
        m_pUi->m_label_movementFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}

//=============================================================================================================

bool HpiSettingsView::getSspStatusChanged()
{
    return m_pUi->m_checkBox_useSSP->isChecked();
}

//=============================================================================================================

bool HpiSettingsView::getCompStatusChanged()
{
    return m_pUi->m_checkBox_useComp->isChecked();
}

//=============================================================================================================

double HpiSettingsView::getAllowedMeanErrorDistChanged()
{
    return m_pUi->m_doubleSpinBox_maxHPIContinousDist->value();
}

//=============================================================================================================

double HpiSettingsView::getAllowedMovementChanged()
{
    return m_pUi->m_doubleSpinBox_moveThreshold->value();
}

//=============================================================================================================

double HpiSettingsView::getAllowedRotationChanged()
{
    return m_pUi->m_doubleSpinBox_rotThreshold->value();
}

//=============================================================================================================

bool HpiSettingsView::continuousHPIChecked()
{
    return m_pUi->m_checkBox_continousHPI->isChecked();
}


//=============================================================================================================

int HpiSettingsView::getFittingWindowSize()
{
    return m_pUi->m_spinBox_samplesToFit->value();
}

//=============================================================================================================

void HpiSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/coilFreqs"),
                      QVariant::fromValue(m_vCoilFreqs));

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/useSSP"),
                      QVariant::fromValue(m_pUi->m_checkBox_useSSP->isChecked()));

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/useCOMP"),
                      QVariant::fromValue(m_pUi->m_checkBox_useComp->isChecked()));

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/continousHPI"),\
                      QVariant::fromValue(m_pUi->m_checkBox_continousHPI->isChecked()));

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/maxError"),
                      QVariant::fromValue(m_pUi->m_doubleSpinBox_maxHPIContinousDist->value()));

    settings.setValue(m_sSettingsPath + QString("/HpiSettingsView/fittingWindowSize"),
                      QVariant::fromValue(m_pUi->m_spinBox_samplesToFit->value()));
}

//=============================================================================================================

void HpiSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
    QVariant defaultData;

    defaultData.setValue(m_vCoilFreqs);
    m_vCoilFreqs = settings.value(m_sSettingsPath + QString("/HpiSettingsView/coilFreqs"), defaultData).value<QVector<int> >();
    emit coilFrequenciesChanged(m_vCoilFreqs);

    m_pUi->m_checkBox_useSSP->setChecked(settings.value(m_sSettingsPath + QString("/HpiSettingsView/useSSP"), false).toBool());
    m_pUi->m_checkBox_useComp->setChecked(settings.value(m_sSettingsPath + QString("/HpiSettingsView/useCOMP"), false).toBool());
    m_pUi->m_checkBox_continousHPI->setChecked(settings.value(m_sSettingsPath + QString("/HpiSettingsView/continousHPI"), false).toBool());
    m_pUi->m_doubleSpinBox_maxHPIContinousDist->setValue(settings.value(m_sSettingsPath + QString("/HpiSettingsView/maxError"), 10.0).toDouble());
    m_pUi->m_spinBox_samplesToFit->setValue(settings.value(m_sSettingsPath + QString("/HpiSettingsView/fittingWindowSize"), 300).toInt());
}

//=============================================================================================================

void HpiSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void HpiSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void HpiSettingsView::onLoadDigitizers()
{
    //Get file location
    QString fileName_HPI = QFileDialog::getOpenFileName(this,
                                                        tr("Open digitizer file"),
                                                        "",
                                                        tr("Fiff file (*.fif)"));

    if(!fileName_HPI.isEmpty()) {
        m_pUi->m_lineEdit_filePath->setText(fileName_HPI);
    }

    //Load Polhemus file
    if (!fileName_HPI.isEmpty()) {
        fileName_HPI = fileName_HPI.trimmed();
        QFileInfo checkFile(fileName_HPI);

        if (checkFile.exists() && checkFile.isFile()) {
            // Stop cont HPI first
            m_pUi->m_checkBox_continousHPI->setChecked(false);
            emit digitizersChanged(readDigitizersFromFile(fileName_HPI), fileName_HPI);
        } else {
            QMessageBox msgBox;
            msgBox.setText("File could not be loaded!");
            msgBox.exec();
            return;
        }
    }
}

//=============================================================================================================

void HpiSettingsView::onFrequencyCellChanged(int row,
                                             int col)
{
    if(col != 1 || row >= m_vCoilFreqs.size()) {
        return;
    }

    if(QTableWidgetItem *pItem = m_pUi->m_tableWidget_Frequencies->item(row, col)) {
        if(pItem->text() == "none") {
            m_vCoilFreqs[row] = -1;
        } else {
            m_vCoilFreqs[row] = pItem->text().toInt();
        }

        emit coilFrequenciesChanged(m_vCoilFreqs);
    }
}

//=============================================================================================================

void HpiSettingsView::onAddCoil()
{
    if(m_pUi->m_tableWidget_Frequencies->rowCount() + 1 > m_pUi->m_label_numberLoadedCoils->text().toInt()) {
        QMessageBox msgBox;
        msgBox.setText("Cannot add more HPI coils. Not enough digitzed HPI coils loaded.");
        msgBox.exec();
        return;
    }

    // Add column 0 in freq table widget
    m_pUi->m_tableWidget_Frequencies->insertRow(m_pUi->m_tableWidget_Frequencies->rowCount());
    QTableWidgetItem* pTableItemA = new QTableWidgetItem(QString::number(m_pUi->m_tableWidget_Frequencies->rowCount()));
    pTableItemA->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_Frequencies->setItem(m_pUi->m_tableWidget_Frequencies->rowCount()-1,
                                             0,
                                             pTableItemA);

    // Add column 1 in freq table widget
    if(m_vCoilFreqs.size() >= m_pUi->m_tableWidget_Frequencies->rowCount()) {
        m_pUi->m_tableWidget_Frequencies->setItem(m_pUi->m_tableWidget_Frequencies->rowCount()-1,
                                                 1,
                                                 new QTableWidgetItem(QString::number(m_vCoilFreqs.at(m_pUi->m_tableWidget_Frequencies->rowCount()-1))));
    } else {
        m_pUi->m_tableWidget_Frequencies->setItem(m_pUi->m_tableWidget_Frequencies->rowCount()-1,
                                                 1,
                                                 new QTableWidgetItem("none"));
        m_vCoilFreqs.append(-1);
    }

    // Add column 0 in error table widget
    m_pUi->m_tableWidget_errors->insertRow(m_pUi->m_tableWidget_errors->rowCount());
    QTableWidgetItem* pTableItemB = new QTableWidgetItem(QString::number(m_pUi->m_tableWidget_Frequencies->rowCount()));
    pTableItemB->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_errors->setItem(m_pUi->m_tableWidget_errors->rowCount()-1,
                                        0,
                                        pTableItemB);

    // Add column 1 in error table widget
    QTableWidgetItem* pTableItemC = new QTableWidgetItem("0mm");
    pTableItemC->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_errors->setItem(m_pUi->m_tableWidget_errors->rowCount()-1,
                                        1,
                                        pTableItemC);

    emit coilFrequenciesChanged(m_vCoilFreqs);
}

//=============================================================================================================

void HpiSettingsView::onRemoveCoil()
{
    int row = m_pUi->m_tableWidget_Frequencies->currentRow();

    if(row >= 0 && row < m_vCoilFreqs.size()) {
        m_vCoilFreqs.remove(row);
        m_pUi->m_tableWidget_Frequencies->removeRow(row);

        for (int i = 0; i < m_pUi->m_tableWidget_Frequencies->rowCount(); ++i) {
            m_pUi->m_tableWidget_Frequencies->item(i, 0)->setText(QString::number(i+1));
        }

        m_pUi->m_tableWidget_errors->removeRow(row);

        for (int i = 0; i < m_pUi->m_tableWidget_errors->rowCount(); ++i) {
            m_pUi->m_tableWidget_errors->item(i, 0)->setText(QString::number(i+1));
        }

        emit coilFrequenciesChanged(m_vCoilFreqs);
    }
}

//=============================================================================================================

QList<FiffDigPoint> HpiSettingsView::readDigitizersFromFile(const QString& fileName)
{
    clearCoilGUI();

    QFile t_fileDig(fileName);
    FiffDigPointSet t_digSet(t_fileDig);
    updateDigitizerInfoGUI(t_digSet);

    return t_digSet.getList();
}

//=============================================================================================================

void HpiSettingsView::newDigitizerList(QList<FIFFLIB::FiffDigPoint> pointList)
{
    clearCoilGUI();

    FiffDigPointSet t_digSet(pointList);
    updateDigitizerInfoGUI(t_digSet);
}

//=============================================================================================================

void HpiSettingsView::updateDigitizerInfoGUI(const FiffDigPointSet& digSet)
{
    qint16 numHPI = 0;
    qint16 numFiducials = 0;
    qint16 numEEG = 0;
    qint16 numAdditional = 0;

    for(int i = 0; i < digSet.size(); ++i) {
        switch(digSet[i].kind) {
        case FIFFV_POINT_HPI:
            if(m_vCoilFreqs.size() <= numHPI) {
                m_vCoilFreqs.append(-1);
            }
            numHPI++;
            break;
        case FIFFV_POINT_CARDINAL:
            numFiducials++;
            break;
        case FIFFV_POINT_EEG:
            numEEG++;
            break;
        case FIFFV_POINT_EXTRA:
            numAdditional++;
            break;
        }
    }

    //Set loaded number of digitizers
    m_pUi->m_label_numberLoadedCoils->setNum(numHPI);
    m_pUi->m_label_numberLoadedFiducials->setNum(numFiducials);
    m_pUi->m_label_numberLoadedEEG->setNum(numEEG);
    m_pUi->m_label_numberLoadedAdditional->setNum(numAdditional);

    m_vCoilFreqs.resize(numHPI);
    populateCoilGUI();
    setupCoilPresets(numHPI);

    // Make sure that the stored coil freqs always match the number of loaded ones
    m_vCoilFreqs.resize(numHPI);
    emit coilFrequenciesChanged(m_vCoilFreqs);
}

//=============================================================================================================

void HpiSettingsView::populateCoilGUI()
{
    for(int iCoilFreq : m_vCoilFreqs){
        addCoilFreqToGUI(iCoilFreq);
        addCoilErrorToGUI();
    }
}

//=============================================================================================================

void HpiSettingsView::addCoilFreqToGUI(int iCoilFreq)
{
    // Add column 0 in freq table widget
    m_pUi->m_tableWidget_Frequencies->insertRow(m_pUi->m_tableWidget_Frequencies->rowCount());
    QTableWidgetItem* pTableItemA = new QTableWidgetItem(QString::number(m_pUi->m_tableWidget_Frequencies->rowCount()));
    pTableItemA->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_Frequencies->setItem(m_pUi->m_tableWidget_Frequencies->rowCount()-1,
                                             0,
                                             pTableItemA);

    // Add column 1 in freq table widget
    m_pUi->m_tableWidget_Frequencies->setItem(m_pUi->m_tableWidget_Frequencies->rowCount()-1,
                                              1,
                                              new QTableWidgetItem(QString::number(iCoilFreq)));
}

//=============================================================================================================

void HpiSettingsView::addCoilErrorToGUI()
{
    // Add column 0 in error table widget
    m_pUi->m_tableWidget_errors->insertRow(m_pUi->m_tableWidget_errors->rowCount());
    QTableWidgetItem* pTableItemB = new QTableWidgetItem(QString::number(m_pUi->m_tableWidget_Frequencies->rowCount()));
    pTableItemB->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_errors->setItem(m_pUi->m_tableWidget_errors->rowCount()-1,
                                        0,
                                        pTableItemB);

    // Add column 1 in error table widget
    QTableWidgetItem* pTableItemC = new QTableWidgetItem("0mm");
    pTableItemC->setFlags(Qt::ItemIsEnabled);
    m_pUi->m_tableWidget_errors->setItem(m_pUi->m_tableWidget_errors->rowCount()-1,
                                        1,
                                        pTableItemC);
}

//=============================================================================================================

void HpiSettingsView::clearCoilGUI()
{
    m_pUi->m_tableWidget_Frequencies->clear();
    m_pUi->m_tableWidget_Frequencies->setRowCount(0);
    m_pUi->m_tableWidget_Frequencies->setHorizontalHeaderItem(0, new QTableWidgetItem("#Coil"));
    m_pUi->m_tableWidget_Frequencies->setHorizontalHeaderItem(1, new QTableWidgetItem("Frequency (Hz)"));

    m_pUi->m_tableWidget_errors->clear();
    m_pUi->m_tableWidget_errors->setRowCount(0);
    m_pUi->m_tableWidget_errors->setHorizontalHeaderItem(0, new QTableWidgetItem("#Coil"));
    m_pUi->m_tableWidget_errors->setHorizontalHeaderItem(1, new QTableWidgetItem("Error"));
}

//=============================================================================================================

void HpiSettingsView::clearView()
{

}

//==============================================void populateCoilGUI(int iNumCoils);===============================================================

void HpiSettingsView::loadCoilPresets(const QString &sFilePath)
{
    QFile inFile(sFilePath);
    if (inFile.open(QIODevice::ReadOnly)){
        QByteArray inArray = inFile.readAll();
        m_CoilPresets = QJsonDocument::fromJson(inArray);
    }
}

//=============================================================================================================

void HpiSettingsView::setupCoilPresets(int iNumCoils)
{
    if(!m_CoilPresets.isNull()){
        QJsonArray presetData = m_CoilPresets.object()[QString::number(iNumCoils)].toArray();
        populatePresetGUI(presetData);
    }
}

//=============================================================================================================

void HpiSettingsView::populatePresetGUI(const QJsonArray& presetData)
{
    m_pUi->comboBox_coilPreset->clear();
    m_pUi->comboBox_coilPreset->addItem("Load preset");

    for(const auto& entry : presetData){
        QString name = entry.toObject()["name"].toString();
        QVector<int> data;

        for (const auto& coil : entry.toObject()["coils"].toArray()){
            data.append(coil.toInt());
        }

        m_pUi->comboBox_coilPreset->addItem(name, QVariant::fromValue(data));
    }
}

//=============================================================================================================

void HpiSettingsView::selectCoilPreset(int iCoilPresetIndex)
{
    if (iCoilPresetIndex < (m_pUi->comboBox_coilPreset->count())){
        auto coilFreqData = m_pUi->comboBox_coilPreset->itemData(iCoilPresetIndex);
        if (!coilFreqData.isNull() && coilFreqData.canConvert<QVector<int>>()){
            m_vCoilFreqs = coilFreqData.value<QVector<int>>();
            clearCoilGUI();
            populateCoilGUI();

            emit coilFrequenciesChanged(m_vCoilFreqs);
        }
    }
}
