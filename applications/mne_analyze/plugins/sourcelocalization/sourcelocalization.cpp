//=============================================================================================================
/**
 * @file     sourcelocalization.cpp
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
 * @brief    SourceLocalization class defintion.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelocalization.h"

#include <anShared/Management/communicator.h>

#include <anShared/Model/abstractmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/forwardsolutionmodel.h>

#include <inverse/minimumNorm/minimumnorm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SOURCELOCALIZATIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLocalization::SourceLocalization()
: m_pCommu(Q_NULLPTR)
, m_pFwdSolutionModel(Q_NULLPTR)
, m_pAverageDataModel(Q_NULLPTR)
, m_pRawDataModel(Q_NULLPTR)
, m_iSelectedSample(-1)
, m_bUpdateMinNorm(false)
{
}

//=============================================================================================================

SourceLocalization::~SourceLocalization()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> SourceLocalization::clone() const
{
    QSharedPointer<SourceLocalization> pSourceLocalizationClone(new SourceLocalization);
    return pSourceLocalizationClone;
}

//=============================================================================================================

void SourceLocalization::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void SourceLocalization::unload()
{

}

//=============================================================================================================

QString SourceLocalization::getName() const
{
    return "Source Localization";
}

//=============================================================================================================

QMenu *SourceLocalization::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *SourceLocalization::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget* SourceLocalization::getControl()
{
    //QDockWidget* pControl = new QDockWidget(getName());

    return Q_NULLPTR;
}

//=============================================================================================================

void SourceLocalization::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    case EVENT_TYPE::MODEL_REMOVED:
        onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
        break;
    case EVENT_TYPE::SAMPLE_SELECTED:
        m_iSelectedSample = e->getData().value<int>();
        sourceLocalizationFromSingleTrial();
    default:
        qWarning() << "[Events::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> SourceLocalization::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(MODEL_REMOVED);
    temp.push_back(SAMPLE_SELECTED);

    return temp;
}

//=============================================================================================================

QString SourceLocalization::getBuildInfo()
{
    return QString(SOURCELOCALIZATIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(SOURCELOCALIZATIONPLUGIN::buildHash());
}

//=============================================================================================================

void SourceLocalization::sourceLocalizationFromAverage()
{
    if(m_sourceLocalizationMode != SOURCE_LOC_FROM_AVG) {
        return;
    }

    if(!m_pFwdSolutionModel){
        qInfo() << "[SourceLocalization::performSourceLocalization] No forward solution available.";
        return;
    }

    if(!m_pAverageDataModel){
        qInfo() << "[SourceLocalization::performSourceLocalization] No average model available.";
        return;
    }
}

//=============================================================================================================

void SourceLocalization::sourceLocalizationFromSingleTrial()
{
    if(m_sourceLocalizationMode != SOURCE_LOC_FROM_SINGLE_TRIAL) {
        return;
    }

    if(!m_pFwdSolutionModel){
        qInfo() << "[SourceLocalization::performSourceLocalization] No forward solution available.";
        return;
    }

    if(!m_pRawDataModel){
        qInfo() << "[SourceLocalization::performSourceLocalization] No raw data model available.";
        return;
    }


    if(m_bUpdateMinNorm) {
        m_pMinimumNorm = INVERSELIB::MinimumNorm::SPtr(new INVERSELIB::MinimumNorm(m_invOp, 1.0f / pow(1.0f, 2), "MNE"));
        m_bUpdateMinNorm = false;

        // Set up the inverse according to the parameters.
        // Use 1 nave here because in case of evoked data as input the minimum norm will always be updated when the source estimate is calculated (see run method).
        m_pMinimumNorm->doInverseSetup(1,true);
    }


}

//=============================================================================================================

void SourceLocalization::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    switch(pNewModel->getType()){
        case ANSHAREDLIB::ANSHAREDLIB_FIFFRAW_MODEL:
            m_pRawDataModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
            break;
        case ANSHAREDLIB::ANSHAREDLIB_AVERAGING_MODEL:
            m_pAverageDataModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
            sourceLocalizationFromAverage();
            break;
        case ANSHAREDLIB::ANSHAREDLIB_FORWARDSOLUTION_MODEL:
            m_pFwdSolutionModel = qSharedPointerCast<ForwardSolutionModel>(pNewModel);

            break;
        default:
            break;
    }
}

//=============================================================================================================

void SourceLocalization::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{

}
