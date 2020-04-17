//=============================================================================================================
/**
 * @file     annotationmanager.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the AnnotationManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmanager.h"
//#include "FormFiles/annotationmanagercontrol.h"

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Management/communicator.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANNOTATIONMANAGEREXTENSION;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationManager::AnnotationManager()
{

}

//=============================================================================================================

AnnotationManager::~AnnotationManager()
{

}

//=============================================================================================================

QSharedPointer<IExtension> AnnotationManager::clone() const
{
    QSharedPointer<AnnotationManager> pAnnotationManagerClone(new AnnotationManager);
    return pAnnotationManagerClone;
}

//=============================================================================================================

void AnnotationManager::init()
{
    m_pCommu = new Communicator(this);

    m_pAnnotationView = QSharedPointer<AnnotationView>(new AnnotationView());

    connect(m_pAnalyzeData.data(), &AnalyzeData::newModelAvailable,
            this, &AnnotationManager::onModelChanged);
    connect(m_pAnalyzeData.data(), &AnalyzeData::selectedModelChanged,
            this, &AnnotationManager::onModelChanged);

//    m_pAnnotationManagerControl = new AnnotationManagerControl();

//    connect(m_pAnnotationManagerControl.data(), &AnnotationManagerControl::loadFiffFile,
//            this, &AnnotationManager::onLoadFiffFilePressed);

//    connect(m_pAnnotationManagerControl.data(), &AnnotationManagerControl::saveFiffFile,
//            this, &AnnotationManager::onSaveFiffFilePressed);
}

//=============================================================================================================

void AnnotationManager::unload()
{

}

//=============================================================================================================

QString AnnotationManager::getName() const
{
    return "Data Loader";
}

//=============================================================================================================

QMenu *AnnotationManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *AnnotationManager::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("Annotation Manager"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        m_pControl->setWidget(m_pAnnotationView.data());
    }

    return m_pControl;
}

//=============================================================================================================

QWidget *AnnotationManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void AnnotationManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
//    case EVENT_TYPE::CURRENTLY_SELECTED_MODEL:
//        m_sCurrentlySelectedModel = e->getData().toString();
//        qDebug() << m_sCurrentlySelectedModel;
//        break;
    case EVENT_TYPE::NEW_ANNOTATION_ADDED:
        m_pAnnotationView->addAnnotationToModel(e->getData().toInt());
    default:
        qWarning() << "[AnnotationManager::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> AnnotationManager::getEventSubscriptions(void) const
{
    //QVector<EVENT_TYPE> temp = {CURRENTLY_SELECTED_MODEL};
    QVector<EVENT_TYPE> temp;
    temp.push_back(NEW_ANNOTATION_ADDED);

    return temp;
}

//=============================================================================================================

void AnnotationManager::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pFiffRawModel) {
            if(m_pFiffRawModel == pNewModel) {
                return;
            }
        }
        m_pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);

        m_pAnnotationView->passFiffParams(m_pFiffRawModel->absoluteFirstSample(),
                                           m_pFiffRawModel->absoluteLastSample(),
                                           m_pFiffRawModel->getFiffInfo()->sfreq);
        setUpControls();
    }
}

//=============================================================================================================

void AnnotationManager::setUpControls()
{
    connect(m_pAnnotationView.data(), &AnnotationView::activeEventsChecked,
            this, &AnnotationManager::toggleDisplayEvent);
}

//=============================================================================================================

void AnnotationManager::toggleDisplayEvent(const int& iToggle)
{
    int m_iToggle = iToggle;
    qDebug() << "toggleDisplayEvent" << iToggle;
    m_pFiffRawModel->toggleDispAnn(m_iToggle);
}
