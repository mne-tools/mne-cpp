//=============================================================================================================
/**
* @file     BabyMEGHPIDgl.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Limin Sun and Matti Hamalainen. All rights reserved.
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
* @brief    BabyMEGHPIDgl class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeghpidgl.h"
#include "ui_babymeghpidgl.h"

#include <fiff/fiff_dig_point_set.h>

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>

#include <mne/mne_bem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace FIFFLIB;
using namespace DISP3DLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGHPIDgl::BabyMEGHPIDgl(BabyMEG* p_pBabyMEG,QWidget *parent)
: QWidget(parent)
, ui(new Ui::BabyMEGHPIDgl)
, m_pBabyMEG(p_pBabyMEG)
, m_pView3D(View3D::SPtr(new View3D))
{
    ui->setupUi(this);

    connect(ui->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &BabyMEGHPIDgl::bnLoadPolhemusFile);

    connect(this, &BabyMEGHPIDgl::SendHPIFiffInfo,
            m_pBabyMEG, &BabyMEG::RecvHPIFiffInfo);

    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_pView3D.data());
    ui->m_gridLayout_main->addWidget(pWidgetContainer,0,0,5,1);

    Control3DWidget* control3DWidget = new Control3DWidget();
    control3DWidget->setView3D(m_pView3D);
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(control3DWidget);
    ui->m_groupBox_3dControl->setLayout(gridLayout);

    //Add sensor surface
    QFile t_fileSensorSurfaceBEM("./resources/sensorSurfaces/BabyMEG.fif");
    MNEBem t_sensorSurfaceBEM(t_fileSensorSurfaceBEM);
    m_pView3D->addBemData("Device", "BabyMEG", t_sensorSurfaceBEM);
}


//*************************************************************************************************************

BabyMEGHPIDgl::~BabyMEGHPIDgl()
{
    delete ui;
}


//*************************************************************************************************************

void BabyMEGHPIDgl::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

void BabyMEGHPIDgl::bnLoadPolhemusFile()
{
    //Get file location
    QString fileName_HPI = QFileDialog::getOpenFileName(this,
            tr("Open digitizer file"), "", tr("Fiff file (*.fif)"));

    ui->m_lineEdit_filePath->setText(fileName_HPI);

    //Load Polhemus file
    if (!fileName_HPI.isEmpty()) {
        fileName_HPI = fileName_HPI.trimmed();
        QFileInfo checkFile(fileName_HPI);

        if (checkFile.exists() && checkFile.isFile()) {
            QList<FiffDigPoint> lDigPoints = readPolhemusDig(fileName_HPI);

            if(m_pBabyMEG->m_pFiffInfo) {
                m_pBabyMEG->m_pFiffInfo->dig = lDigPoints;
            }
        } else {
            QMessageBox msgBox;
            msgBox.setText("File could not be loaded!");
            msgBox.exec();
            return;
        }
    }
}


//*************************************************************************************************************

QList<FiffDigPoint> BabyMEGHPIDgl::readPolhemusDig(QString fileName)
{
    QFile t_fileDig(fileName);
    FiffDigPointSet t_digSet(t_fileDig);
    FiffDigPointSet t_digSetWithoutAdditional;

    QList<FiffDigPoint> lDigPoints;

    qint16 numHPI = 0;
    qint16 numDig = 0;
    qint16 numFiducials = 0;
    qint16 numEEG = 0;

    for(int i = 0; i < t_digSet.size(); ++i) {
        lDigPoints.append(t_digSet[i]);

        switch(t_digSet[i].kind)
        {
            case FIFFV_POINT_HPI:
                numHPI++;
                t_digSetWithoutAdditional << t_digSet[i];
                break;

            case FIFFV_POINT_EXTRA:
                numDig++;
                break;

            case FIFFV_POINT_CARDINAL:
                numFiducials++;
                t_digSetWithoutAdditional << t_digSet[i];
                break;

            case FIFFV_POINT_EEG:
                numEEG++;
                t_digSetWithoutAdditional << t_digSet[i];
                break;
        }
    }


    //Add all digitizer but additional points to View3D
    m_pView3D->addDigitizerData("Subject", "Digitizer", t_digSetWithoutAdditional);

    //Set loaded number of digitizers
    ui->m_label_numberLoadedCoils->setNum(numHPI);
    ui->m_label_numberLoadedDigitizers->setNum(numDig);
    ui->m_label_numberLoadedFiducials->setNum(numFiducials);
    ui->m_label_numberLoadedEEG->setNum(numEEG);

    return lDigPoints;
}


