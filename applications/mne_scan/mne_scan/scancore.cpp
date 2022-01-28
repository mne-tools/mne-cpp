//=============================================================================================================
/**
 * @file     scancore.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.9
 * @date     January, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the ScanCore class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scancore.h"
#include "mainwindow.h"

#include <scMeas/measurementtypes.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

namespace MNESCAN {

//=============================================================================================================
// CONST
//=============================================================================================================

const QString pluginDir = "/mne_scan_plugins";          /**< holds path to plugins.*/

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScanCore::ScanCore(QObject *parent)
: QObject(parent)
, m_bGuiMode(true)
{
    registerMetatypes();

    initPlugins();

    if(m_bGuiMode){
        initGUI();
    }
}

//=============================================================================================================

void ScanCore::registerMetatypes()
{
    SCMEASLIB::MeasurementTypes::registerTypes();
}

//=============================================================================================================

void ScanCore::initPlugins()
{
    m_pPluginManager = std::make_shared<SCSHAREDLIB::PluginManager>();
    m_pPluginManager->loadPlugins(qApp->applicationDirPath() + pluginDir);
    m_pPluginSceneManager = std::make_shared<SCSHAREDLIB::PluginSceneManager>();
}

//=============================================================================================================

void ScanCore::initGUI()
{
    m_pMainWindow = std::make_unique<MainWindow>(this);
    m_pMainWindow->setupPlugins(m_pPluginManager, m_pPluginSceneManager);
    m_pMainWindow->setupUI();
}

//=============================================================================================================

bool ScanCore::startMeasurement()
{
    if(!m_pPluginSceneManager->startPlugins()) {
        m_pPluginSceneManager->stopPlugins();
        return false;
    }
    return true;
}

//=============================================================================================================

bool ScanCore::stopMeasurement()
{
    m_pPluginSceneManager->stopPlugins();
    return true;
}

} //namespace
