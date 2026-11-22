//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     realtime3dwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Definition of the RealTime3DWidget Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtime3dwidget.h"

#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimehpiresult.h>
#include <mne/connectivity/network/network.h>
#include <mne/mne/mne_bem.h>
#include <mne/disp3D/view/brainview.h>
#include <mne/disp3D/model/braintreemodel.h>
#include <mne/disp3D/model/items/networktreeitem.h>
#include <mne/disp3D/model/items/sensortreeitem.h>
#include <mne/disp3D/model/items/surfacetreeitem.h>
#include <mne/disp3D/model/items/bemtreeitem.h>
#include <mne/disp3D/model/items/digitizersettreeitem.h>
#include <mne/disp/viewers/control3dview.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_digitizer_data.h>
#include <mne/mne/mne_msh_display_surface_set.h>
#include <mne/mne/mne_msh_display_surface.h>
#include <mne/mne/mne_surface_or_volume.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QGridLayout>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne::disp;
using namespace mne;

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
    m_p3DView->setMinimumSize(400, 400);

    QGridLayout* pMainLayoutView = new QGridLayout();
    pMainLayoutView->addWidget(m_p3DView, 0, 0);
    pMainLayoutView->setContentsMargins(0, 0, 0, 0);

    this->setLayout(pMainLayoutView);
}

//=============================================================================================================

