//=============================================================================================================
/**
 * @file     filtering.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     May, 2020
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
 * @brief    Definition of the Filtering class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtering.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/filterdesignview.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FILTERINGPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Filtering::Filtering()
: m_pFilterSettingsView(Q_NULLPTR)
{
}

//=============================================================================================================

Filtering::~Filtering()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Filtering::clone() const
{
    QSharedPointer<Filtering> pFilteringClone = QSharedPointer<Filtering>::create();
    return pFilteringClone;
}

//=============================================================================================================

void Filtering::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void Filtering::unload()
{
}

//=============================================================================================================

QString Filtering::getName() const
{
    return "Filter";
}

//=============================================================================================================

QMenu *Filtering::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *Filtering::getControl()
{
    m_pFilterSettingsView = new FilterSettingsView("MNEANALYZE");

    connect(this, &Filtering::guiModeChanged,
            m_pFilterSettingsView.data(), &FilterSettingsView::setGuiMode, Qt::UniqueConnection);

    connect(this, &Filtering::guiStyleChanged,
            m_pFilterSettingsView.data(), &FilterSettingsView::guiStyleChanged, Qt::UniqueConnection);

    connect(m_pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
            this, &Filtering::setFilterChannelType, Qt::UniqueConnection);

    connect(m_pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
            this, &Filtering::setFilter, Qt::UniqueConnection);

    connect(m_pFilterSettingsView.data(), &FilterSettingsView::filterActivationChanged,
            this, &Filtering::setFilterActive, Qt::UniqueConnection);

    setFilter(m_pFilterSettingsView->getFilterView()->getCurrentFilter());

    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    pControlDock->setWidget(m_pFilterSettingsView);
    pControlDock->setObjectName(getName());

    return pControlDock;
}

//=============================================================================================================

QWidget *Filtering::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void Filtering::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case SELECTED_MODEL_CHANGED:
        if(e->getData().value<QSharedPointer<AbstractModel> >()->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
            if(QSharedPointer<FiffRawViewModel> pModel = qSharedPointerCast<FiffRawViewModel>(e->getData().value<QSharedPointer<AbstractModel> >())) {
                if(m_pFilterSettingsView) {
                    setFilterActive(m_pFilterSettingsView->getFilterActive());
                    if(auto info = pModel->getFiffInfo()){
                        m_pFilterSettingsView->getFilterView()->setSamplingRate(info->sfreq);
                        //m_pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(pModel->getFiffInfo()->sfreq);
                        setFilterChannelType(m_pFilterSettingsView->getFilterView()->getChannelType());
                        setFilter(m_pFilterSettingsView->getFilterView()->getCurrentFilter());
                    }
                }
            }
        }
        break;

    default:
        qWarning() << "[Filtering::handleEvent] received an Event that is not handled by switch-cases";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> Filtering::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void Filtering::setFilterChannelType(const QString& sType)
{
    QVariant data;
    data.setValue(sType);
    m_pCommu->publishEvent(EVENT_TYPE::FILTER_CHANNEL_TYPE_CHANGED, data);
}

//=============================================================================================================

void Filtering::setFilter(const FilterKernel& filterData)
{
    QVariant data;
    data.setValue(filterData);
    m_pCommu->publishEvent(EVENT_TYPE::FILTER_DESIGN_CHANGED, data);
}

//=============================================================================================================

void Filtering::setFilterActive(bool state)
{
    QVariant data;
    data.setValue(state);
    m_pCommu->publishEvent(EVENT_TYPE::FILTER_ACTIVE_CHANGED, data);
}

//=============================================================================================================

QString Filtering::getBuildInfo()
{
    return QString(FILTERINGPLUGIN::buildDateTime()) + QString(" - ")  + QString(FILTERINGPLUGIN::buildHash());
}
