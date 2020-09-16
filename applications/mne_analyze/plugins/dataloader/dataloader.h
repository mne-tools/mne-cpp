//=============================================================================================================
/**
 * @file     dataloader.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Contains the declaration of the DataLoader class.
 *
 */

#ifndef DATALOADER_H
#define DATALOADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader_global.h"

#include <anShared/Interfaces/IPlugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

//=============================================================================================================
// DEFINE NAMESPACE DATALOADERPLUGIN
//=============================================================================================================

namespace DATALOADERPLUGIN
{

//=============================================================================================================
// DATALOADERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class DataLoaderControl;

//=============================================================================================================
/**
 * DataLoader Plugin
 *
 * @brief The DataLoader class provides input and output capabilities for the fiff file format.
 */
class DATALOADERSHARED_EXPORT DataLoader : public ANSHAREDLIB::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "dataloader.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a DataLoader.
     */
    DataLoader();

    //=========================================================================================================
    /**
     * Destroys the DataLoader.
     */
    ~DataLoader() override;

    // IPlugin functions
    virtual QSharedPointer<IPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

    //=========================================================================================================
    /**
     * Initializes the plugin based on cmd line inputs given by the user.
     *
     * @param[in] sArguments  the cmd line arguments
     */
    virtual void cmdLineStartup(const QStringList& sArguments) override;

private:
    //=========================================================================================================
    /**
     * Load file from given file path.
     *
     * @param[in] sFilePath  the file path to load data from.
     */
    void loadFilePath(const QString& sFilePath);

    //=========================================================================================================
    /**
     * This functions is called when the load from file button is pressed.
     */
    void onLoadFilePressed();

    //=========================================================================================================
    /**
     * Triggers after file is done loading. Calls triggerLoadingEnd.
     */
    void loadFileEnd();

    //=========================================================================================================
    /**
     * This functions is called when the save to file button is pressed.
     */
    void onSaveFilePressed();

    //=========================================================================================================
    /**
     * Saves selected model (m_pSelectedModel) to file. To be run in separate thread with QFuture.
     *
     * @param sFilePath
     */
    void saveFile(const QString sFilePath);

    //=========================================================================================================
    /**
     * Triggers after file is done saving. Calls triggerLoadingEnd.
     */
    void saveFileEnd();

    //=========================================================================================================
    /**
     * Sends event to trigger loading bar to appear and sMessage to show
     *
     * @param [in] sMessage     loading bar message
     */
    void triggerLoadingStart(const QString& sMessage);

    //=========================================================================================================
    /**
     * Sends event to hide loading bar
     */
    void triggerLoadingEnd(const QString& sMessage);

    QPointer<ANSHAREDLIB::Communicator>         m_pCommu;               /** < Communicator used to send events via the event system */

    QSharedPointer<ANSHAREDLIB::AbstractModel>  m_pSelectedModel;       /** < Pointer to currently selected model */

    QFutureWatcher <void>                       m_FutureWatcher;        /** < Watches m_Future and signals when done */
    QFuture<void>                               m_Future;               /** < Used to perfom operations in a separate thread */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DATALOADER_H
