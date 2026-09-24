//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     datamanager.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2018
 * @brief    Contains the declaration of the DataManager class.
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
    virtual QString getBuildInfo() override;
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
