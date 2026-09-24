//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     filtering.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.2
 * @date     May, 2020
 * @brief    Definition of the Filtering class.
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

#include <dsp/filterkernel.h>

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
using namespace UTILSLIB;

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
