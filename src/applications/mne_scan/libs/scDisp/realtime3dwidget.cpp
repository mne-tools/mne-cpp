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

#include <mne/mne_bem.h>
#include <disp3D_rhi/view/brainview.h>
#include <disp3D_rhi/model/braintreemodel.h>
#include <disp3D_rhi/model/items/networktreeitem.h>
#include <disp3D_rhi/model/items/sensortreeitem.h>
#include <disp3D_rhi/model/items/surfacetreeitem.h>
#include <disp3D_rhi/model/items/bemtreeitem.h>
#include <disp3D_rhi/model/items/digitizersettreeitem.h>

#include <disp/viewers/control3dview.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_digitizer_data.h>

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
, m_pData3DModel(QSharedPointer<BrainTreeModel>::create())
, m_p3DView(new BrainView())
, m_bRtSourceActive(false)
{
    m_mriHeadTrans = FiffCoordTrans();
    //Init 3D View
    m_p3DView->setModel(m_pData3DModel.data());

    createGUI();
}

//=============================================================================================================

RealTime3DWidget::~RealTime3DWidget()
{
}

//=============================================================================================================

void RealTime3DWidget::createGUI()
{
    m_p3DView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_p3DView->setMinimumSize(400,400);

    QGridLayout* pMainLayoutView = new QGridLayout();
    pMainLayoutView->addWidget(m_p3DView,0,0);
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

        // Add rt connectivity data
        if(!m_pRtConnectivityItem) {
            m_pRtConnectivityItem = m_pData3DModel->addNetwork(*(pRTCE->getValue().data()),
                                                               "Functional Data");

            if(pRTCE->getSurfSet() && pRTCE->getAnnotSet()) {
                for(int i = 0; i < pRTCE->getSurfSet()->size(); ++i) {
                    const FSLIB::Surface &surf = (*pRTCE->getSurfSet())[i];
                    QString hemi = (surf.hemi() == 0) ? "lh" : "rh";
                    QString surfType = surf.surf().isEmpty() ? "inflated" : surf.surf();
                    m_pData3DModel->addSurface("Subject", hemi, surfType, surf);
                    m_pData3DModel->addAnnotation("Subject", hemi, (*pRTCE->getAnnotSet())[i]);
                }
            }

            // Note: Sensor surface visualization from FiffChInfo/MNEBem is not
            // directly supported in disp3D_rhi's addSensors API. Use loadSensors()
            // with a .fif file path if sensor visualization is needed.
            m_iNumberBadChannels = pRTCE->getFiffInfo() ? pRTCE->getFiffInfo()->bads.size() : 0;
        } else {
            // Update existing connectivity data - replace with new network
            QPair<float,float> freqs = pRTCE->getValue()->getFrequencyRange();
            QString sItemName = QString("%1_%2_%3").arg(pRTCE->getValue()->getConnectivityMethod(), QString::number(freqs.first), QString::number(freqs.second));
            m_pRtConnectivityItem->setText(sItemName);

            if(pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                if(m_iNumberBadChannels != pRTCE->getFiffInfo()->bads.size()) {
                    // Note: Sensor update from in-memory FiffChInfo/MNEBem not
                    // directly supported in disp3D_rhi.
                    m_iNumberBadChannels = pRTCE->getFiffInfo()->bads.size();
                }
            }
        }
    } else if(RealTimeSourceEstimate::SPtr pRTSE = qSharedPointerDynamicCast<RealTimeSourceEstimate>(pMeasurement)) {
        QList<MNESourceEstimate::SPtr> lMNEData = pRTSE->getValue();

        // Add source estimate data via BrainView's realtime streaming
        if(!lMNEData.isEmpty()) {
            if(!m_bRtSourceActive && pRTSE->getAnnotSet() && pRTSE->getSurfSet() && pRTSE->getFwdSolution()) {
                // Add surfaces first
                for(int i = 0; i < pRTSE->getSurfSet()->size(); ++i) {
                    const FSLIB::Surface &surf = (*pRTSE->getSurfSet())[i];
                    QString hemi = (surf.hemi() == 0) ? "lh" : "rh";
                    QString surfType = surf.surf().isEmpty() ? "inflated" : surf.surf();
                    m_pData3DModel->addSurface("Subject", hemi, surfType, surf);
                    m_pData3DModel->addAnnotation("Subject", hemi, (*pRTSE->getAnnotSet())[i]);
                }
                // Configure and start realtime streaming
                m_p3DView->setSourceColormap("Hot");
                m_p3DView->startRealtimeStreaming();
                m_bRtSourceActive = true;
                m_mriHeadTrans = pRTSE->getMriHeadTrans();
            } else {
                if(m_bRtSourceActive) {
                    // Extract data column from MNESourceEstimate
                    const MNESourceEstimate &stc = *lMNEData.first();
                    if(stc.data.cols() > 0) {
                        m_p3DView->pushRealtimeSourceData(stc.data.col(0));
                    }
                }
            }
        }
    } else if(RealTimeHpiResult::SPtr pRTHR = qSharedPointerDynamicCast<RealTimeHpiResult>(pMeasurement)) {
        if(!m_pBemHeadAvr) {
            // Note: Loading sensor surfaces from in-memory MNEBem is not directly
            // supported by disp3D_rhi's addSensors API. Sensor surfaces skipped.

            // Add average head surface
            QFile t_fileHeadAvr(QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-head.fif");
            MNEBem t_BemHeadAvr(t_fileHeadAvr);
            if(!t_BemHeadAvr.isEmpty()) {
                m_pBemHeadAvr = m_pData3DModel->addBemSurface("Subject", "Average head", t_BemHeadAvr[0]);
            }
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
                    m_pFiffDigitizerData = pDigData;
                    FiffDigPointSet digSet(pDigData->points);
                    addDigSetToView(digSet);
                    alignFiducials(pDigData);
                }
            }

            //Add fitted digitizers to 3D view
            m_pData3DModel->addDigitizerData(
                                             pHpiFitResult->fittedCoils.pickTypes(QList<int>()<<FIFFV_POINT_EEG).getList());

            // Note: Per-item transforms for HPI tracking are not yet supported in disp3D_rhi.
            // The BrainSurface renderable supports applyTransform() but tree items do not
            // expose this directly yet.
        }
    }
}

