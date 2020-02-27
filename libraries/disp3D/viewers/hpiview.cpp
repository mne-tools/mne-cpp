//=============================================================================================================
/**
 * @file     hpiview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
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

    //Add sensor surface
    QFile t_fileBabyMEGSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/BabyMEG.fif");
    MNEBem t_babyMEGsensorSurfaceBEM(t_fileBabyMEGSensorSurfaceBEM);
    m_pData3DModel->addMegSensorInfo("Device", "BabyMEG", QList<FiffChInfo>(), t_babyMEGsensorSurfaceBEM);

    QFile t_fileVVSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m.fif");
    MNEBem t_sensorVVSurfaceBEM(t_fileVVSensorSurfaceBEM);
    BemTreeItem* pVVItem = m_pData3DModel->addBemData("Device", "VectorView", t_sensorVVSurfaceBEM);
    pVVItem->setCheckState(Qt::Unchecked);

    QFile t_fileHeadKid(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_BemHeadKid(t_fileHeadKid);
    m_pBemHeadKid = m_pData3DModel->addBemData("Head", "Child", t_BemHeadKid);
    m_pBemHeadKid->setCheckState(Qt::Unchecked);

    QFile t_fileHeadAdult(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_BemHeadAdult(t_fileHeadAdult);
    m_pBemHeadAdult = m_pData3DModel->addBemData("Head", "Adult", t_BemHeadAdult);
    m_pBemHeadAdult->setCheckState(Qt::Unchecked);

    QFile t_fileHeadAvr(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif");;
    MNEBem t_BemHeadAvr(t_fileHeadAvr);
    m_pBemHeadAvr = m_pData3DModel->addBemData("Head", "Average", t_BemHeadAvr);
    m_pBemHeadAvr->setCheckState(Qt::Unchecked);

    //Always on top
    //this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

    //Init coil freqs
    m_vCoilFreqs << 155 << 165 << 190 << 220;

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

QVector<double> HpiView::getGOF()
{
    return m_vGof;
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

    QFile file (QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-trans.fif");
    FiffCoordTrans transAvr(file);

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

    // read fsaverage fiducials
    QFile t_fileDigAvr(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-fiducials.fif");
    FiffDigPointSet t_digAvr(t_fileDigAvr);
    //t_digAvr.applyTransform(transAvr);
    DigitizerSetTreeItem* pDigItem = m_pData3DModel->addDigitizerData("Head", "avr", t_digAvr);

    Eigen::MatrixXf sLm(3,3);
    Eigen::MatrixXf dLm(3,3);
//    QFile t_fileSample(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
//    FiffDigPointSet t_digAvr(t_fileSample);

    for(int i = 0; i < t_digAvr.size(); i++){
        if(t_digAvr[i].kind == FIFFV_POINT_CARDINAL/* || t_digAvr[i].kind ==  FIFFV_POINT_HPI*/){
        sLm(i,0) = t_digAvr[i].r[0];
        sLm(i,1) = t_digAvr[i].r[1];
        sLm(i,2) = t_digAvr[i].r[2];
        }
    }

    for(int i = 0; i < t_digSet.size(); i++){
        if(t_digSet[i].kind == FIFFV_POINT_CARDINAL/* || t_digSet[i].kind ==  FIFFV_POINT_HPI*/) {
            dLm(i,0) = t_digSet[i].r[0];
            dLm(i,1) = t_digSet[i].r[1];
            dLm(i,2) = t_digSet[i].r[2];
        }
    }
    qDebug() << "sLm";
    std::cout << sLm << std::endl;
    qDebug() << "dLm";
    std::cout << dLm << std::endl;

    Matrix4f trans = computeTransformation(dLm, sLm);
    qDebug() << "trans";
    std::cout << trans << std::endl;
    MatrixXf temp = sLm;
    temp.conservativeResize(sLm.rows(),sLm.cols()+1);
    temp.block(0,3,3,1).setOnes();
    temp.transposeInPlace();
    MatrixXf tSLm = trans * temp;
    sLm = tSLm.block(0,0,3,3);

    qDebug() << "tsLm";
    std::cout << sLm << std::endl;

//    RowVector3f vScale(3);

//    vScale(0) = dLm.row(0).norm() / sLm.row(0).norm();
//    vScale(1) = dLm.row(1).norm() / sLm.row(1).norm();
//    vScale(2) = dLm.row(2).norm() / sLm.row(2).norm();

//    float scale = vScale.mean();

//    qDebug() << "vScale: " ;
//    std::cout << vScale << std::endl;
//    qDebug() << "Scale: " << scale;

    Qt3DCore::QTransform transform;
    QMatrix4x4 mat;
    for(int r = 0; r < 4; ++r) {
        for(int c = 0; c < 4; ++c) {
            mat(r,c) = trans(r,c);
        }
    }
    transform.setMatrix(mat);

    pDigItem->setTransform(transform);

    //Update Average head surface
    QList<QStandardItem*> itemList = m_pBemHeadAvr->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
    for(int j = 0; j < itemList.size(); ++j) {
        if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
            //If it is the kid's model scale it
            pBemItem->setTransform(transAvr);
            pBemItem->setTransform(transform);
            //pBemItem->setScale(scale);
        }
    }

    return lDigPoints;
}

//=============================================================================================================

