//=============================================================================================================
/**
 * @file     dataloader.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     November, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Gabriel Motta. All rights reserved.
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

#include <anShared/Plugins/abstractplugin.h>
#include <anShared/Model/fiffrawviewmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

namespace DISPLIB {
    class ProgressView;
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
class DATALOADERSHARED_EXPORT DataLoader : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "dataloader.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

    enum FileType {DATA_FILE, AVERAGE_FILE, EVENT_FILE};
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

    //=========================================================================================================
    /**
     * Initializes the plugin based on cmd line inputs given by the user.
     *
     * @param[in] sArguments  the cmd line arguments.
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

    void onLoadScanSessionPressed();

    //=========================================================================================================
    /**
     * This functions is called when the save to file button is pressed.
     *
     * @param[in] type     type of file being saved.
     */
    void onSaveFilePressed(FileType type = DATA_FILE);

    //=========================================================================================================
    /**
     * This function is called when the load from folder button is pressed
     */
    void onLoadSubjectPressed();

    //=========================================================================================================
    /**
     * This function is called when the load from session button is pressed
     */
    void onLoadSessionPressed();

    //=========================================================================================================
    /**
     * Shows loading bar with sMessage, dims therest of the window
     *
     * @param[in] sMessage     message to be shown alongside loading bar.
     */
    void startProgress(QString sMessage);

    //=========================================================================================================
    /**
     * Hides loading and loading message, undims the window
     */
    void endProgress();

    //=========================================================================================================
    /**
     * Loads settings from the system.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Save the settings in the register/file/mechanism for lon-term storage.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Update the value of m_sLastDir member variable, both in the in-memory object and also in the storage system.
     */
    void updateLastDir(const QString& lastDir);

    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    QPointer<ANSHAREDLIB::Communicator>             m_pCommu;                   /**< Used for sending events. */
    QPointer<DISPLIB::ProgressView>                 m_pProgressView;            /**< Holds loading bar and loading message. */
    QPointer<QWidget>                               m_pProgressViewWidget;      /**< Window for ProgressView. */

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>   m_pSelectedModel;           /**< Pointer to currently selected Fiff model. */
    QString                                         m_sSettingsPath;            /**< Variable that stores the key where to store settings for this plugin.*/
    QString                                         m_sLastDir;                 /**< Variable to store the last directory from where data were loaded.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DATALOADER_H
