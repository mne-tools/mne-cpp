//=============================================================================================================
/**
* @file     hpiwidget.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    HPIWidget class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpiwidget.h"
#include "ui_hpiwidget.h"

#include <fiff/fiff_dig_point_set.h>

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>
#include <disp3D/model/data3Dtreemodel.h>

#include <rtProcessing/rthpis.h>

#include <mne/mne_bem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace FIFFLIB;
using namespace DISP3DLIB;
using namespace MNELIB;
using namespace RTPROCESSINGLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HPIWidget::HPIWidget(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo, QWidget *parent)
: QWidget(parent)
, ui(new Ui::HPIWidget)
, m_pFiffInfo(pFiffInfo)
, m_pView3D(View3D::SPtr(new View3D))
, m_pData3DModel(Data3DTreeModel::SPtr(new Data3DTreeModel))
{
    ui->setupUi(this);

    //Do GUI connects
    connect(ui->m_pushButton_doSingleFit, &QPushButton::released,
            this, &HPIWidget::onBtnDoSingleFit);

    connect(ui->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &HPIWidget::onBtnLoadPolhemusFile);

    connect(ui->m_spinBox_freqCoil1, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HPIWidget::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil2, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HPIWidget::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil3, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HPIWidget::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil4, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HPIWidget::onFreqsChanged);

    //Setup View3D
    m_pView3D->setModel(m_pData3DModel);

    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_pView3D.data());
    ui->m_gridLayout_main->addWidget(pWidgetContainer,0,0,5,1);

    QStringList slFlag = QStringList() << "Data";

    Control3DWidget* control3DWidget = new Control3DWidget(this, slFlag);
    control3DWidget->init(m_pData3DModel, m_pView3D);
    control3DWidget->onTreeViewDescriptionHide();

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(control3DWidget);
    ui->m_groupBox_3dControl->setLayout(gridLayout);

    //Add sensor surface
    QFile t_fileSensorSurfaceBEM("./resources/sensorSurfaces/BabyMEG.fif");
    MNEBem t_sensorSurfaceBEM(t_fileSensorSurfaceBEM);
    m_pData3DModel->addBemData("Device", "BabyMEG", t_sensorSurfaceBEM);

    //Always on top
    //this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

    //Init coil freqs
    m_vCoilFreqs << 155 << 165 << 190 << 220;

    //Init data
    m_matValue.resize(0,0);
}


//*************************************************************************************************************

HPIWidget::~HPIWidget()
{
    delete ui;
}


//*************************************************************************************************************

void HPIWidget::setData(const Eigen::MatrixXd& data)
{
    m_matValue = data;
}


//*************************************************************************************************************

void HPIWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

void HPIWidget::setDigitizerDataToView3D(const FiffDigPointSet& digPointSet,
                                             const FiffDigPointSet& fittedPointSet,
                                             const QVector<double>& vGof,
                                             bool bSortOutAdditionalDigitizer)
{
    if(bSortOutAdditionalDigitizer) {
        FiffDigPointSet t_digSetWithoutAdditional;

        for(int i = 0; i < digPointSet.size(); ++i) {
            switch(digPointSet[i].kind)
            {
                case FIFFV_POINT_HPI:
                    t_digSetWithoutAdditional << digPointSet[i];
                    break;

                case FIFFV_POINT_CARDINAL:
                    t_digSetWithoutAdditional << digPointSet[i];
                    break;

                case FIFFV_POINT_EEG:
                    t_digSetWithoutAdditional << digPointSet[i];
                    break;
            }
        }

        m_pData3DModel->addDigitizerData("Head", "Transformed", t_digSetWithoutAdditional);

        t_digSetWithoutAdditional.clear();
        for(int i = 0; i < fittedPointSet.size(); ++i) {
            switch(fittedPointSet[i].kind)
            {
                case FIFFV_POINT_EEG:
                    t_digSetWithoutAdditional << fittedPointSet[i];
                    break;
            }
        }

        m_pData3DModel->addDigitizerData("Head", "Fitted", t_digSetWithoutAdditional);

        //Update gof labels and transform from m to mm
        QString sGof("0mm");
        if(vGof.size() > 0) {
            sGof = QString("%1mm").arg(1000*vGof[0]);
            ui->m_label_gofCoil1->setText(sGof);
        }

        if(vGof.size() > 1) {
            sGof = QString("%1mm").arg(1000*vGof[1]);
            ui->m_label_gofCoil2->setText(sGof);
        }

        if(vGof.size() > 2) {
            sGof = QString("%1mm").arg(1000*vGof[2]);
            ui->m_label_gofCoil3->setText(sGof);
        }

        if(vGof.size() > 3) {
            sGof = QString("%1mm").arg(1000*vGof[3]);
            ui->m_label_gofCoil4->setText(sGof);
        }
    } else {
        m_pData3DModel->addDigitizerData("Head", "Transformed", digPointSet);
        m_pData3DModel->addDigitizerData("Head", "Fitted", fittedPointSet);
    }
}


//*************************************************************************************************************

bool HPIWidget::hpiLoaded()
{
    if(ui->m_label_numberLoadedCoils->text().toInt() >= 3) {
        return true;
    }

    return false;
}


//*************************************************************************************************************

QList<FiffDigPoint> HPIWidget::readPolhemusDig(QString fileName)
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

        switch(t_digSet[i].kind)
        {
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

    //Add all digitizer but additional points to View3D
    QVector<double> vGof;
    vGof << 0.0 << 0.0 << 0.0 << 0.0;

    this->setDigitizerDataToView3D(t_digSet, FiffDigPointSet(), vGof);

    //Set loaded number of digitizers
    ui->m_label_numberLoadedCoils->setNum(numHPI);
    ui->m_label_numberLoadedDigitizers->setNum(numDig);
    ui->m_label_numberLoadedFiducials->setNum(numFiducials);
    ui->m_label_numberLoadedEEG->setNum(numEEG);

    //Hdie show frequencies and errors based on the number of coils
    if(numHPI == 3) {
        ui->m_label_gofCoil4->hide();
        ui->m_label_gofCoil4Description->hide();
        ui->m_label_freqCoil4->hide();
        ui->m_spinBox_freqCoil4->hide();

        m_vCoilFreqs.clear();
        m_vCoilFreqs << 155 << 165 << 190;
    } else {
        ui->m_label_gofCoil4->show();
        ui->m_label_gofCoil4Description->show();
        ui->m_label_freqCoil4->show();
        ui->m_spinBox_freqCoil4->show();

        m_vCoilFreqs.clear();
        m_vCoilFreqs << 155 << 165 << 190 << 220;
    }

    return lDigPoints;
}


//*************************************************************************************************************

void HPIWidget::onBtnDoSingleFit()
{    
    if(!this->hpiLoaded()) {
       QMessageBox msgBox;
       msgBox.setText("Please load a digitizer set with at lesat 3 HPI coils first!");
       msgBox.exec();
       return;
    }

    this->performHPIFitting(m_vCoilFreqs);

    if(m_pFiffInfo) {
        FiffCoordTrans devHeadTrans = m_pFiffInfo->dev_head_t;

        ui->m_label_mat00->setNum(devHeadTrans.trans(0,0));
        ui->m_label_mat01->setNum(devHeadTrans.trans(0,1));
        ui->m_label_mat02->setNum(devHeadTrans.trans(0,2));
        ui->m_label_mat03->setNum(devHeadTrans.trans(0,3));

        ui->m_label_mat10->setNum(devHeadTrans.trans(1,0));
        ui->m_label_mat11->setNum(devHeadTrans.trans(1,1));
        ui->m_label_mat12->setNum(devHeadTrans.trans(1,2));
        ui->m_label_mat13->setNum(devHeadTrans.trans(1,3));

        ui->m_label_mat20->setNum(devHeadTrans.trans(2,0));
        ui->m_label_mat21->setNum(devHeadTrans.trans(2,1));
        ui->m_label_mat22->setNum(devHeadTrans.trans(2,2));
        ui->m_label_mat23->setNum(devHeadTrans.trans(2,3));

        ui->m_label_mat30->setNum(devHeadTrans.trans(3,0));
        ui->m_label_mat31->setNum(devHeadTrans.trans(3,1));
        ui->m_label_mat32->setNum(devHeadTrans.trans(3,2));
        ui->m_label_mat33->setNum(devHeadTrans.trans(3,3));
    }
}


//*************************************************************************************************************

void HPIWidget::onBtnLoadPolhemusFile()
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

            if(m_pFiffInfo) {
                m_pFiffInfo->dig = lDigPoints;
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

void HPIWidget::onFreqsChanged()
{
    m_vCoilFreqs.clear();
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil1->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil2->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil3->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil4->value());
}


//*************************************************************************************************************

void HPIWidget::performHPIFitting(const QVector<int>& vFreqs)
{
    //Generate/Update current dev/head transfomration. We do not need to make use of rtHPI plugin here since the fitting is only needed once here.
    //rt head motion correction will be performed using the rtHPI plugin.
    if(m_pFiffInfo) {
        if(this->hpiLoaded()) {
            // Wait for data
            emit needData();

            while(m_matValue.rows() == 0 && m_matValue.cols() == 0) {
                //Wait unti ldata was received
            }

            //Perform actual fitting
            QVector<double> vGof;
            FiffDigPointSet t_fittedSet;
            RtHPIS::SPtr pRtHpis = RtHPIS::SPtr(new RtHPIS(m_pFiffInfo));
            FiffCoordTrans transDevHead;
            transDevHead.from = 1;
            transDevHead.to = 4;

            qDebug() << "HPIWidget::performHPIFitting - 0";
            pRtHpis->singleHPIFit(m_matValue,
                                  transDevHead,
                                  vFreqs,
                                  vGof,
                                  t_fittedSet);

            qDebug() << "HPIWidget::performHPIFitting - 1";
            m_matValue.resize(0,0);

            //Set newly calculated transforamtion amtrix to fiff info
            m_pFiffInfo->dev_head_t = transDevHead;

            //Apply new dev/head matrix to current digitizer and update in 3D view in HPI control widget
            FiffDigPointSet t_digSet;

            for(int i = 0; i < m_pFiffInfo->dig.size(); ++i) {
                FiffDigPoint digPoint = m_pFiffInfo->dig.at(i);

                MatrixX3f matPos(1,3);
                matPos(0,0) = digPoint.r[0];
                matPos(0,1) = digPoint.r[1];
                matPos(0,2) = digPoint.r[2];

                MatrixX3f matPosTrans = m_pFiffInfo->dev_head_t.apply_inverse_trans(matPos);

                digPoint.r[0] = matPosTrans(0,0);
                digPoint.r[1] = matPosTrans(0,1);
                digPoint.r[2] = matPosTrans(0,2);

                t_digSet << digPoint;
            }

            this->setDigitizerDataToView3D(t_digSet, t_fittedSet, vGof);
        }
    }
}
