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

#include <mne/base/log.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/filter_settings_panel.h>
#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FILTERINGPLUGIN;
using namespace mne::analyze;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Filtering::Filtering()
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

QMenu* Filtering::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QDockWidget* Filtering::getControl()
{
    auto pControlDock = std::make_unique<QDockWidget>(getName());
    m_pFilterPanel = std::make_shared<FilterSettingsPanel>();
    auto* pFilterWidget = new PanelWidget(m_pFilterPanel, "MNEANALYZE", pControlDock.get());

    connect(this, &Filtering::guiModeChanged,
            pFilterWidget, &PanelWidget::setGuiMode, Qt::UniqueConnection);

    const QPointer<Filtering> plugin(this);
    const std::shared_ptr<FilterDesignPanel> design = m_pFilterPanel->design();
    FilterDesignPanel::Callbacks designCallbacks;
    designCallbacks.channel_type_changed = [plugin](const std::string& type) {
        if (plugin) {
            plugin->setFilterChannelType(QString::fromStdString(type));
        }
    };
    designCallbacks.filter_changed = [plugin](const FilterKernel& filter) {
        if (plugin) {
            plugin->setFilter(filter);
        }
    };
    design->set_callbacks(std::move(designCallbacks));

    // The filter design opens in a window of its own
    FilterSettingsPanel::Callbacks filterCallbacks;
    filterCallbacks.activation_changed = [plugin](bool active) {
        if (plugin) {
            plugin->setFilterActive(active);
        }
    };
    const QPointer<PanelWidget> filterWidget(pFilterWidget);
    filterCallbacks.show_design = [filterWidget, design]() {
        if (!filterWidget) {
            return;
        }
        auto* window = filterWidget->findChild<PanelWidget*>(QStringLiteral("FilterDesignWindow"));
        if (!window) {
            window = new PanelWidget(design, QString(), filterWidget);
            window->setObjectName(QStringLiteral("FilterDesignWindow"));
            window->setWindowFlags(Qt::Window);
        }
        if (window->isVisible() && window->isActiveWindow()) {
            window->hide();
        } else {
            window->show();
            window->raise();
            window->activateWindow();
        }
    };
    m_pFilterPanel->set_callbacks(std::move(filterCallbacks));

    setFilter(design->filter());

    pControlDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    pControlDock->setWidget(pFilterWidget);
    pControlDock->setObjectName(getName());

    return pControlDock.release();
}

//=============================================================================================================

QWidget* Filtering::getView()
{
    return nullptr;
}

//=============================================================================================================

void Filtering::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case SELECTED_MODEL_CHANGED:
            if (e->getData().value<QSharedPointer<AbstractModel>>()->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
                if (QSharedPointer<FiffRawViewModel> pModel = qSharedPointerCast<FiffRawViewModel>(e->getData().value<QSharedPointer<AbstractModel>>())) {
                    if (m_pFilterPanel) {
                        setFilterActive(m_pFilterPanel->active());
                        if (auto info = pModel->getFiffInfo()) {
                            m_pFilterPanel->set_sampling_rate(info->sfreq);
                            setFilterChannelType(QString::fromStdString(m_pFilterPanel->design()->channel_type()));
                            setFilter(m_pFilterPanel->design()->filter());
                        }
                    }
                }
            }
            break;

        default:
            Log::warning("[Filtering::handleEvent] received an Event that is not handled by switch-cases");
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
    return QString(FILTERINGPLUGIN::buildDateTime()) + QString(" - ") + QString(FILTERINGPLUGIN::buildHash());
}
