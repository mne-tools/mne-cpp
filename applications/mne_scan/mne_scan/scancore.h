//=============================================================================================================
/**
 * @file     scancore.h
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
 * @brief    Declaration of the ScanCore class.
 *
 */

#ifndef SCANCORE_H
#define SCANCORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"

#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/pluginscenemanager.h>

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// NAMESPACE
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================

class ScanCore : public QObject
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
     * Constructs a ScanCore object. Main object that controls MNE Scan.
     */
    explicit ScanCore(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Attempts to start current workflow. Returns wheteher successful.
     */
    bool startMeasurement();

    //=========================================================================================================
    /**
     * Attempts to stop current workflow. Returns wheteher successful.
     */
    bool stopMeasurement();

private:
    //=========================================================================================================
    /**
     * Registers types with Qt for use in QVariant and signals/slots.
     */
    void registerQtMetaTypes();

    //=========================================================================================================
    /**
     * Inititlaizes plguins management classes and loads plugins.
     */
    void initPlugins();

    //=========================================================================================================
    /**
     * Initializes GUI elements.
     */
    void initGUI();

    bool m_bGuiMode;                                /**< Whether to use a GUI. */
    std::unique_ptr<MainWindow> m_pMainWindow;      /**< GUI main window. */

    std::shared_ptr<SCSHAREDLIB::PluginManager>         m_pPluginManager;           /**< Loads and holds plugins. */
    std::shared_ptr<SCSHAREDLIB::PluginSceneManager>    m_pPluginSceneManager;      /**< Stores selected and running plugins */

};
}//namespace
#endif // SCANCORE_H
