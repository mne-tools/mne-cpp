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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HpiSettingsView::HpiSettingsView(QWidget *parent,
                                 Qt::WindowFlags f)
: QWidget(parent, f)
, m_ui(new Ui::HpiSettingsViewWidget)
{
    m_ui->setupUi(this);

    connect(m_ui->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &HpiSettingsView::onLoadDigitizers);
}

//=============================================================================================================

HpiSettingsView::~HpiSettingsView()
{
    delete m_ui;
}

//=============================================================================================================

void HpiSettingsView::onLoadDigitizers()
{
    //Get file location
    QString fileName_HPI = QFileDialog::getOpenFileName(this,
            tr("Open digitizer file"),"", tr("Fiff file (*.fif)"));

    if(!fileName_HPI.isEmpty()) {
        m_ui->m_lineEdit_filePath->setText(fileName_HPI);
    }

    //Load Polhemus file
    if (!fileName_HPI.isEmpty()) {
        fileName_HPI = fileName_HPI.trimmed();
        QFileInfo checkFile(fileName_HPI);

        if (checkFile.exists() && checkFile.isFile()) {
            emit digitizersChanged(readPolhemusDig(fileName_HPI));
        } else {
            QMessageBox msgBox;
            msgBox.setText("File could not be loaded!");
            msgBox.exec();
            return;
        }
    }
}

//=============================================================================================================

QList<FiffDigPoint> HpiSettingsView::readPolhemusDig(const QString& fileName)
{
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
            case FIFFV_POINT_HPI:
                numHPI++;
                break;

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

//    //Hide/show frequencies and errors based on the number of coils
//    if(numHPI == 3) {
//        m_ui->m_label_gofCoil4->hide();
//        m_ui->m_label_gofCoil4Description->hide();
//        m_ui->m_label_freqCoil4->hide();
//        m_ui->m_spinBox_freqCoil4->hide();

//        m_vCoilFreqs.clear();
//        m_vCoilFreqs << 155 << 165 << 190;
//    } else {
//        m_ui->m_label_gofCoil4->show();
//        m_ui->m_label_gofCoil4Description->show();
//        m_ui->m_label_freqCoil4->show();
//        m_ui->m_spinBox_freqCoil4->show();

//        m_vCoilFreqs.clear();
//        m_vCoilFreqs << 155 << 165 << 190 << 200;
//    }

    return lDigPoints;
}
