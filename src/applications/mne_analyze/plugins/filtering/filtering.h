//=============================================================================================================
/**
 * @file     filtering.h
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
 * @brief    Contains the declaration of the Filtering class.
 *
 */

#ifndef FILTERING_H
#define FILTERING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtering_global.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
}

namespace RTPROCESSINGLIB {
    class FilterKernel;
}

namespace DISPLIB {
    class FilterSettingsView;
}

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace FILTERINGPLUGIN
{

//=============================================================================================================
/**
 * Filtering Plugin
 *
 * @brief The Filtering class provides a view with all currently loaded models.
 */
class FILTERINGSHARED_EXPORT Filtering : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "filtering.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a Filtering.
     */
    Filtering();

    //=========================================================================================================
    /**
     * Destroys the Filtering.
     */
    virtual ~Filtering() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;
    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Sets the type of channel which are to be filtered
     *
     * @param[in] sType    The channel type which is to be filtered (EEG, MEG, All).
     */
    void setFilterChannelType(const QString& sType);

    //=========================================================================================================
    /**
     * Filter parameters changed
     *
     * @param[in] filterData    The currently active filter.
     */
    void setFilter(const RTPROCESSINGLIB::FilterKernel& filterData);

    //=========================================================================================================
    /**
     * Filter avtivated
     *
     * @param[in] state    The filter on/off flag.
     */
    void setFilterActive(bool state);

    QPointer<ANSHAREDLIB::Communicator>         m_pCommu;               /**< To broadcst signals. */
    QPointer<DISPLIB::FilterSettingsView>       m_pFilterSettingsView;

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FILTERING_H
