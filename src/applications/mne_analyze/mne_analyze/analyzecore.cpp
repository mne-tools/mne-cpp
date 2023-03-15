//=============================================================================================================
/**
 * @file     analyzecore.cpp
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
 * @brief    AnalyzeCore class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <functional>
#include "analyzecore.h"
#include "mainwindow.h"
#include "../libs/anShared/Plugins/abstractplugin.h"
#include "../libs/anShared/Management/analyzedata.h"
#include "../libs/anShared/Management/pluginmanager.h"
#include "../libs/anShared/Management/eventmanager.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;

//=============================================================================================================
// CONST
//=============================================================================================================

const char* pluginsDir = "/mne_analyze_plugins";        /**< Holds path to the plugins.*/

//=============================================================================================================
// GLOBAL DEFINES
//=============================================================================================================

QPointer<MainWindow> appMainWindowHandler(Q_NULLPTR);          /**< The main window. */

//=============================================================================================================
/**
 * Custom Qt message handler.
 *
 * @param[in] type      enum to identify the various message types.
 * @param[in] context   additional information about a log message.
 * @param[in] msg       the message to log.
 */
void customMessageHandler(QtMsgType type, const
                          QMessageLogContext &context,
                          const QString &msg)
{
    appMainWindowHandler->writeToLog(type, context, msg);
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeCore::AnalyzeCore(QObject* parent)
: QObject(parent)
, m_pPluginManager(Q_NULLPTR)
, m_pAnalyzeData(Q_NULLPTR)
, m_pMainWindow(Q_NULLPTR)
{    
    // Initialize cmd line parser
    initCmdLineParser();

    //show splash screen for 1 second
    QPixmap pixmap(":/images/splashscreen_mne_analyze.png");
    QSplashScreen splash(pixmap);
    splash.show();

    registerMetaTypes();

    initGlobalData();

    initEventSystem();
    initPluginManager();
    initMainWindow();

    splash.hide();

    // Setup log window
    appMainWindowHandler = m_pMainWindow;
    qInstallMessageHandler(customMessageHandler);

    // Initialize cmd line inputs if provided by the user
    parseCmdLineInputs();
}

//=============================================================================================================

AnalyzeCore::~AnalyzeCore()
{

}

//=============================================================================================================

void AnalyzeCore::initCmdLineParser()
{
    m_cmdLineParser.setApplicationDescription("MNE Analyze: Browse, Process & Visualize. For more information and documentation please visit www.mne-cpp.org.");
    m_cmdLineParser.addHelpOption();

    QCommandLineOption inFileOpt(QStringList() << "f" << "file",
                                 QCoreApplication::translate("main","File to load."),
                                 QCoreApplication::translate("main","filePath"));

    m_cmdLineParser.addOption(inFileOpt);

    m_cmdLineParser.process(QApplication::arguments());
}

//=============================================================================================================

void AnalyzeCore::parseCmdLineInputs()
{
    if(!m_pPluginManager) {
        return;
    }

    if(m_cmdLineParser.isSet("file")) {
        for(int i = 0; i < m_pPluginManager->getPlugins().size(); i++) {
            if(m_pPluginManager->getPlugins().at(i)->getName() == "Data Loader") {
                m_pPluginManager->getPlugins().at(i)->cmdLineStartup(QStringList() << "file" << m_cmdLineParser.value("file"));
            }
        }
    }
}

//=============================================================================================================

void AnalyzeCore::showMainWindow()
{
    m_pMainWindow->show();
}

//=============================================================================================================

void AnalyzeCore::initGlobalData()
{
    m_pAnalyzeData = AnalyzeData::SPtr::create();
}

//=============================================================================================================

void AnalyzeCore::initEventSystem()
{
    EventManager::startEventHandling();
}

//=============================================================================================================

void AnalyzeCore::initPluginManager()
{
    m_pPluginManager = QSharedPointer<PluginManager>::create();
    loadandInitPlugins();
}

//=============================================================================================================

void AnalyzeCore::loadandInitPlugins()
{
    m_pPluginManager->loadPluginsFromDirectory(qApp->applicationDirPath() + pluginsDir);
    m_pPluginManager->initPlugins(m_pAnalyzeData);
}

//=============================================================================================================

QVector<ANSHAREDLIB::AbstractPlugin*> AnalyzeCore::getLoadedPlugins()
{
    return m_pPluginManager->getPlugins();
}

//=============================================================================================================

bool AnalyzeCore::pluginsInitialized() const
{
    return !m_pPluginManager.isNull();
}

//=============================================================================================================

void AnalyzeCore::initMainWindow()
{
    m_pMainWindow = new MainWindow(this);
    QObject::connect(m_pMainWindow.data(), &MainWindow::mainWindowClosed,
                     this, &AnalyzeCore::shutdown);
    QObject::connect(m_pMainWindow.data(), &MainWindow::reloadPlugins,
                     this, &AnalyzeCore::reloadPlugins);
}

//=============================================================================================================

void AnalyzeCore::registerMetaTypes()
{
    qRegisterMetaType<QSharedPointer<Event>>("QSharedPointer<Event>");
}

//=============================================================================================================

void AnalyzeCore::shutdown()
{
    EventManager::shutdown();

    // shutdown every plugin, empty analzye data etc.
    m_pPluginManager->shutdown();
}

//=============================================================================================================

void AnalyzeCore::reloadPlugins()
{
    loadandInitPlugins();
    m_pMainWindow->updatePluginsUI();
}