void RealTime3DWidget::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (!m_bDisplayWidgetsInitialized) {
        initDisplayControllWidgets();
    }

    if (RealTimeConnectivityEstimate::SPtr pRTCE = qSharedPointerDynamicCast<RealTimeConnectivityEstimate>(pMeasurement)) {
        if (pRTCE->getValue().data()->empty()) {
            return;
        }

        // Add rt connectivity data
        if (!m_pRtConnectivityItem) {
            m_pRtConnectivityItem = m_pData3DModel->addNetwork(*(pRTCE->getValue().data()),
                                                               "Functional Data");

            if (pRTCE->getSurfSet() && pRTCE->getAnnotSet()) {
                for (int i = 0; i < pRTCE->getSurfSet()->size(); ++i) {
                    const mne::FsSurface& surf = (*pRTCE->getSurfSet())[i];
                    QString hemi = (surf.hemi() == 0) ? "lh" : "rh";
                    QString surfType = QString::fromStdString(surf.surf().empty() ? "inflated" : surf.surf());
                    m_pData3DModel->addSurface("Subject", hemi, surfType, surf);
                    m_pData3DModel->addAnnotation("Subject", hemi, (*pRTCE->getAnnotSet())[i]);
                }
            }

            // Note: Sensor surface visualization from FiffChInfo/MNEBem is not
            // directly supported in disp3D's addSensors API. Use loadSensors()
            // with a .fif file path if sensor visualization is needed.
            m_iNumberBadChannels = pRTCE->getFiffInfo() ? static_cast<int>(pRTCE->getFiffInfo()->bads.size()) : 0;
        } else {
            // Update existing connectivity data - replace with new network
            const auto [lower, upper] = pRTCE->getValue()->frequency_range();
            QString sItemName = QString("%1_%2_%3").arg(QString::fromStdString(pRTCE->getValue()->method()), QString::number(lower), QString::number(upper));
            m_pRtConnectivityItem->setText(sItemName);

            if (pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                if (m_iNumberBadChannels != static_cast<int>(pRTCE->getFiffInfo()->bads.size())) {
                    // Note: Sensor update from in-memory FiffChInfo/MNEBem not
                    // directly supported in disp3D.
                    m_iNumberBadChannels = static_cast<int>(pRTCE->getFiffInfo()->bads.size());
                }
            }
        }
    } else if (RealTimeSourceEstimate::SPtr pRTSE = qSharedPointerDynamicCast<RealTimeSourceEstimate>(pMeasurement)) {
        QList<std::shared_ptr<InvSourceEstimate>> lMNEData = pRTSE->getValue();

        // Add source estimate data via BrainView's realtime streaming
        if (!lMNEData.isEmpty()) {
            if (!m_bRtSourceActive && pRTSE->getAnnotSet() && pRTSE->getSurfSet() && pRTSE->getFwdSolution()) {
                // Add surfaces first
                for (int i = 0; i < pRTSE->getSurfSet()->size(); ++i) {
                    const mne::FsSurface& surf = (*pRTSE->getSurfSet())[i];
                    QString hemi = (surf.hemi() == 0) ? "lh" : "rh";
                    QString surfType = QString::fromStdString(surf.surf().empty() ? "inflated" : surf.surf());
                    m_pData3DModel->addSurface("Subject", hemi, surfType, surf);
                    m_pData3DModel->addAnnotation("Subject", hemi, (*pRTSE->getAnnotSet())[i]);
                }
                // Configure and start realtime streaming
                m_p3DView->setSourceColormap("Hot");
                m_p3DView->startRealtimeStreaming();
                m_bRtSourceActive = true;
                m_mriHeadTrans = pRTSE->getMriHeadTrans();
            } else {
                if (m_bRtSourceActive) {
                    // Extract data column from InvSourceEstimate
                    const InvSourceEstimate& stc = *lMNEData.first();
                    if (stc.data.cols() > 0) {
                        m_p3DView->pushRealtimeSourceData(stc.data.col(0));
                    }
                }
            }
        }
    } else if (RealTimeHpiResult::SPtr pRTHR = qSharedPointerDynamicCast<RealTimeHpiResult>(pMeasurement)) {
        if (!m_pBemHeadAvr) {
            // Note: Loading sensor surfaces from in-memory MNEBem is not directly
            // supported by disp3D's addSensors API. Sensor surfaces skipped.

            // Add average head surface
            MNEBem t_BemHeadAvr((QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-head.fif").toStdU16String());
            if (!t_BemHeadAvr.empty()) {
                m_pBemHeadAvr = m_pData3DModel->addBemSurface("Subject", "Average head", t_BemHeadAvr[0]);
            }
        }

        if (QSharedPointer<HpiFitResult> pHpiFitResult = pRTHR->getValue()) {
            const QString digitizerFile = QString::fromStdString(mne::utf8_from_path(pHpiFitResult->digitizerFile));
            if (m_sFilePathDigitizers != digitizerFile && m_pBemHeadAvr) {
                //Add all digitizer but additional points to the 3D view
                FiffDigPointSet digSet(pHpiFitResult->digitizerFile);
                addDigSetToView(digSet);
                alignFiducials(digitizerFile);
            }
            if (!m_pFiffDigitizerData && m_pBemHeadAvr) {
                if (auto pDigData = pRTHR->digitizerData()) {
                    m_pFiffDigitizerData = pDigData;
                    addDigSetToView(pDigData->points);
                    alignFiducials(pDigData);
                }
            }

            //Add fitted digitizers to 3D view
            m_pData3DModel->addDigitizerData(pHpiFitResult->fittedCoils.pick_types({FIFFV_POINT_EEG}).points());

            // Draw where the subject's head has been over the course of the
            // measurement, so drift is visible rather than just the current
            // position.
            m_p3DView->setHeadMovementPath(pRTHR->headPositionHistory());

            // Note: Per-item transforms for HPI tracking are not yet supported in disp3D.
            // The BrainSurface renderable supports applyTransform() but tree items do not
            // expose this directly yet.
        }
    }
}

//=============================================================================================================

void RealTime3DWidget::addDigSetToView(const mne::FiffDigPointSet& digSet)
{
    const FiffDigPointSet digSetWithoutAdditional = digSet.pick_types({FIFFV_POINT_HPI, FIFFV_POINT_CARDINAL, FIFFV_POINT_EEG, FIFFV_POINT_EXTRA});
    m_pData3DModel->addDigitizerData(digSetWithoutAdditional.points());
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(const QString& sFilePath)
{
    m_sFilePathDigitizers = sFilePath;

    std::shared_ptr<mne::FiffDigitizerData> t_digData = std::make_shared<mne::FiffDigitizerData>(sFilePath.toStdU16String());

    alignFiducials(t_digData);
}

//=============================================================================================================

void RealTime3DWidget::alignFiducials(const std::shared_ptr<mne::FiffDigitizerData>& pDigData)
{
    std::unique_ptr<MNEMshDisplaySurfaceSet> pMneMshDisplaySurfaceSet = std::make_unique<MNEMshDisplaySurfaceSet>();
    pMneMshDisplaySurfaceSet->add_bem_surface((QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-head.fif").toStdU16String(),
                                              FIFFV_BEM_SURF_ID_HEAD,
                                              "head",
                                              1,
                                              1);

    MNEMshDisplaySurface* surface = pMneMshDisplaySurfaceSet->surfs[0].get();


    Eigen::Vector3f scales;
    QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData((QCoreApplication::applicationDirPath() + "/../resources/general/hpiAlignment/fsaverage-fiducials.fif").toStdU16String()));
    surface->align_fiducials(*pDigData,
                             *t_digDataReference,
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
    if (m_pBemHeadAvr) {
        m_pBemHeadAvr->setTransform(m_tAlignment);
    }
}

//=============================================================================================================

QMatrix4x4 RealTime3DWidget::calculateInverseMatrix(const std::shared_ptr<mne::FiffDigitizerData>& pDigData,
                                                    float scale) const
{
    QMatrix4x4 invMat;

    // use inverse transform
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            // also apply scaling factor
            invMat(r, c) = pDigData->head_mri_t_adj->invrot()(r, c) * scale;
        }
    }
    invMat(0, 3) = pDigData->head_mri_t_adj->invmove()(0);
    invMat(1, 3) = pDigData->head_mri_t_adj->invmove()(1);
    invMat(2, 3) = pDigData->head_mri_t_adj->invmove()(2);

    return invMat;
}

//=============================================================================================================

void RealTime3DWidget::initDisplayControllWidgets()
{
    //Init control widgets
    QList<QWidget*> lControlWidgets;

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    Control3DView* pControl3DView = new Control3DView("MNESCAN/RT3DW", nullptr, slControlFlags);
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
