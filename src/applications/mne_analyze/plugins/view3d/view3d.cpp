//=============================================================================================================
/**
 * @file     view3d.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.6
 * @date     August, 2020
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
 * @brief    View3D class defintion.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include "view3d.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/dipolefitmodel.h>
#include <anShared/Model/abstractmodel.h>

#include <disp3D_rhi/view/brainview.h>
#include <disp3D_rhi/model/braintreemodel.h>
#include <disp3D_rhi/model/items/digitizersettreeitem.h>
#include <disp3D_rhi/model/items/digitizertreeitem.h>
#include <disp3D_rhi/model/items/bemtreeitem.h>
#include <disp3D_rhi/model/items/dipoletreeitem.h>

#include <disp/viewers/control3dview.h>

#include <fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace VIEW3DPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISPLIB;
using namespace FIFFLIB;
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

View3D::View3D()
    : m_pCommu(Q_NULLPTR)
    , m_pBemTreeCoreg(Q_NULLPTR)
    , m_pView3D(Q_NULLPTR)
    , m_bPickingActivated(false)
{
    m_iFiducial = 1;
}

//=============================================================================================================

View3D::~View3D()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> View3D::clone() const
{
    QSharedPointer<View3D> pView3DClone(new View3D);
    return pView3DClone;
}

//=============================================================================================================

void View3D::init()
{
    m_pCommu = new Communicator(this);
    m_p3DModel = QSharedPointer<BrainTreeModel>::create();
}

//=============================================================================================================

void View3D::unload()
{

}

//=============================================================================================================

QString View3D::getName() const
{
    return "3D View";
}

//=============================================================================================================

QMenu *View3D::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *View3D::getView()
{
    if(!m_pView3D) {
        m_pView3D = new BrainView();
    }

    m_pView3D->setModel(m_p3DModel.data());
    new3DModel(m_p3DModel);

    // Note: BrainView (QRhiWidget) does not support scene color, camera rotation toggle,
    // coord axis toggle, light color/intensity, or screenshot via direct slots.
    // Use BrainView::saveSnapshot() for screenshots, setLightingEnabled() for lighting.
    connect(this, &View3D::takeScreenshotChanged,
            m_pView3D, &BrainView::saveSnapshot);

    // BrainView is a QWidget (QRhiWidget), can be embedded directly
    return m_pView3D;
}

//=============================================================================================================

QDockWidget* View3D::getControl()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void View3D::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_BEM_CHANGED:
            updateCoregBem(e->getData().value<QSharedPointer<ANSHAREDLIB::BemDataModel>>());
            break;
        case EVENT_TYPE::NEW_DIGITIZER_ADDED:
            updateCoregDigitizer(e->getData().value<FiffDigPointSet>());
            break;
        case EVENT_TYPE::NEW_FIDUCIALS_ADDED:
            updateCoregMriFid(e->getData().value<FiffDigPointSet>());
            break;
        case EVENT_TYPE::NEW_TRANS_AVAILABE:
            updateCoregTrans(e->getData().value<FiffCoordTrans>());
            break;
        case EVENT_TYPE::FID_PICKING_STATUS:
            fiducialPicking(e->getData().value<bool>());
            break;
        case EVENT_TYPE::FIDUCIAL_CHANGED:
            onFiducialChanged(e->getData().value<int>());
            break;
        case EVENT_TYPE::VIEW3D_SETTINGS_CHANGED:
            settingsChanged(e->getData().value<ANSHAREDLIB::View3DParameters>());
            break;
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
            break;
        default:
            qWarning() << "[View3D::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> View3D::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_BEM_CHANGED);
    temp.push_back(NEW_DIGITIZER_ADDED);
    temp.push_back(NEW_FIDUCIALS_ADDED);
    temp.push_back(NEW_TRANS_AVAILABE);
    temp.push_back(FID_PICKING_STATUS);
    temp.push_back(FIDUCIAL_CHANGED);
    temp.push_back(VIEW3D_SETTINGS_CHANGED);
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(MODEL_REMOVED);
    return temp;
}

//=============================================================================================================

void View3D::updateCoregBem(QSharedPointer<ANSHAREDLIB::BemDataModel> pNewModel)
{
    if(!pNewModel){
        qWarning() << "[View3D::updateCoregBem] Null Bem Model pointer.";
        return;
    } else if(!m_p3DModel){
        std::cout << "[View3D::updateCoregBem] Null BrainTreeModel";
        return;
    } else if(pNewModel->getType() == ANSHAREDLIB_BEMDATA_MODEL) {
        if(!pNewModel->getBem().data()->isEmpty()) {
            m_pBemTreeCoreg = m_p3DModel->addBemSurface("Co-Registration",
                                                        QFileInfo(pNewModel->getModelPath()).fileName(),
                                                        (*pNewModel->getBem().data())[0]);
        }
    }
    return;
}

//=============================================================================================================

void View3D::updateCoregDigitizer(FiffDigPointSet digSet)
{
    m_p3DModel->addDigitizerData(digSet.getList());
    return;
}

//=============================================================================================================
void View3D::updateCoregMriFid(FiffDigPointSet digSetFid)
{
    m_p3DModel->addDigitizerData(digSetFid.getList());
    return;
}

//=============================================================================================================

void View3D::updateCoregTrans(FiffCoordTrans headMriTrans)
{
    // Note: Per-item transforms for digitizers are not yet supported in disp3D_rhi.
    // The coregistration transform will need to be applied via BrainView's scene transform
    // or by rebuilding the digitizer data in the transformed space.
    Q_UNUSED(headMriTrans)
    return;
}

//=============================================================================================================

void View3D::fiducialPicking(const bool bActivatePicking)
{
    m_bPickingActivated = bActivatePicking;

    if(bActivatePicking) {
        onFiducialChanged(m_iFiducial);
    }
}

//=============================================================================================================

void View3D::newPickingEvent(const QVector3D &worldIntersection)
{
    QVariant data = QVariant::fromValue(worldIntersection);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_FIDUCIAL_PICKED, data);
}

//=============================================================================================================

void View3D::onFiducialChanged(const int iFiducial)
{
    // Note: BrainView does not have setCameraRotation(int degrees).
    // Camera rotation for fiducial picking is not yet supported in disp3D_rhi.
    Q_UNUSED(iFiducial)
    m_iFiducial = iFiducial;
}

//=============================================================================================================

void View3D::new3DModel(QSharedPointer<BrainTreeModel> pModel)
{
    m_pCommu->publishEvent(EVENT_TYPE::SET_DATA3D_TREE_MODEL, QVariant::fromValue(pModel));
}

//=============================================================================================================

void View3D::settingsChanged(ANSHAREDLIB::View3DParameters viewParameters)
{
    switch (viewParameters.m_settingsToApply){
    case ANSHAREDLIB::View3DParameters::View3DSetting::sceneColor:
        emit sceneColorChanged(viewParameters.m_sceneColor);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::rotation:
        emit rotationChanged(viewParameters.m_bToggleRotation);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::coordAxis:
        emit showCoordAxis(viewParameters.m_bToogleCoordAxis);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::fullscreen:
        emit showFullScreen(viewParameters.m_bToggleFullscreen);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::lightColor:
        emit lightColorChanged(viewParameters.m_lightColor);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::lightIntensity:
        emit lightIntensityChanged(viewParameters.m_dLightIntensity);
        break;
    case ANSHAREDLIB::View3DParameters::View3DSetting::screenshot:
        emit takeScreenshotChanged();
        break;
    default:
        qInfo() << "[View3D::settingsChanged] Unknown setting";
    }
}

//=============================================================================================================

void View3D::newDipoleFit(const INVERSELIB::ECDSet &ecdSet)
{
    m_p3DModel->addDipoles(ecdSet);
}

//=============================================================================================================

void View3D::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_DIPOLEFIT_MODEL) {
        newDipoleFit(qSharedPointerCast<DipoleFitModel>(pNewModel)->data(QModelIndex()).value<INVERSELIB::ECDSet>());
    }
}

//=============================================================================================================

void View3D::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_DIPOLEFIT_MODEL) {
        QList<QStandardItem *> lItemList = m_p3DModel->findItems("Dipole Fit");
        if(!lItemList.isEmpty()){
            for(QStandardItem * pItem : lItemList){
                QModelIndex index = m_p3DModel->indexFromItem(pItem);
                m_p3DModel->removeRows(index.row(),
                                       1,
                                       index.parent());
            }
        }

        if(m_pView3D) {
            m_pView3D->hide();
            m_pView3D->show();
        }

    } else if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL){
        if(!m_pBemTreeCoreg){
            return;
        }

        QModelIndex index = m_p3DModel->indexFromItem(m_pBemTreeCoreg);

        m_p3DModel->removeRows(index.row(),
                               1,
                               index.parent());

        m_p3DModel->removeRows(index.parent().row(),
                               1,
                               index.parent().parent());

        m_p3DModel->removeRows(index.parent().parent().row(),
                               1,
                               index.parent().parent().parent());

        m_pBemTreeCoreg = Q_NULLPTR;

        if(m_pView3D) {
            m_pView3D->hide();
            m_pView3D->show();
        }
    }
}

//=============================================================================================================

QString View3D::getBuildInfo()
{
    return QString(UTILSLIB::dateTimeNow());
}
