//=============================================================================================================
/**
 * @file     realtime3dwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RealTime3DWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtime3dwidget.h"

#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimehpiresult.h>

#include <connectivity/network/network.h>

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>
#include <disp3D/engine/model/items/sensorspace/sensorsettreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/delegate/data3Dtreedelegate.h>
#include <disp3D/engine/model/items/bem/bemtreeitem.h>
#include <disp3D/engine/model/items/bem/bemsurfacetreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizertreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizersettreeitem.h>

#include <disp/viewers/control3dview.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/c/fiff_digitizer_data.h>

#include <mne/c/mne_msh_display_surface_set.h>
#include <mne/c/mne_surface_or_volume.h>

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace DISP3DLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTime3DWidget::RealTime3DWidget(QWidget* parent)
: MeasurementWidget(parent)
, m_iNumberBadChannels(0)
, m_pData3DModel(Data3DTreeModel::SPtr::create())
, m_p3DView(new View3D())
{
    m_mriHeadTrans = FiffCoordTrans();
    //Init 3D View
    m_p3DView->setModel(m_pData3DModel);

    createGUI();
}

//=============================================================================================================

RealTime3DWidget::~RealTime3DWidget()
{
}

//=============================================================================================================

void RealTime3DWidget::createGUI()
{
    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_p3DView, this, Qt::Widget);
    pWidgetContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pWidgetContainer->setMinimumSize(400,400);

    QGridLayout* pMainLayoutView = new QGridLayout();
    pMainLayoutView->addWidget(pWidgetContainer,0,0);
    pMainLayoutView->setContentsMargins(0,0,0,0);

    this->setLayout(pMainLayoutView);
}

//=============================================================================================================

void RealTime3DWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(!m_bDisplayWidgetsInitialized) {
        initDisplayControllWidgets();
    }

    if(RealTimeConnectivityEstimate::SPtr pRTCE = qSharedPointerDynamicCast<RealTimeConnectivityEstimate>(pMeasurement)) {
        if(pRTCE->getValue().data()->isEmpty()) {
            return;
        }

        // Add rt brain data
        if(!m_pRtConnectivityItem) {
            //qDebug()<<"RealTime3DWidget::getData - Creating m_pRtConnectivityItem";
            m_pRtConnectivityItem = m_pData3DModel->addConnectivityData("Subject",
                                                                        "Functional Data",
                                                                        *(pRTCE->getValue().data()));

            m_pRtConnectivityItem->setThresholds(QVector3D(0.9f,0.95f,1.0f));

            if(pRTCE->getSurfSet() && pRTCE->getAnnotSet()) {
                QList<FsSurfaceTreeItem*> lSurfaces = m_pData3DModel->addSurfaceSet("Subject",
                                                                                    "MRI",
                                                                                    *(pRTCE->getSurfSet().data()),
                                                                                    *(pRTCE->getAnnotSet().data()));

                for(int i = 0; i < lSurfaces.size(); i++) {
                    lSurfaces.at(i)->setAlpha(0.3f);
                }
            }

            if(pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                m_pData3DModel->addMegSensorInfo("Subject",
                                                 "Sensors",
                                                 pRTCE->getFiffInfo()->chs,
                                                 *(pRTCE->getSensorSurface()),
                                                 pRTCE->getFiffInfo()->bads);
                m_iNumberBadChannels = pRTCE->getFiffInfo()->bads.size();
            }
        } else {
            //qDebug()<<"RealTime3DWidget::getData - Working with m_pRtConnectivityItem";
            QPair<float,float> freqs = pRTCE->getValue()->getFrequencyRange();
            QString sItemName = QString("%1_%2_%3").arg(pRTCE->getValue()->getConnectivityMethod()).arg(QString::number(freqs.first)).arg(QString::number(freqs.second));
            m_pRtConnectivityItem->setText(sItemName);
            m_pRtConnectivityItem->addData(*(pRTCE->getValue().data()));

            if(pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                if(m_iNumberBadChannels != pRTCE->getFiffInfo()->bads.size()) {
                    m_pData3DModel->addMegSensorInfo("Subject",
                                                     "Sensors",
                                                     pRTCE->getFiffInfo()->chs,
                                                     *(pRTCE->getSensorSurface()),
                                                     pRTCE->getFiffInfo()->bads);
                    m_iNumberBadChannels = pRTCE->getFiffInfo()->bads.size();
                }
            }
        }
    } else if(RealTimeSourceEstimate::SPtr pRTSE = qSharedPointerDynamicCast<RealTimeSourceEstimate>(pMeasurement)) {
        QList<MNESourceEstimate::SPtr> lMNEData = pRTSE->getValue();

        // Add source estimate data
        if(!lMNEData.isEmpty()) {
            if(!m_pRtMNEItem && pRTSE->getAnnotSet() && pRTSE->getSurfSet() && pRTSE->getFwdSolution()) {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Creating m_lRtItem list";
                m_pRtMNEItem = m_pData3DModel->addSourceData("Subject", "Functional Data",
                                                             *lMNEData.first(),
                                                             *pRTSE->getFwdSolution(),
                                                             *pRTSE->getSurfSet(),
                                                             *pRTSE->getAnnotSet());

                m_pRtMNEItem->setLoopState(false);
                m_pRtMNEItem->setTimeInterval(17);
                m_pRtMNEItem->setThresholds(QVector3D(0.0,5,10));
                m_pRtMNEItem->setColormapType("Hot");
                m_pRtMNEItem->setVisualizationType("Annotation based");
                m_pRtMNEItem->setNumberAverages(1); // Set to 1 because we want to enable time point picking which only includes one sample
                m_pRtMNEItem->setAlpha(1.0);
                m_pRtMNEItem->setStreamingState(true);
                m_pRtMNEItem->setSFreq(pRTSE->getFiffInfo()->sfreq);
                m_mriHeadTrans = pRTSE->getMriHeadTrans();
            } else {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Working with m_lRtItem list";

                if(m_pRtMNEItem) {
                    m_pRtMNEItem->addData(*lMNEData.first());
                }
            }
        }
    } else if(RealTimeHpiResult::SPtr pRTHR = qSharedPointerDynamicCast<RealTimeHpiResult>(pMeasurement)) {
        if(!m_pBemHeadAvr) {
            // Add sensor surface BabyMeg
            QFile t_fileBabyMEGSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/BabyMEG.fif");
            MNEBem t_babyMEGsensorSurfaceBEM(t_fileBabyMEGSensorSurfaceBEM);
            m_pData3DModel->addMegSensorInfo("Device", "BabyMEG", QList<FiffChInfo>(), t_babyMEGsensorSurfaceBEM)->setCheckState(Qt::Unchecked);

            // Add sensor surface VectorView
            QFile t_fileVVSensorSurfaceBEM(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m.fif");
            MNEBem t_sensorVVSurfaceBEM(t_fileVVSensorSurfaceBEM);
            m_pData3DModel->addMegSensorInfo("Device", "VectorView", QList<FiffChInfo>(), t_sensorVVSurfaceBEM);

            // Add average head surface
            QFile t_fileHeadAvr(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif");;
            MNEBem t_BemHeadAvr(t_fileHeadAvr);
            m_pBemHeadAvr = m_pData3DModel->addBemData("Subject", "Average head", t_BemHeadAvr);
        }

        if(QSharedPointer<HpiFitResult> pHpiFitResult = pRTHR->getValue()) {
            if(m_sFilePathDigitizers != pHpiFitResult->sFilePathDigitzers && m_pBemHeadAvr) {
                //Add all digitizer but additional points to the 3D view
                QFile fileDig(pHpiFitResult->sFilePathDigitzers);
                FiffDigPointSet digSet(fileDig);
                addDigSetToView(digSet);
                alignFiducials(pHpiFitResult->sFilePathDigitzers);
            }
            if (!m_pFiffDigitizerData && m_pBemHeadAvr){
                if (auto pDigData = pRTHR->digitizerData()){
                    FiffDigPointSet digSet(pDigData->points);
                    addDigSetToView(digSet);
                    alignFiducials(pDigData);
                }
            }

            //Add and update items to 3D view
            m_pData3DModel->addDigitizerData("Subject",
                                             "Fitted Digitizers",
                                             pHpiFitResult->fittedCoils.pickTypes(QList<int>()<<FIFFV_POINT_EEG));

            if(m_pTrackedDigitizer && m_pBemHeadAvr) {
                //Update fast scan / tracked digitizer
                QList<QStandardItem*> itemList = m_pTrackedDigitizer->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
                for(int j = 0; j < itemList.size(); ++j) {
                    if(DigitizerTreeItem* pDigItem = dynamic_cast<DigitizerTreeItem*>(itemList.at(j))) {
                        // apply inverse to get from head to device space
                        pDigItem->setTransform(pHpiFitResult->devHeadTrans, true);
                    }
                }

                // Update average head
                itemList = m_pBemHeadAvr->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
                for(int j = 0; j < itemList.size(); ++j) {
                    if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
                        pBemItem->setTransform(m_tAlignment);
                        // apply inverse to get from head to device space
                        pBemItem->applyTransform(pHpiFitResult->devHeadTrans, true);
                    }
                }
            }

            if(m_pRtMNEItem) {
                if(!m_mriHeadTrans.isEmpty()) {
                    m_pRtMNEItem->setTransform(m_mriHeadTrans,false);
                    m_pRtMNEItem->applyTransform(pHpiFitResult->devHeadTrans, true);
                } else {
                    m_pRtMNEItem->setTransform(pHpiFitResult->devHeadTrans, true);
                }

            }

            if(m_pRtConnectivityItem) {
                m_pRtConnectivityItem->setTransform(pHpiFitResult->devHeadTrans, true);
            }
        }
    }
}

//=============================================================================================================

void RealTime3DWidget::addDigSetToView(const FIFFLIB::FiffDigPointSet& digSet)
{
    FiffDigPointSet digSetWithoutAdditional = digSet.pickTypes(QList<int>()<<FIFFV_POINT_HPI<<FIFFV_POINT_CARDINAL<<FIFFV_POINT_EEG<<FIFFV_POINT_EXTRA);
    m_pTrackedDigitizer = m_pData3DModel->addDigitizerData("Subject",
                                                           "Tracked Digitizers",
                                                           digSetWithoutAdditional);
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(const QString& sFilePath)
{
    m_sFilePathDigitizers = sFilePath;

    QFile t_fileDigData(sFilePath);
    FiffDigitizerData* t_digData = new FiffDigitizerData(t_fileDigData);

    alignFiducials(t_digData);
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(QSharedPointer<FIFFLIB::FiffDigitizerData> pDigData)
{
    m_pFiffDigitizerData = pDigData;

    alignFiducials(m_pFiffDigitizerData.data());
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(FIFFLIB::FiffDigitizerData *pDigData)
{
    std::unique_ptr<MneMshDisplaySurfaceSet> pMneMshDisplaySurfaceSet = std::make_unique<MneMshDisplaySurfaceSet>();
    MneMshDisplaySurfaceSet::add_bem_surface(pMneMshDisplaySurfaceSet.get(),
                                             QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif",
                                             FIFFV_BEM_SURF_ID_HEAD,
                                             "head",
                                             1,
                                             1);

    MneMshDisplaySurface* surface = pMneMshDisplaySurfaceSet->surfs[0];

    QFile t_fileDigDataReference(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-fiducials.fif");

    float scales[3];
    QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData(t_fileDigDataReference));
    MneSurfaceOrVolume::align_fiducials(pDigData,
                                        t_digDataReference.data(),
                                        surface,
                                        10,
                                        1,
                                        0,
                                        scales);

    QMatrix4x4 invMat = calculateInverseMatrix(pDigData, scales[0]);

    applyAlignmentTransform(invMat);
}

//=============================================================================================================

void RealTime3DWidget::applyAlignmentTransform(QMatrix4x4 invMat)
{
    Qt3DCore::QTransform identity;
    m_tAlignment.setMatrix(invMat);

    // align and scale average head (now in head space)
    QList<QStandardItem*> itemList = m_pBemHeadAvr->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
    for(int j = 0; j < itemList.size(); ++j) {
        if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
            pBemItem->setTransform(m_tAlignment);
        }
    }
}

//=============================================================================================================

QMatrix4x4 RealTime3DWidget::calculateInverseMatrix(FIFFLIB::FiffDigitizerData *pDigData,
                                                    float scale)
{
    QMatrix4x4 invMat;

    // use inverse transform
    for(int r = 0; r < 3; ++r) {
        for(int c = 0; c < 3; ++c) {
            // also apply scaling factor
            invMat(r,c) = pDigData->head_mri_t_adj->invrot(r,c) * scale;
        }
    }
    invMat(0,3) = pDigData->head_mri_t_adj->invmove(0);
    invMat(1,3) = pDigData->head_mri_t_adj->invmove(1);
    invMat(2,3) = pDigData->head_mri_t_adj->invmove(2);

    return invMat;
}

//=============================================================================================================

void RealTime3DWidget::initDisplayControllWidgets()
{
    Data3DTreeDelegate* pData3DTreeDelegate = new Data3DTreeDelegate(this);

    //Init control widgets
    QList<QWidget*> lControlWidgets;

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    Control3DView* pControl3DView = new Control3DView("MNESCAN/RT3DW", Q_NULLPTR, slControlFlags);
    pControl3DView->setObjectName("group_tab_View_General");
    lControlWidgets.append(pControl3DView);

    pControl3DView->setDelegate(pData3DTreeDelegate);
    pControl3DView->setModel(m_pData3DModel.data());

    connect(pControl3DView, &Control3DView::sceneColorChanged,
            m_p3DView.data(), &View3D::setSceneColor);

    connect(pControl3DView, &Control3DView::rotationChanged,
            m_p3DView.data(), &View3D::startStopCameraRotation);

    connect(pControl3DView, &Control3DView::showCoordAxis,
            m_p3DView.data(), &View3D::toggleCoordAxis);

    connect(pControl3DView, &Control3DView::showFullScreen,
            m_p3DView.data(), &View3D::showFullScreen);

    connect(pControl3DView, &Control3DView::lightColorChanged,
            m_p3DView.data(), &View3D::setLightColor);

    connect(pControl3DView, &Control3DView::lightIntensityChanged,
            m_p3DView.data(), &View3D::setLightIntensity);

    connect(pControl3DView, &Control3DView::takeScreenshotChanged,
            m_p3DView.data(), &View3D::takeScreenshot);

    emit displayControlWidgetsChanged(lControlWidgets, "3D View");

    m_bDisplayWidgetsInitialized = true;
}
