//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     analyzecore.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2018
 * @brief     AnalyzeCore class declaration.
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

class QSplashScreen;

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
     * Initializes the plugin manager.
     *
     * @param[in] pSplash   Pointer to the splash screen.
     */
    void initPluginManager(QSplashScreen* pSplash = Q_NULLPTR);

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
