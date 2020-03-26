//=============================================================================================================
/**
 * @file     hpisettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

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
: QWidget(parent, f)
, m_ui(new Ui::HpiSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
{
    m_ui->setupUi(this);

    connect(m_ui->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &HpiSettingsView::onLoadDigitizers);
    connect(m_ui->m_pushButton_doFreqOrder, &QPushButton::clicked,
            this, &HpiSettingsView::doFreqOrder);
    connect(m_ui->m_pushButton_doSingleFit, &QPushButton::clicked,
            this, &HpiSettingsView::doSingleHpiFit);
    connect(m_ui->m_tableWidget_Frequencies, &QTableWidget::cellChanged,
            this, &HpiSettingsView::onFrequencyCellChanged);
    connect(m_ui->m_pushButton_addCoil, &QPushButton::clicked,
            this, &HpiSettingsView::onAddCoil);
    connect(m_ui->m_pushButton_removeCoil, &QPushButton::clicked,
            this, &HpiSettingsView::onRemoveCoil);
    connect(m_ui->m_checkBox_useSSP, &QCheckBox::clicked,
            this, &HpiSettingsView::sspStatusChanged);
    connect(m_ui->m_checkBox_useComp, &QCheckBox::clicked,
            this, &HpiSettingsView::compStatusChanged);
    connect(m_ui->m_checkBox_continousHPI, &QCheckBox::clicked,
            this, &HpiSettingsView::contHpiStatusChanged);
    connect(m_ui->m_doubleSpinBox_maxHPIContinousDist, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HpiSettingsView::allowedMeanErrorDistChanged);

    //Init coil freqs
    m_vCoilFreqs << 155 << 165 << 190 << 200;
    qRegisterMetaTypeStreamOperators<QVector<int> >("QVector<int>");

    loadSettings(m_sSettingsPath);
}

//=============================================================================================================

HpiSettingsView::~HpiSettingsView()
{
    saveSettings(m_sSettingsPath);

    delete m_ui;
}

//=============================================================================================================

void HpiSettingsView::setErrorLabels(const QVector<double>& vError,
                                     double dMeanErrorDist)
{
    //Update eror labels and change from m to mm
    QString sGof("0mm");

    for(int i = 0; i < vError.size(); ++i) {
        if(i < m_ui->m_tableWidget_errors->rowCount()) {
            sGof = QString::number(vError[i]*1000,'f',2)+QString("mm");
            m_ui->m_tableWidget_errors->item(i, 1)->setText(sGof);
        }
    }

    m_ui->m_label_averagedFitError->setText(QString::number(dMeanErrorDist*1000,'f',2)+QString("mm"));

    //Update good/bad fit label
    if(dMeanErrorDist*1000 > m_ui->m_doubleSpinBox_maxHPIContinousDist->value()) {
        m_ui->m_label_fitFeedback->setText("Last fit: Bad");
        m_ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        m_ui->m_label_fitFeedback->setText("Last fit: Good");
        m_ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}

//=============================================================================================================

bool HpiSettingsView::getSspStatusChanged()
{
    return m_ui->m_checkBox_useSSP->isChecked();
}

//=============================================================================================================

bool HpiSettingsView::getCompStatusChanged()
{
    return m_ui->m_checkBox_useComp->isChecked();
}

//=============================================================================================================

double HpiSettingsView::getAllowedMeanErrorDistChanged()
{
    return m_ui->m_doubleSpinBox_maxHPIContinousDist->value();
}

//=============================================================================================================

void HpiSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
    QVariant data;

    data.setValue(m_vCoilFreqs);
    settings.setValue(settingsPath + QString("/coilFreqs"), data);

    data.setValue(m_ui->m_checkBox_useSSP->isChecked());
    settings.setValue(settingsPath + QString("/useSSP"), data);

    data.setValue(m_ui->m_checkBox_useComp->isChecked());
    settings.setValue(settingsPath + QString("/useCOMP"), data);

    data.setValue(m_ui->m_doubleSpinBox_maxHPIContinousDist->value());
    settings.setValue(settingsPath + QString("/maxError"), data);
}

//=============================================================================================================

void HpiSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
    QVariant defaultData;

    defaultData.setValue(m_vCoilFreqs);
    m_vCoilFreqs = settings.value(settingsPath + QString("/coilFreqs"), defaultData).value<QVector<int> >();
    emit coilFrequenciesChanged(m_vCoilFreqs);

    m_ui->m_checkBox_useSSP->setChecked(settings.value(settingsPath + QString("/useSSP"), false).toBool());
    m_ui->m_checkBox_useComp->setChecked(settings.value(settingsPath + QString("/useCOMP"), false).toBool());
    m_ui->m_doubleSpinBox_maxHPIContinousDist->setValue(settings.value(settingsPath + QString("/maxError"), 10.0).toDouble());
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
        m_ui->m_lineEdit_filePath->setText(fileName_HPI);
    }

    //Load Polhemus file
    if (!fileName_HPI.isEmpty()) {
        fileName_HPI = fileName_HPI.trimmed();
        QFileInfo checkFile(fileName_HPI);

        if (checkFile.exists() && checkFile.isFile()) {
            // Stop cont HPI first
            m_ui->m_checkBox_continousHPI->setChecked(false);
            emit digitizersChanged(readPolhemusDig(fileName_HPI), fileName_HPI);
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

    if(QTableWidgetItem *pItem = m_ui->m_tableWidget_Frequencies->item(row, col)) {
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
    if(m_ui->m_tableWidget_Frequencies->rowCount() + 1 > m_ui->m_label_numberLoadedCoils->text().toInt()) {
        QMessageBox msgBox;
        msgBox.setText("Cannot add more HPI coils. Not enough digitzed HPI coils loaded.");
        msgBox.exec();
        return;
    }

    // Add column 0 in freq table widget
    m_ui->m_tableWidget_Frequencies->insertRow(m_ui->m_tableWidget_Frequencies->rowCount());
    QTableWidgetItem* pTableItemA = new QTableWidgetItem(QString::number(m_ui->m_tableWidget_Frequencies->rowCount()));
    pTableItemA->setFlags(Qt::ItemIsEnabled);
    m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                             0,
                                             pTableItemA);

    // Add column 1 in freq table widget
    if(m_vCoilFreqs.size() >= m_ui->m_tableWidget_Frequencies->rowCount()) {
        m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                                 1,
                                                 new QTableWidgetItem(QString::number(m_vCoilFreqs.at(m_ui->m_tableWidget_Frequencies->rowCount()-1))));
    } else {
        m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                                 1,
                                                 new QTableWidgetItem("none"));
        m_vCoilFreqs.append(-1);
    }

    // Add column 0 in error table widget
    m_ui->m_tableWidget_errors->insertRow(m_ui->m_tableWidget_errors->rowCount());
    QTableWidgetItem* pTableItemB = new QTableWidgetItem(QString::number(m_ui->m_tableWidget_Frequencies->rowCount()));
    pTableItemB->setFlags(Qt::ItemIsEnabled);
    m_ui->m_tableWidget_errors->setItem(m_ui->m_tableWidget_errors->rowCount()-1,
                                        0,
                                        pTableItemB);

    // Add column 1 in error table widget
    QTableWidgetItem* pTableItemC = new QTableWidgetItem("0mm");
    pTableItemC->setFlags(Qt::ItemIsEnabled);
    m_ui->m_tableWidget_errors->setItem(m_ui->m_tableWidget_errors->rowCount()-1,
                                        1,
                                        pTableItemC);

    emit coilFrequenciesChanged(m_vCoilFreqs);
}

//=============================================================================================================

void HpiSettingsView::onRemoveCoil()
{
    int row = m_ui->m_tableWidget_Frequencies->currentRow();

    if(row >= 0 && row < m_vCoilFreqs.size()) {
        m_vCoilFreqs.remove(row);
        m_ui->m_tableWidget_Frequencies->removeRow(row);

        for (int i = 0; i < m_ui->m_tableWidget_Frequencies->rowCount(); ++i) {
            m_ui->m_tableWidget_Frequencies->item(i, 0)->setText(QString::number(i+1));
        }

        m_ui->m_tableWidget_errors->removeRow(row);

        for (int i = 0; i < m_ui->m_tableWidget_errors->rowCount(); ++i) {
            m_ui->m_tableWidget_errors->item(i, 0)->setText(QString::number(i+1));
        }

        emit coilFrequenciesChanged(m_vCoilFreqs);
    }
}

//=============================================================================================================

QList<FiffDigPoint> HpiSettingsView::readPolhemusDig(const QString& fileName)
{
    m_ui->m_tableWidget_Frequencies->clear();
    m_ui->m_tableWidget_Frequencies->setRowCount(0);
    m_ui->m_tableWidget_Frequencies->setHorizontalHeaderItem(0, new QTableWidgetItem("#Coil"));
    m_ui->m_tableWidget_Frequencies->setHorizontalHeaderItem(1, new QTableWidgetItem("Frequency (Hz)"));

    m_ui->m_tableWidget_errors->clear();
    m_ui->m_tableWidget_errors->setRowCount(0);
    m_ui->m_tableWidget_errors->setHorizontalHeaderItem(0, new QTableWidgetItem("#Coil"));
    m_ui->m_tableWidget_errors->setHorizontalHeaderItem(1, new QTableWidgetItem("Error"));

    QFile t_fileDig(fileName);
    FiffDigPointSet t_digSet(t_fileDig);

    QList<FiffDigPoint> lDigPoints;

    qint16 numHPI = 0;
    qint16 numDig = 0;
    qint16 numFiducials = 0;
    qint16 numEEG = 0;

    for(int i = 0; i < t_digSet.size(); ++i) {
        lDigPoints.append(t_digSet[i]);

        switch(t_digSet[i].kind) {
            case FIFFV_POINT_HPI: {
                // Add column 0 in freq table widget
                m_ui->m_tableWidget_Frequencies->insertRow(m_ui->m_tableWidget_Frequencies->rowCount());
                QTableWidgetItem* pTableItemA = new QTableWidgetItem(QString::number(m_ui->m_tableWidget_Frequencies->rowCount()));
                pTableItemA->setFlags(Qt::ItemIsEnabled);
                m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                                         0,
                                                         pTableItemA);

                // Add column 1 in freq table widget
                if(m_vCoilFreqs.size() > numHPI) {
                    m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                                             1,
                                                             new QTableWidgetItem(QString::number(m_vCoilFreqs.at(numHPI))));
                } else {
                    m_ui->m_tableWidget_Frequencies->setItem(m_ui->m_tableWidget_Frequencies->rowCount()-1,
                                                             1,
                                                             new QTableWidgetItem("none"));
                    m_vCoilFreqs.append(-1);
                }

                // Add column 0 in error table widget
                m_ui->m_tableWidget_errors->insertRow(m_ui->m_tableWidget_errors->rowCount());
                QTableWidgetItem* pTableItemB = new QTableWidgetItem(QString::number(m_ui->m_tableWidget_Frequencies->rowCount()));
                pTableItemB->setFlags(Qt::ItemIsEnabled);
                m_ui->m_tableWidget_errors->setItem(m_ui->m_tableWidget_errors->rowCount()-1,
                                                    0,
                                                    pTableItemB);

                // Add column 1 in error table widget
                QTableWidgetItem* pTableItemC = new QTableWidgetItem("0mm");
                pTableItemC->setFlags(Qt::ItemIsEnabled);
                m_ui->m_tableWidget_errors->setItem(m_ui->m_tableWidget_errors->rowCount()-1,
                                                    1,
                                                    pTableItemC);

                numHPI++;
                break;
            }

            case FIFFV_POINT_EXTRA:
                numDig++;
                break;

            case FIFFV_POINT_CARDINAL:
                numFiducials++;
                break;

            case FIFFV_POINT_EEG:
                numEEG++;
                break;
        }
    }

    //Set loaded number of digitizers
    m_ui->m_label_numberLoadedCoils->setNum(numHPI);
    m_ui->m_label_numberLoadedDigitizers->setNum(numDig);
    m_ui->m_label_numberLoadedFiducials->setNum(numFiducials);
    m_ui->m_label_numberLoadedEEG->setNum(numEEG);

    // Make sure that the stored coil freqs always match the number of loaded ones
    m_vCoilFreqs.resize(numHPI);
    emit coilFrequenciesChanged(m_vCoilFreqs);

    return lDigPoints;
}
