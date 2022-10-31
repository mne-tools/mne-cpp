//=============================================================================================================
/**
 * @file     abstractview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    AbstractView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractview.h"

#include "../engine/view/view3D.h"
#include "../engine/model/data3Dtreemodel.h"
#include "../engine/delegate/data3Dtreedelegate.h"

#include <inverse/dipoleFit/dipole_fit_settings.h>
#include <inverse/dipoleFit/ecd_set.h>
#include <mne/mne_bem.h>
#include <disp/viewers/quickcontrolview.h>
#include <disp/viewers/control3dview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QSizePolicy>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractView::AbstractView(QWidget* parent,
                           Qt::WindowFlags f)
: QWidget(parent, f)
, m_p3DView(View3D::SPtr(new View3D()))
, m_pData3DModel(Data3DTreeModel::SPtr(new Data3DTreeModel()))
{
    //Init 3D View
    m_p3DView->setFlag(Qt::FramelessWindowHint, true);
    m_p3DView->setModel(m_pData3DModel);

    // Init 3D control view
    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    m_pControl3DView = DISPLIB::Control3DView::SPtr(new DISPLIB::Control3DView("DISP3D", this, slControlFlags));

    Data3DTreeDelegate* pData3DTreeDelegate = new Data3DTreeDelegate(this);
    m_pControl3DView->setDelegate(pData3DTreeDelegate);
    m_pControl3DView->setModel(m_pData3DModel.data());

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::sceneColorChanged,
            m_p3DView.data(), &View3D::setSceneColor);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::rotationChanged,
            m_p3DView.data(), &View3D::startStopCameraRotation);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::showCoordAxis,
            m_p3DView.data(), &View3D::toggleCoordAxis);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::showFullScreen,
            m_p3DView.data(), &View3D::showFullScreen);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::lightColorChanged,
            m_p3DView.data(), &View3D::setLightColor);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::lightIntensityChanged,
            m_p3DView.data(), &View3D::setLightIntensity);

    connect(m_pControl3DView.data(), &DISPLIB::Control3DView::takeScreenshotChanged,
            m_p3DView.data(), &View3D::takeScreenshot);

    createGUI();
}

//=============================================================================================================

AbstractView::~AbstractView()
{
}

//=============================================================================================================

QSharedPointer<DISP3DLIB::View3D> AbstractView::getView()
{
    return m_p3DView;
}

//=============================================================================================================

QSharedPointer<DISPLIB::Control3DView> AbstractView::getControlView()
{
    return m_pControl3DView;
}

//=============================================================================================================

QSharedPointer<DISP3DLIB::Data3DTreeModel> AbstractView::getTreeModel()
{
    return m_pData3DModel;
}

//=============================================================================================================

QPointer<DISPLIB::QuickControlView> AbstractView::getQuickControl()
{
    return m_pQuickControlView;
}

//=============================================================================================================

void AbstractView::setQuickControlWidgets(const QList<QWidget*>& lControlWidgets)
{
    if(m_pQuickControlView) {
        for(int i = 0; i < lControlWidgets.size(); i++) {
            if(lControlWidgets.at(i)) {
                m_pQuickControlView->addGroupBox(lControlWidgets.at(i),
                                                 lControlWidgets.at(i)->windowTitle(),
                                                 "AbstractView");
            }
        }
    }
}

//=============================================================================================================

void AbstractView::createGUI()
{
    m_pQuickControlView = new DISPLIB::QuickControlView("3DView",
                                                        "3D View",
                                                        Qt::Widget,
                                                        this,
                                                        false);
    m_pQuickControlView->setVisiblityHideOpacityClose(false);

    //Create widget GUI
    m_pQuickControlView->addGroupBox(m_pControl3DView.data(), "3D View", "AbstractView");

    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_p3DView.data(), this, Qt::Widget);
    pWidgetContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pWidgetContainer->setMinimumSize(400,400);

    QGridLayout* pMainLayoutView = new QGridLayout();
    pMainLayoutView->addWidget(pWidgetContainer,0,0);
    pMainLayoutView->addWidget(m_pQuickControlView.data(),0,1);    
    pMainLayoutView->setContentsMargins(0,0,0,0);

    this->setLayout(pMainLayoutView);
}
