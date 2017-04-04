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

#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/control/control3dwidget.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/bem/bemtreeitem.h>
#include <disp3D/engine/model/items/bem/bemsurfacetreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizersettreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizertreeitem.h>
#include <disp3D/engine/model/3dhelpers/renderable3Dentity.h>

#include <inverse/hpiFit/hpifit.h>

#include <mne/mne_bem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <Qt3DCore/QTransform>


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
using namespace INVERSELIB;


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
, m_pRtHPI(RtHPIS::SPtr(new RtHPIS(m_pFiffInfo)))
, m_dMaxHPIFitError(0.01)
, m_dMeanErrorDist(0.0)
, m_iNubmerBadChannels(0)
, m_bUseSSP(false)
, m_bUseComp(true)
, m_bLastFitGood(false)
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

    connect(ui->m_checkBox_continousHPI, &QCheckBox::clicked,
            this, &HPIWidget::onDoContinousHPI);

    connect(ui->m_doubleSpinBox_maxHPIContinousDist, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HPIWidget::onContinousHPIMaxDistChanged);

    connect(ui->m_checkBox_useSSP, &QCheckBox::clicked,
            this, &HPIWidget::onSSPCompUsageChanged);
    connect(ui->m_checkBox_useComp, &QCheckBox::clicked,
            this, &HPIWidget::onSSPCompUsageChanged);

    //Init from default values
    ui->m_checkBox_useSSP->setChecked(m_bUseSSP);
    ui->m_checkBox_useComp->setChecked(m_bUseComp);

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
    QFile t_fileBabyMEGSensorSurfaceBEM("./resources/sensorSurfaces/BabyMEG.fif");
    MNEBem t_babyMEGsensorSurfaceBEM(t_fileBabyMEGSensorSurfaceBEM);
    m_pData3DModel->addMegSensorData("Device", "BabyMEG", t_babyMEGsensorSurfaceBEM);

    QFile t_fileVVSensorSurfaceBEM("./resources/sensorSurfaces/306m.fif");
    MNEBem t_sensorVVSurfaceBEM(t_fileVVSensorSurfaceBEM);
    BemTreeItem* pVVItem = m_pData3DModel->addBemData("Device", "VectorView", t_sensorVVSurfaceBEM);
    pVVItem->setCheckState(Qt::Unchecked);

    QFile t_fileHeadKid("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_BemHeadKid(t_fileHeadKid);
    m_pBemHeadKid = m_pData3DModel->addBemData("Head", "Child", t_BemHeadKid);
    m_pBemHeadKid->setCheckState(Qt::Unchecked);

    QFile t_fileHeadAdult("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_BemHeadAdult(t_fileHeadAdult);
    m_pBemHeadAdult = m_pData3DModel->addBemData("Head", "Adult", t_BemHeadAdult);
    m_pBemHeadAdult->setCheckState(Qt::Unchecked);

    //Always on top
    //this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

    //Init coil freqs
    m_vCoilFreqs << 155 << 165 << 190 << 220;

    //Init data
    m_matValue.resize(0,0);

    //Init RtHPIs
    m_pRtHPI->setCoilFrequencies(m_vCoilFreqs);
    connect(m_pRtHPI.data(), &RtHPIS::newFittingResultAvailable,
            this, &HPIWidget::onNewFittingResultAvailable);
}


//*************************************************************************************************************

HPIWidget::~HPIWidget()
{
    delete ui;
}


//*************************************************************************************************************

void HPIWidget::setData(const Eigen::MatrixXd& matData)
{
    //If bad channels changed, recalcluate projectors
    if(m_iNubmerBadChannels != m_pFiffInfo->bads.size() || m_matCompProjectors.rows() == 0 || m_matCompProjectors.cols() == 0) {
        updateProjections();
        m_iNubmerBadChannels = m_pFiffInfo->bads.size();
    }

    m_matValue = m_matCompProjectors * matData;

    //Do continous HPI if wanted
    if(ui->m_checkBox_continousHPI->isChecked()) {
        m_pRtHPI->append(m_matValue);
    }
}


//*************************************************************************************************************

QVector<double> HPIWidget::getGOF()
{
    return m_vGof;
}


//*************************************************************************************************************

bool HPIWidget::wasLastFitOk()
{
    return m_bLastFitGood;
}


//*************************************************************************************************************

void HPIWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}


//*************************************************************************************************************

