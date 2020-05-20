//=============================================================================================================
/**
 * @file     analyzecore.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
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

#include "analyzecore.h"
#include "mainwindow.h"
#include "../libs/anShared/Interfaces/IPlugin.h"
#include "../libs/anShared/Management/analyzesettings.h"
#include "../libs/anShared/Management/analyzedata.h"
#include "../libs/anShared/Management/pluginmanager.h"
#include "../libs/anShared/Management/eventmanager.h"

#include<iostream>

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

const char* pluginsDir = "/mne_analyze_plugins";        /**< holds path to the plugins.*/

//=============================================================================================================
// GLOBAL DEFINES
//=============================================================================================================

QPointer<MainWindow> m_pMainWindow;          /**< The main window. */

//=============================================================================================================
/**
 * Custom Qt message handler.
 *
 * @param [in] type      enum to identify the various message types
 * @param [in] context   additional information about a log message
 * @param [in] msg       the message to log
 */
void customMessageHandler(QtMsgType type, const
                          QMessageLogContext &context,
                          const QString &msg)
{
    m_pMainWindow->writeToLog(type, context, msg);
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeCore::AnalyzeCore(QObject* parent)
: QObject(parent)
{    
    //show splash screen for 1 second
    QPixmap pixmap(":/images/splashscreen_mne_analyze.png");
    QSplashScreen splash(pixmap);
    splash.show();

    registerMetaTypes();

    initGlobalSettings();
    initGlobalData();

    initEventSystem();
    initPluginManager();
    initMainWindow();

    splash.hide();

    // Setup log window
    //qInstallMessageHandler(customMessageHandler);
}

//=============================================================================================================

AnalyzeCore::~AnalyzeCore()
{

}

//=============================================================================================================

void AnalyzeCore::showMainWindow()
{
    m_pMainWindow->show();
}

//=============================================================================================================

void AnalyzeCore::initGlobalSettings()
{
    m_analyzeSettings = AnalyzeSettings::SPtr::create();
}

//=============================================================================================================

void AnalyzeCore::initGlobalData()
{
    m_analyzeData = AnalyzeData::SPtr::create();
}

//=============================================================================================================

void AnalyzeCore::initEventSystem()
{
    EventManager::getEventManager().startEventHandling();
}

//=============================================================================================================

void AnalyzeCore::initPluginManager()
{
    m_pPluginManager = QSharedPointer<PluginManager>::create();
    m_pPluginManager->loadPluginsFromDirectory(qApp->applicationDirPath() +  pluginsDir);
    m_pPluginManager->initPlugins(m_analyzeSettings, m_analyzeData);
}

//=============================================================================================================

void AnalyzeCore::initMainWindow()
{
    m_pMainWindow = new MainWindow(m_pPluginManager);
    QObject::connect(m_pMainWindow.data(), &MainWindow::mainWindowClosed,
                     this, &AnalyzeCore::onMainWindowClosed);
}

//=============================================================================================================

void AnalyzeCore::registerMetaTypes()
{
    qRegisterMetaType<QSharedPointer<Event>>("QSharedPointer<Event>");
    qRegisterMetaType<QSharedPointer<Event>>("ChannelData");
}

//=============================================================================================================

void AnalyzeCore::onMainWindowClosed()
{
    EventManager::getEventManager().shutdown();
    // shutdown every plugin, empty analzye data etc.
    m_pPluginManager->shutdown();
}