//=============================================================================================================

void RealTime3DWidget::addDigSetToView(const FIFFLIB::FiffDigPointSet& digSet)
{
    FiffDigPointSet digSetWithoutAdditional = digSet.pickTypes(QList<int>()<<FIFFV_POINT_HPI<<FIFFV_POINT_CARDINAL<<FIFFV_POINT_EEG<<FIFFV_POINT_EXTRA);
    m_pData3DModel->addDigitizerData(digSetWithoutAdditional.getList());
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(const QString& sFilePath)
{
    m_sFilePathDigitizers = sFilePath;

    QFile t_fileDigData(sFilePath);
    QSharedPointer<FIFFLIB::FiffDigitizerData> t_digData = QSharedPointer<FIFFLIB::FiffDigitizerData>::create(t_fileDigData);

    alignFiducials(t_digData);
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(QSharedPointer<FIFFLIB::FiffDigitizerData> pDigData)
{
    std::unique_ptr<MneMshDisplaySurfaceSet> pMneMshDisplaySurfaceSet = std::make_unique<MneMshDisplaySurfaceSet>();
    MneMshDisplaySurfaceSet::add_bem_surface(pMneMshDisplaySurfaceSet.get(),
                                             QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-head.fif",
                                             FIFFV_BEM_SURF_ID_HEAD,
                                             "head",
                                             1,
                                             1);

    MneMshDisplaySurface* surface = pMneMshDisplaySurfaceSet->surfs[0];

    QFile t_fileDigDataReference(QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-fiducials.fif");

    float scales[3];
    QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData(t_fileDigDataReference));
    MneSurfaceOrVolume::align_fiducials(pDigData.data(),
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

void RealTime3DWidget::applyAlignmentTransform(QMatrix4x4& invMat)
{
    m_tAlignment = invMat;

    // Apply alignment transform to the BEM tree item
    if(m_pBemHeadAvr) {
        m_pBemHeadAvr->setTransform(m_tAlignment);
    }
}

//=============================================================================================================

QMatrix4x4 RealTime3DWidget::calculateInverseMatrix(const QSharedPointer<FIFFLIB::FiffDigitizerData> pDigData,
                                                    float scale) const
{
    QMatrix4x4 invMat;

    // use inverse transform
    for(int r = 0; r < 3; ++r) {
        for(int c = 0; c < 3; ++c) {
            // also apply scaling factor
            invMat(r,c) = pDigData->head_mri_t_adj->invrot()(r,c) * scale;
        }
    }
    invMat(0,3) = pDigData->head_mri_t_adj->invmove()(0);
    invMat(1,3) = pDigData->head_mri_t_adj->invmove()(1);
    invMat(2,3) = pDigData->head_mri_t_adj->invmove()(2);

    return invMat;
}

//=============================================================================================================

void RealTime3DWidget::initDisplayControllWidgets()
{
    //Init control widgets
    QList<QWidget*> lControlWidgets;

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    Control3DView* pControl3DView = new Control3DView("MNESCAN/RT3DW", Q_NULLPTR, slControlFlags);
    pControl3DView->setObjectName("group_tab_View_General");
    lControlWidgets.append(pControl3DView);

    // Note: BrainView does not support setSceneColor, startStopCameraRotation,
    // toggleCoordAxis, setLightColor, setLightIntensity directly.
    // Connect available BrainView slots:
    connect(pControl3DView, &Control3DView::takeScreenshotChanged,
            m_p3DView.data(), &BrainView::saveSnapshot);

    connect(pControl3DView, &Control3DView::toggleSingleView,
            m_p3DView.data(), &BrainView::showSingleView);

    connect(pControl3DView, &Control3DView::toggleMutiview,
            m_p3DView.data(), &BrainView::showMultiView);

    emit displayControlWidgetsChanged(lControlWidgets, "3D View");

    m_bDisplayWidgetsInitialized = true;
}
