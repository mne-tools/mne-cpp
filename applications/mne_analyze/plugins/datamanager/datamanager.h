//=============================================================================================================
/**
 * @file     datamanager.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the DataManager class.
 *
 */

#ifndef DATAMANAGER_H
#define DATAMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamanager_global.h"
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

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace DATAMANAGERPLUGIN
{

//=============================================================================================================
/**
 * DataManager Plugin
 *
 * @brief The DataManager class provides a view with all currently loaded models.
 */
class DATAMANAGERSHARED_EXPORT DataManager : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "datamanager.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a DataManager.
     */
    DataManager();

    //=========================================================================================================
    /**
     * Destroys the DataManager.
     */
    virtual ~DataManager() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;
    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildDateTime() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Handles the event when the currently selected model was changed.
     *
     * @param[in] data  The data from the currently selected QStandardItem.
     */
    void onCurrentlySelectedModelChanged(const QVariant& data);

    //=========================================================================================================
    /**
     * Triggers update to saved items in AnalyzeData
     *
     * @param[in] pIndex   index of newly selected item to be stored in AnalyzeData.
     */
    void onCurrentItemChanged(const QModelIndex& pIndex);

    //=========================================================================================================
    /**
     * Triggers removeal of item at index
     *
     * @param[in] index    model index of item to be removed.
     */
    void onRemoveItem(const QModelIndex &index);

    QPointer<ANSHAREDLIB::Communicator> m_pCommu;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DATAMANAGER_H