void HPIWidget::updateProjections()
{
    m_matProjectors = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    Eigen::MatrixXd matComp = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    if(m_bUseSSP) {
        // Use SSP + SGM + calibration
        //Do a copy here because we are going to change the activity flags of the SSP's
        FiffInfo infoTemp = *(m_pFiffInfo.data());

        //Turn on all SSP
        for(int i = 0; i < infoTemp.projs.size(); ++i) {
            infoTemp.projs[i].active = true;
        }

        //Create the projector for all SSP's on
        infoTemp.make_projector(m_matProjectors);
        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
            m_matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
        }
    }

    if(m_bUseComp) {
        // Setup Comps
        FiffCtfComp newComp;
        m_pFiffInfo->make_compensator(0, 101, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
        matComp = newComp.data->data;
    }

    m_matCompProjectors = m_matProjectors * matComp;

    m_pRtHPI->setProjectionMatrix(m_matProjectors);
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

QList<FiffDigPoint> HPIWidget::readPolhemusDig(const QString& fileName)
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

    //Add all digitizer but additional points to the 3D view
    FiffDigPointSet t_digSetWithoutAdditional = t_digSet.pickTypes(QList<int>()<<FIFFV_POINT_HPI<<FIFFV_POINT_CARDINAL<<FIFFV_POINT_EEG);
    m_pTrackedDigitizer = m_pData3DModel->addDigitizerData("Head", "Tracked", t_digSetWithoutAdditional);

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

void HPIWidget::onNewFittingResultAvailable(RTPROCESSINGLIB::FittingResult fitResult)
{
    m_vGof = fitResult.errorDistances;

    storeResults(fitResult.devHeadTrans, fitResult.fittedCoils);
}


//*************************************************************************************************************

void HPIWidget::onBtnDoSingleFit()
{    
    if(!this->hpiLoaded()) {
       QMessageBox msgBox;
       msgBox.setText("Please load a digitizer set with at least 3 HPI coils first!");
       msgBox.exec();
       return;
    }

    if(m_matValue.rows() == 0 || m_matValue.cols() == 0) {
       QMessageBox msgBox;
       msgBox.setText("No data has been received yet! Please start the measurement first!");
       msgBox.exec();
       return;
    }

    //Generate/Update current dev/head transfomration. We do not need to make use of rtHPI plugin here since the fitting is only needed once here.
    //rt head motion correction will be performed using the rtHPI plugin.
    if(m_pFiffInfo) {
        //Perform actual fitting
        FiffDigPointSet fittedCoils;
        FiffCoordTrans devHeadTrans;
        devHeadTrans.from = 1;
        devHeadTrans.to = 4;

        HPIFit::fitHPI(m_matValue,
                        m_matProjectors,
                        devHeadTrans,
                        m_vCoilFreqs,
                        m_vGof,
                        fittedCoils,
                        m_pFiffInfo,
                        true);

        storeResults(devHeadTrans, fittedCoils);
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

void HPIWidget::onDoContinousHPI()
{
    if(!this->hpiLoaded()) {
       QMessageBox msgBox;
       msgBox.setText("Please load a digitizer set with at least 3 HPI coils first!");
       msgBox.exec();
       ui->m_checkBox_continousHPI->setChecked(!ui->m_checkBox_continousHPI->isChecked());
       return;
    }

    if(m_matValue.rows() == 0 || m_matValue.cols() == 0) {
       QMessageBox msgBox;
       msgBox.setText("No data has been received yet! Please start the measurement first!");
       msgBox.exec();
       ui->m_checkBox_continousHPI->setChecked(!ui->m_checkBox_continousHPI->isChecked());
       return;
    }

    emit continousHPIToggled(ui->m_checkBox_continousHPI->isChecked());

    if(ui->m_checkBox_continousHPI->isChecked()) {
        m_pRtHPI->start();
    } else {
        m_pRtHPI->stop();
    }
}


//*************************************************************************************************************

void HPIWidget::onContinousHPIMaxDistChanged()
{
    m_dMaxHPIFitError = ui->m_doubleSpinBox_maxHPIContinousDist->value() * 0.001;
}


//*************************************************************************************************************

void HPIWidget::onSSPCompUsageChanged()
{
    m_bUseSSP = ui->m_checkBox_useSSP->isChecked();
    m_bUseComp = ui->m_checkBox_useComp->isChecked();

    updateProjections();
}


//*************************************************************************************************************

void HPIWidget::updateErrorLabels()
{
    //Update gof labels and transform from m to mm
    QString sGof("0mm");
    if(m_vGof.size() > 0) {
        sGof = QString::number(m_vGof[0]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil1->setText(sGof);
    }

    if(m_vGof.size() > 1) {
        sGof = QString::number(m_vGof[1]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil2->setText(sGof);
    }

    if(m_vGof.size() > 2) {
        sGof = QString::number(m_vGof[2]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil3->setText(sGof);
    }

    if(m_vGof.size() > 3) {
        sGof = QString::number(m_vGof[3]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil4->setText(sGof);
    }

    ui->m_label_averagedFitError->setText(QString::number(m_dMeanErrorDist*1000,'f',2)+QString("mm"));

    //Update good/bad fit label
    if(m_dMeanErrorDist > m_dMaxHPIFitError) {
        ui->m_label_fitFeedback->setText("Bad Fit");
        ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        ui->m_label_fitFeedback->setText("Good Fit");
        ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}


//*************************************************************************************************************

void HPIWidget::updateTransLabels()
{
    //Update labels with new dev/trans matrix
    FiffCoordTrans devHeadTrans = m_pFiffInfo->dev_head_t;

    ui->m_label_mat00->setText(QString::number(devHeadTrans.trans(0,0),'f',4));
    ui->m_label_mat01->setText(QString::number(devHeadTrans.trans(0,1),'f',4));
    ui->m_label_mat02->setText(QString::number(devHeadTrans.trans(0,2),'f',4));
    ui->m_label_mat03->setText(QString::number(devHeadTrans.trans(0,3),'f',4));

    ui->m_label_mat10->setText(QString::number(devHeadTrans.trans(1,0),'f',4));
    ui->m_label_mat11->setText(QString::number(devHeadTrans.trans(1,1),'f',4));
    ui->m_label_mat12->setText(QString::number(devHeadTrans.trans(1,2),'f',4));
    ui->m_label_mat13->setText(QString::number(devHeadTrans.trans(1,3),'f',4));

    ui->m_label_mat20->setText(QString::number(devHeadTrans.trans(2,0),'f',4));
    ui->m_label_mat21->setText(QString::number(devHeadTrans.trans(2,1),'f',4));
    ui->m_label_mat22->setText(QString::number(devHeadTrans.trans(2,2),'f',4));
    ui->m_label_mat23->setText(QString::number(devHeadTrans.trans(2,3),'f',4));

    ui->m_label_mat30->setText(QString::number(devHeadTrans.trans(3,0),'f',4));
    ui->m_label_mat31->setText(QString::number(devHeadTrans.trans(3,1),'f',4));
    ui->m_label_mat32->setText(QString::number(devHeadTrans.trans(3,2),'f',4));
    ui->m_label_mat33->setText(QString::number(devHeadTrans.trans(3,3),'f',4));
}


//*************************************************************************************************************

void HPIWidget::storeResults(const FiffCoordTrans& devHeadTrans, const FiffDigPointSet& fittedCoils)
{
    //Check if git meets distance requirement (GOF)
    if(m_vGof.size() > 0) {
        m_dMeanErrorDist = 0;
        for(int i = 0; i < m_vGof.size(); ++i) {
            m_dMeanErrorDist += m_vGof.at(i);
        }
        m_dMeanErrorDist = m_dMeanErrorDist/m_vGof.size();
    }

    //Update error labels
    updateErrorLabels();

    //If distance is to big, do not store results
    if(m_dMeanErrorDist > m_dMaxHPIFitError) {
        m_bLastFitGood = false;
        return;
    }

    //Update transformation labels
    updateTransLabels();

    m_bLastFitGood = true;

    //If fit was good, set newly calculated transformation matrix to fiff info
    m_pFiffInfo->dev_head_t = devHeadTrans;

    //Add and update items to 3D view
    m_pData3DModel->addDigitizerData("Head", "Fitted", fittedCoils.pickTypes(QList<int>()<<FIFFV_POINT_EEG));

    update3DView();
}


//*************************************************************************************************************

void HPIWidget::update3DView()
{
    qDebug()<< "m_pTrackedDigitizer" <<m_pTrackedDigitizer;
    qDebug()<< "m_pFiffInfo" <<m_pFiffInfo;
    qDebug()<< "m_pBemHeadAdult" <<m_pBemHeadAdult;
    qDebug()<< "m_pBemHeadKid" <<m_pBemHeadKid;

    if(m_pTrackedDigitizer &&
            m_pFiffInfo &&
            m_pBemHeadAdult &&
            m_pBemHeadKid) {
        qDebug()<< "updating 3d view";

        //Prepare new transform
        QMatrix4x4 mat;
        for(int r = 0; r < 3; ++r) {
            for(int c = 0; c < 3; ++c) {
                mat(r,c) = m_pFiffInfo->dev_head_t.trans(r,c);
            }
        }

        Qt3DCore::QTransform pTransform;
        pTransform.setMatrix(mat);

        //Update fast scan / tracked digitizer
        QList<QStandardItem*> itemList = m_pTrackedDigitizer->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(DigitizerTreeItem* pDigItem = dynamic_cast<DigitizerTreeItem*>(itemList.at(j))) {
                pDigItem->setTransform(pTransform);
            }
        }

        //Update adult head surface
        itemList = m_pBemHeadAdult->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                pBemItem->setTransform(pTransform);
            }
        }

        //Update kid's head surface
        itemList = m_pBemHeadKid->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                pBemItem->setTransform(pTransform);

                //If it is the kid's model scale it
                pBemItem->setScale(0.6f);
            }
        }
    }
}


////*************************************************************************************************************

//int HPIWidget::alignFiducials(digitizerData head_dig, digitizerData mri_dig)
///*
// * Align the MEG fiducials to the MRI fiducials
// */
//{
//    float          *head_fid[3],*mri_fid[3],**fid;
//    int            j,k;
//    fiffDigPoint   p;
//    digitizerData  dig;
//    float          nasion_weight = 5.0;
//    float          scales[3];

//    if (!head_dig) {
//        err_set_error("MEG head coordinate system digitizer data not available");
//        goto bad;
//    }

//    if (!mri_dig) {
//        err_set_error("MRI coordinate system digitizer data not available");
//        goto bad;
//    }

//    for (j = 0; j < 2; j++) {
//        dig = j == 0 ? head_dig : mri_dig;
//        fid = j == 0 ? head_fid : mri_fid;

//        for (k = 0; k < 3; k++)
//            fid[k] = NULL;

//            for (k = 0, p = dig->points; k < dig->npoint; k++,p++) {
//                if (p->kind == FIFFV_POINT_CARDINAL) {
//                    if (p->ident == FIFFV_POINT_LPA)
//                        fid[0] = p->r;
//                    else if (p->ident == FIFFV_POINT_NASION)
//                        fid[1] = p->r;
//                    else if (p->ident == FIFFV_POINT_RPA)
//                        fid[2] = p->r;
//                }
//            }
//    }

//    for (k = 0; k < 3; k++) {
//        if (!head_fid[k]) {
//            err_set_error("Some of the MEG fiducials were missing");
//            goto bad;
//        }

//        if (!mri_fid[k]) {
//            err_set_error("Some of the MRI fiducials were missing");
//            goto bad;
//        }
//    }

//    if (scale_head) {
//        get_head_scale(head_dig,mri_fid,head_surf,scales);
//        fprintf(stderr,"xscale = %.3f yscale = %.3f zscale = %.3f\n",scales[0],scales[1],scales[2]);

//        for (j = 0; j < 3; j++)
//            for (k = 0; k < 3; k++)
//                mri_fid[j][k] = mri_fid[j][k]*scales[k];

//            scale_display_surface(head_surf,scales);
//    }
//    /*
//    * Initial alignment
//    */
//    FREE(head_dig->head_mri_t_adj);
//    head_dig->head_mri_t_adj = fiff_make_transform_card(FIFFV_COORD_HEAD,FIFFV_COORD_MRI,
//    mri_fid[0],mri_fid[1],mri_fid[2]);

//    for (k = 0; k < head_dig->nfids; k++)
//        VEC_COPY(head_dig->mri_fids[k].r,mri_fid[k]);

//    mne_print_coord_transform_label(stderr,"After simple alignment : ",head_dig->head_mri_t_adj);

//    if (omit_dist > 0)
//        discard_outlier_digitizer_points(head_dig,head_surf,omit_dist);
//    /*
//    * Optional iterative refinement
//    */
//    if (niter > 0 && head_surf) {
//        for (k = 0; k < niter; k++) {
//            if (iterate_alignment_once(head_dig,head_surf,nasion_weight,mri_fid[1],k == niter-1 && niter > 1) == FAIL)
//                goto bad;
//        }
//        fprintf(stderr,"%d / %d iterations done. RMS dist = %7.1f mm\n",k,niter,
//        1000.0*rms_digitizer_distance(head_dig,head_surf));
//        mne_print_coord_transform_label(stderr,"After refinement : ",head_dig->head_mri_t_adj);
//    }

//    return OK;

//    bad :
//    return FAIL;
//}

