//=============================================================================================================
/**
 * @file     analyzecore.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzecore.h"
#include "mainwindow.h"
#include "../libs/anShared/Interfaces/IExtension.h"
#include "../libs/anShared/Management/analyzesettings.h"
#include "../libs/anShared/Management/analyzedata.h"
#include "../libs/anShared/Management/extensionmanager.h"
#include "../libs/anShared/Management/eventmanager.h"

#include<iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// CONST
//=============================================================================================================

const char* extensionsDir = "/mne_analyze_extensions";        /**< holds path to the extensions.*/


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeCore::AnalyzeCore(QObject* parent)
    : QObject(parent)
{
    registerMetaTypes();

    initGlobalSettings();
    initGlobalData();

    initEventSystem();
    initExtensionManager();
    initMainWindow();
}


//*************************************************************************************************************

AnalyzeCore::~AnalyzeCore()
{

}


//*************************************************************************************************************

void AnalyzeCore::showMainWindow()
{
    m_pMainWindow->show();
}


//*************************************************************************************************************

void AnalyzeCore::initGlobalSettings()
{
    m_analyzeSettings = AnalyzeSettings::SPtr::create();
}


//*************************************************************************************************************

void AnalyzeCore::initGlobalData()
{
    m_analyzeData = AnalyzeData::SPtr::create();
}


//*************************************************************************************************************

void AnalyzeCore::initEventSystem()
{
    EventManager::getEventManager().startEventHandling();
}


//*************************************************************************************************************

void AnalyzeCore::initExtensionManager()
{
    m_pExtensionManager = QSharedPointer<ExtensionManager>::create();
    m_pExtensionManager->loadExtensionsFromDirectory(qApp->applicationDirPath() +  extensionsDir);
    m_pExtensionManager->initExtensions(m_analyzeSettings, m_analyzeData);
}


//*************************************************************************************************************

void AnalyzeCore::initMainWindow()
{
    m_pMainWindow = new MainWindow(m_pExtensionManager);
    QObject::connect(m_pMainWindow.data(), &MainWindow::mainWindowClosed,
                     this, &AnalyzeCore::onMainWindowClosed);
}


//*************************************************************************************************************

void AnalyzeCore::registerMetaTypes()
{
    qRegisterMetaType<QSharedPointer<Event>>("QSharedPointer<Event>");
    qRegisterMetaType<QSharedPointer<Event>>("ChannelData");
}


//*************************************************************************************************************

void AnalyzeCore::onMainWindowClosed()
{
    EventManager::getEventManager().shutdown();
    // shutdown every extension, empty analzye data etc.
    m_pExtensionManager->shutdown();
}