void HpiView::alignFiducials(const QString& fileNameDigData)
{
    QFile test(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-fiducials.fif");
    FiffDigPointSet testdata(test);
    m_pData3DModel->addDigitizerData("Head", "avr", testdata);

    //Calculate the alignment of the fiducials
    MneMshDisplaySurfaceSet* pMneMshDisplaySurfaceSet = new MneMshDisplaySurfaceSet();
    QFile t_fileHeadAvr (QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif");
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

    //FiffDigitizerData* t_digDataReference = new FiffDigitizerData(t_fileDigDataReference);
    QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData(t_fileDigDataReference));
    MneSurfaceOrVolume::align_fiducials(t_digData,
                                        t_digDataReference.data(),
                                        surface,
                                        10,
                                        1,
                                        0);

    QMatrix4x4 mat;
    for(int r = 0; r < 3; ++r) {
        for(int c = 0; c < 3; ++c) {
            mat(r,c) = t_digData->head_mri_t_adj->rot(r,c);
        }
    }
    mat(0,3) = t_digData->head_mri_t_adj->move(0);
    mat(1,3) = t_digData->head_mri_t_adj->move(1);
    mat(2,3) = t_digData->head_mri_t_adj->move(2);

    Qt3DCore::QTransform transform;
    transform.setMatrix(mat);

    //Update fast scan / tracked digitizer
    QList<QStandardItem*> itemList = m_pTrackedDigitizer->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
    for(int j = 0; j < itemList.size(); ++j) {
        if(DigitizerTreeItem* pDigItem = dynamic_cast<DigitizerTreeItem*>(itemList.at(j))) {
            pDigItem->setTransform(transform);
        }
    }

    std::cout<<"rot:"<<std::endl<<t_digData->head_mri_t_adj->rot;
    std::cout<<std::endl<<"move:"<<std::endl<<t_digData->head_mri_t_adj->move;

}

//=============================================================================================================

void HpiView::onNewFittingResultAvailable(const RTPROCESSINGLIB::FittingResult& fitResult)
{
    m_vGof = fitResult.errorDistances;

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
            tr("Open digitizer file"),QCoreApplication::applicationDirPath() + "MNE-sample-data/chpi/raw" , tr("Fiff file (*.fif)"));

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
    if(m_pTrackedDigitizer && m_pFiffInfo && m_pBemHeadAdult && m_pBemHeadKid) {
        //Prepare new transform
        QMatrix4x4 mat;
        for(int r = 0; r < 4; ++r) {
            for(int c = 0; c < 4; ++c) {
                mat(r,c) = m_pFiffInfo->dev_head_t.invtrans(r,c);
            }
        }

        Qt3DCore::QTransform transform;
        transform.setMatrix(mat);

        //Update fast scan / tracked digitizer
        QList<QStandardItem*> itemList = m_pTrackedDigitizer->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(DigitizerTreeItem* pDigItem = dynamic_cast<DigitizerTreeItem*>(itemList.at(j))) {
                pDigItem->setTransform(transform);
            }
        }

        //Update adult head surface
        itemList = m_pBemHeadAdult->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                pBemItem->setTransform( transform);
            }
        }

        //Update kid's head surface
        itemList = m_pBemHeadKid->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
        for(int j = 0; j < itemList.size(); ++j) {
            if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                //If it is the kid's model scale it
                pBemItem->setTransform(transform);
                pBemItem->setScale(0.65f);
            }
        }
    }
}

//*************************************************************************************************************

Eigen::Matrix4f HpiView::computeTransformation(Eigen::MatrixXf NH, MatrixXf BT)
{
    MatrixXf xdiff, ydiff, zdiff, C, Q;
    Matrix4f transFinal = Matrix4f::Identity(4,4);
    Matrix4f Rot = Matrix4f::Zero(4,4);
    Matrix4f Trans = Matrix4f::Identity(4,4);
    double meanx,meany,meanz,normf;

    for(int i = 0; i < 15; ++i) {
        // Calculate mean translation for all points -> centroid of both data sets
        xdiff = NH.col(0) - BT.col(0);
        ydiff = NH.col(1) - BT.col(1);
        zdiff = NH.col(2) - BT.col(2);

        meanx = xdiff.mean();
        meany = ydiff.mean();
        meanz = zdiff.mean();

        // Apply translation -> bring both data sets to the same center location
        for (int j = 0; j < BT.rows(); ++j) {
            BT(j,0) = BT(j,0) + meanx;
            BT(j,1) = BT(j,1) + meany;
            BT(j,2) = BT(j,2) + meanz;
        }

        // Estimate rotation component
        C = BT.transpose() * NH;

        JacobiSVD< MatrixXf > svd(C ,Eigen::ComputeThinU | ComputeThinV);

        Q = svd.matrixU() * svd.matrixV().transpose();

        //Handle special reflection case
        if(Q.determinant() < 0) {
            Q(0,2) = Q(0,2) * -1;
            Q(1,2) = Q(1,2) * -1;
            Q(2,2) = Q(2,2) * -1;
        }

        // Apply rotation on translated points
        BT = BT * Q;

        // Calculate GOF
        normf = (NH.transpose()-BT.transpose()).norm();

        // Store rotation part to transformation matrix
        Rot(3,3) = 1;
        for(int j = 0; j < 3; ++j) {
            for(int k = 0; k < 3; ++k) {
                Rot(j,k) = Q(k,j);
            }
        }

        // Store translation part to transformation matrix
        Trans(0,3) = meanx;
        Trans(1,3) = meany;
        Trans(2,3) = meanz;

        // Safe rotation and translation to final matrix for next iteration step
        // This step is safe to do since we change one of the input point sets (BT)
        // ToDo: Replace this for loop with a least square solution process
        transFinal = Rot * Trans * transFinal;
    }
    return transFinal;
}
