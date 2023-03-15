//=============================================================================================================
/**
 * @file     analyzecore.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief     AnalyzeCore class declaration.
 *
 */

#ifndef MNEANALYZE_ANALYZECORE_H
#define MNEANALYZE_ANALYZECORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QCommandLineParser>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB
{
    class AbstractPlugin;
    class PluginManager;
    class AnalyzeData;
}

//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE {

//=============================================================================================================
// MNEANALYZE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow;

//=============================================================================================================
/**
 * The AnalyzeCore class holds all of MNE-Analyze components.
 * It is the entry point for starting MNE-Analyze.
 *
 * @brief The AnalyzeCore holds all of MNE-Analyze components.
 */
class AnalyzeCore : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<AnalyzeCore> SPtr;            /**< Shared pointer type for AnalyzeCore. */
    typedef QSharedPointer<const AnalyzeCore> ConstSPtr; /**< Const shared pointer type for AnalyzeCore. */

    //=========================================================================================================
    /**
     * Constructs an AnalyzeCore object.
     */
    AnalyzeCore(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructs an AnalyzeCore object.
     */
    ~AnalyzeCore();

    //=========================================================================================================
    /**
     * This makes the main window visible.
     */
    void showMainWindow();

    //=========================================================================================================
    /**
     * Make pluginManager to reload and reinitialize all the necessary plugins, and make the MainWindow to
     * show the results.
     */
    void reloadPlugins();

    //=========================================================================================================
    /**
     * Initializes the global data base
     */
    void initGlobalData();

    //=========================================================================================================
    /**
     * Retrieve all the loaded plugins from the Application.
     */
    QVector<ANSHAREDLIB::AbstractPlugin*> getLoadedPlugins();

    //=========================================================================================================
    /**
     * Check for the initialization state of the plugins in the application.
     * @return Initialization state.
     */
    bool pluginsInitialized() const;

private:
    //=========================================================================================================
    /**
     * Init the command line parser
     */
    void initCmdLineParser();

    //=========================================================================================================
    /**
     * Parse the cmd line arguments and pass them to the plugins
     */
    void parseCmdLineInputs();

    //=========================================================================================================
    /**
     * This is executed when the user presses "close" button (via QConnection from MainWindow)
     */
    void shutdown();

    //=========================================================================================================
    /**
     * This initializes the EventSystem.
     */
    void initEventSystem();

    //=========================================================================================================
    /**
     * This initializes the PluginManager.
     */
    void initPluginManager();

    //=========================================================================================================
    /**
     * This initializes the MainWindow.
     */
    void initMainWindow();

    //=========================================================================================================
    /**
     * This function call qRegisterMetatype() on all custom types that are used in QObject::connect() calls.
     */
    void registerMetaTypes();

    //=========================================================================================================
    /**
     * Load plugins from plugin directory and then initializes their data.
     */
    void loadandInitPlugins();

    QSharedPointer<ANSHAREDLIB::PluginManager>      m_pPluginManager;       /**< Holds plugin manager. */
    QSharedPointer<ANSHAREDLIB::AnalyzeData>        m_pAnalyzeData;         /**< The global data base. */
    QPointer<MainWindow>                            m_pMainWindow;          /**< The main window. */
    QCommandLineParser                              m_cmdLineParser;        /**< The command line parser. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace MNEANALYZE

#endif // MNEANALYZE_ANALYZECORE_H
