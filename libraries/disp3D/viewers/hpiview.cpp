//=============================================================================================================
/**
 * @file     hpiview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    HpiView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpiview.h"
#include "ui_hpiview.h"

#include "../engine/view/view3D.h"
#include "../engine/model/data3Dtreemodel.h"
#include "../engine/model/items/bem/bemtreeitem.h"
#include "../engine/model/items/bem/bemsurfacetreeitem.h"
#include "../engine/model/items/digitizer/digitizersettreeitem.h"
#include "../engine/model/items/digitizer/digitizertreeitem.h"
#include "../engine/model/3dhelpers/renderable3Dentity.h"

#include <fiff/fiff_dig_point_set.h>
#include <fiff/c/fiff_digitizer_data.h>

#include <inverse/hpiFit/hpifit.h>

#include <fwd/fwd_bem_model.h>

#include <mne/c/mne_msh_display_surface_set.h>
#include <mne/c/mne_msh_display_surface.h>
#include <mne/c/mne_surface_or_volume.h>
#include <mne/mne_bem.h>

#include <utils/sphere.h>

#include <disp/viewers/control3dview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <Qt3DCore/QTransform>
#include <QScopedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================
#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace DISP3DLIB;
using namespace DISPLIB;
using namespace MNELIB;
using namespace RTPROCESSINGLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HpiView::HpiView(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                 QWidget *parent,
                 Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::HpiViewWidget)
, m_pFiffInfo(pFiffInfo)
, m_pView3D(View3D::SPtr(new View3D))
, m_pData3DModel(Data3DTreeModel::SPtr(new Data3DTreeModel))
, m_pRtHPI(RtHpi::SPtr(new RtHpi(m_pFiffInfo)))
, m_dMaxHpiFitError(0.01)
, m_dMeanErrorDist(0.0)
, m_iNubmerBadChannels(0)
, m_bUseSSP(false)
, m_bUseComp(true)
, m_bLastFitGood(false)
{
    ui->setupUi(this);

    //Do GUI connects
    connect(ui->m_pushButton_doSingleFit, &QPushButton::released,
            this, &HpiView::onBtnDoSingleFit);

    connect(ui->m_pushButton_loadDigitizers, &QPushButton::released,
            this, &HpiView::onBtnLoadPolhemusFile);

    connect(ui->m_spinBox_freqCoil1, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HpiView::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil2, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HpiView::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil3, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HpiView::onFreqsChanged);
    connect(ui->m_spinBox_freqCoil4, static_cast<void(QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged),
            this, &HpiView::onFreqsChanged);

    connect(ui->m_checkBox_continousHPI, &QCheckBox::clicked,
            this, &HpiView::onDoContinousHPI);

    connect(ui->m_doubleSpinBox_maxHPIContinousDist, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &HpiView::onContinousHPIMaxDistChanged);

    connect(ui->m_checkBox_useSSP, &QCheckBox::clicked,
            this, &HpiView::onSSPCompUsageChanged);
    connect(ui->m_checkBox_useComp, &QCheckBox::clicked,
            this, &HpiView::onSSPCompUsageChanged);

    //Init from default values
    ui->m_checkBox_useSSP->setChecked(m_bUseSSP);
    ui->m_checkBox_useComp->setChecked(m_bUseComp);

    //Setup View3D
    m_pView3D->setModel(m_pData3DModel);

    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_pView3D.data());
    ui->m_gridLayout_main->addWidget(pWidgetContainer,0,0,5,1);

    QStringList slFlag = QStringList() << "Data";

    Control3DView* control3DWidget = new Control3DView(this, slFlag);
    control3DWidget->setModel(m_pData3DModel.data());

    connect(control3DWidget, &Control3DView::sceneColorChanged,
            m_pView3D.data(), &View3D::setSceneColor);

    connect(control3DWidget, &Control3DView::rotationChanged,
            m_pView3D.data(), &View3D::startStopModelRotation);

    connect(control3DWidget, &Control3DView::showCoordAxis,
            m_pView3D.data(), &View3D::toggleCoordAxis);

    connect(control3DWidget, &Control3DView::showFullScreen,
            m_pView3D.data(), &View3D::showFullScreen);

    connect(control3DWidget, &Control3DView::lightColorChanged,
            m_pView3D.data(), &View3D::setLightColor);

    connect(control3DWidget, &Control3DView::lightIntensityChanged,
            m_pView3D.data(), &View3D::setLightIntensity);

    connect(control3DWidget, &Control3DView::takeScreenshotChanged,
            m_pView3D.data(), &View3D::takeScreenshot);

    control3DWidget->onTreeViewDescriptionHide();

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(control3DWidget);
    ui->m_groupBox_3dControl->setLayout(gridLayout);

    // Add sensor surface BabyMeg
    QFile t_fileBabyMEGSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/BabyMEG.fif");
    MNEBem t_babyMEGsensorSurfaceBEM(t_fileBabyMEGSensorSurfaceBEM);
    m_pData3DModel->addMegSensorInfo("Device", "BabyMEG", QList<FiffChInfo>(), t_babyMEGsensorSurfaceBEM);

    // Add sensor surface VectorView
    QFile t_fileVVSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m.fif");
    MNEBem t_sensorVVSurfaceBEM(t_fileVVSensorSurfaceBEM);
    BemTreeItem* pVVItem = m_pData3DModel->addBemData("Device", "VectorView", t_sensorVVSurfaceBEM);
    pVVItem->setCheckState(Qt::Unchecked);

    // Add average head surface
    QFile t_fileHeadAvr(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif");;
    MNEBem t_BemHeadAvr(t_fileHeadAvr);
    m_pBemHeadAvr = m_pData3DModel->addBemData("Head", "Average", t_BemHeadAvr);
    m_pBemHeadAvr->setCheckState(Qt::Unchecked);

    //Always on top
    //this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

    //Init coil freqs
    m_vCoilFreqs << 155 << 165 << 190 << 200;

    //Init data
    m_matValue.resize(0,0);

    //Init RtHPIs
    m_pRtHPI->setCoilFrequencies(m_vCoilFreqs);
    connect(m_pRtHPI.data(), &RtHpi::newFittingResultAvailable,
            this, &HpiView::onNewFittingResultAvailable);
}

//=============================================================================================================

HpiView::~HpiView()
{
    delete ui;
}

//=============================================================================================================

void HpiView::setData(const Eigen::MatrixXd& matData)
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

//=============================================================================================================

QVector<double> HpiView::getError()
{
    return m_vError;
}

//=============================================================================================================

Eigen::VectorXd HpiView::getGoF()
{
    return m_vGoF;
}

//=============================================================================================================

bool HpiView::wasLastFitOk()
{
    return m_bLastFitGood;
}

//=============================================================================================================

void HpiView::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
}

//=============================================================================================================

void HpiView::updateProjections()
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
        //Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
        if(m_pFiffInfo->make_compensator(0, 101, newComp)) {
            matComp = newComp.data->data;
        }
    }

    m_matCompProjectors = m_matProjectors * matComp;

    m_pRtHPI->setProjectionMatrix(m_matProjectors);
}

//=============================================================================================================

bool HpiView::hpiLoaded()
{
    if(ui->m_label_numberLoadedCoils->text().toInt() >= 3) {
        return true;
    }

    return false;
}

//=============================================================================================================

QList<FiffDigPoint> HpiView::readPolhemusDig(const QString& fileName)
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
    //m_pData3DModel->addDigitizerData("Head", "Tracked", t_digSetWithoutAdditional);

    //Set loaded number of digitizers
    ui->m_label_numberLoadedCoils->setNum(numHPI);
    ui->m_label_numberLoadedDigitizers->setNum(numDig);
    ui->m_label_numberLoadedFiducials->setNum(numFiducials);
    ui->m_label_numberLoadedEEG->setNum(numEEG);

    //Hide/show frequencies and errors based on the number of coils
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
        m_vCoilFreqs << 155 << 165 << 190 << 200;
    }

    // align fiducials and scale average head
    alignFiducials(fileName);

    return lDigPoints;
}

//=============================================================================================================

void HpiView::alignFiducials(const QString& fileNameDigData)
{
    //Calculate the alignment of the fiducials
    MneMshDisplaySurfaceSet* pMneMshDisplaySurfaceSet = new MneMshDisplaySurfaceSet();
    MneMshDisplaySurfaceSet::add_bem_surface(pMneMshDisplaySurfaceSet,
                                             QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif",
                                             FIFFV_BEM_SURF_ID_HEAD,
                                             "head",
                                             1,
                                             1);

    MneMshDisplaySurface* surface = pMneMshDisplaySurfaceSet->surfs[0];

    // fid from .fif file with digitizers
    QFile t_fileDigData(fileNameDigData);
    FiffDigitizerData* t_digData = new FiffDigitizerData(t_fileDigData);

    QFile t_fileDigDataReference(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-fiducials.fif");

    float scales[3];
    //FiffDigitizerData* t_digDataReference = new FiffDigitizerData(t_fileDigDataReference);
    QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData(t_fileDigDataReference));
    MneSurfaceOrVolume::align_fiducials(t_digData,
                                        t_digDataReference.data(),
                                        surface,
                                        10,
                                        1,
                                        0,
                                        scales);

    QMatrix4x4 invMat;

    // use inverse transform
    for(int r = 0; r < 3; ++r) {
        for(int c = 0; c < 3; ++c) {
            // also apply scaling factor
            invMat(r,c) = t_digData->head_mri_t_adj->invrot(r,c) * scales[0];
        }
    }
    invMat(0,3) = t_digData->head_mri_t_adj->invmove(0);
    invMat(1,3) = t_digData->head_mri_t_adj->invmove(1);
    invMat(2,3) = t_digData->head_mri_t_adj->invmove(2);

    Qt3DCore::QTransform identity;
    m_tAlignment.setMatrix(invMat);

    // align and scale average head (now in head space)
    QList<QStandardItem*> itemList = m_pBemHeadAvr->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
    for(int j = 0; j < itemList.size(); ++j) {
        if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
            pBemItem->setTransform(m_tAlignment);
        }
    }

    delete pMneMshDisplaySurfaceSet;
}

//=============================================================================================================

void HpiView::onNewFittingResultAvailable(const RTPROCESSINGLIB::FittingResult& fitResult)
{
    m_vError = fitResult.errorDistances;
    m_vGoF = fitResult.GoF;
    storeResults(fitResult.devHeadTrans, fitResult.fittedCoils);
}

//=============================================================================================================

void HpiView::onBtnDoSingleFit()
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
        m_pRtHPI->append(m_matValue);
    }
}

//=============================================================================================================

void HpiView::onBtnLoadPolhemusFile()
{
    //Get file location
    QString fileName_HPI = QFileDialog::getOpenFileName(this,
            tr("Open digitizer file"),"", tr("Fiff file (*.fif)"));

    if(!fileName_HPI.isEmpty()) {
        ui->m_lineEdit_filePath->setText(fileName_HPI);
    }

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

//=============================================================================================================

void HpiView::onFreqsChanged()
{
    m_vCoilFreqs.clear();
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil1->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil2->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil3->value());
    m_vCoilFreqs.append(ui->m_spinBox_freqCoil4->value());

    m_pRtHPI->setCoilFrequencies(m_vCoilFreqs);
}

//=============================================================================================================

void HpiView::onDoContinousHPI()
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
}

//=============================================================================================================

void HpiView::onContinousHPIMaxDistChanged()
{
    m_dMaxHpiFitError = ui->m_doubleSpinBox_maxHPIContinousDist->value() * 0.001;
}

//=============================================================================================================

void HpiView::onSSPCompUsageChanged()
{
    m_bUseSSP = ui->m_checkBox_useSSP->isChecked();
    m_bUseComp = ui->m_checkBox_useComp->isChecked();

    updateProjections();
}

//=============================================================================================================

void HpiView::updateErrorLabels()
{
    //Update gof labels and m_tAlignment from m to mm
    QString sGof("0mm");
    if(m_vError.size() > 0) {
        sGof = QString::number(m_vError[0]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil1->setText(sGof);
    }

    if(m_vError.size() > 1) {
        sGof = QString::number(m_vError[1]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil2->setText(sGof);
    }

    if(m_vError.size() > 2) {
        sGof = QString::number(m_vError[2]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil3->setText(sGof);
    }

    if(m_vError.size() > 3) {
        sGof = QString::number(m_vError[3]*1000,'f',2)+QString("mm");
        ui->m_label_gofCoil4->setText(sGof);
    }

    ui->m_label_averagedFitError->setText(QString::number(m_dMeanErrorDist*1000,'f',2)+QString("mm"));

    //Update good/bad fit label
    if(m_dMeanErrorDist > m_dMaxHpiFitError) {
        ui->m_label_fitFeedback->setText("Bad Fit");
        ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : red;}");
    } else {
        ui->m_label_fitFeedback->setText("Good Fit");
        ui->m_label_fitFeedback->setStyleSheet("QLabel { background-color : green;}");
    }
}

//=============================================================================================================

void HpiView::updateTransLabels()
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

//=============================================================================================================

void HpiView::storeResults(const FiffCoordTrans& devHeadTrans, const FiffDigPointSet& fittedCoils)
{
    //Check if git meets distance requirement (GOF)
    if(m_vError.size() > 0) {
        m_dMeanErrorDist = 0;        
        m_dMeanErrorDist = std::accumulate(m_vError.begin(), m_vError.end(), .0) / m_vError.size();
    }

    //Update error labels
    updateErrorLabels();

    //If distance is to big, do not store results
    if(m_dMeanErrorDist > m_dMaxHpiFitError) {
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

//=============================================================================================================

void HpiView::update3DView()
{
    if(m_pTrackedDigitizer && m_pFiffInfo && m_pBemHeadAvr) {

        //Update fast scan / tracked digitizer
        QList<QStandardItem*> itemList = m_pTrackedDigitizer->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(DigitizerTreeItem* pDigItem = dynamic_cast<DigitizerTreeItem*>(itemList.at(j))) {
                // apply inverse to get from head to device space
                pDigItem->setTransform(m_pFiffInfo->dev_head_t,true);
            }
        }

        // Update average head
        itemList = m_pBemHeadAvr->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                pBemItem->setTransform(m_tAlignment);
                // apply inverse to get from head to device space
                pBemItem->applyTransform(m_pFiffInfo->dev_head_t,true);
            }
        }
    }
}
