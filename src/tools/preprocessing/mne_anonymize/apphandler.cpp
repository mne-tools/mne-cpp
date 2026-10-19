//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     apphandler.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2020
 * @brief    AppHandler class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "apphandler.h"
#include "settingscontrollercl.h"
#include "settingscontrollergui.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AppHandler::AppHandler()
: m_bGuiMode(true)
{
}

//=============================================================================================================

std::unique_ptr<QCoreApplication> AppHandler::createApplication(int& argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "--no-gui")) {
            m_bGuiMode = false;
            return std::make_unique<QCoreApplication>(argc, argv);
        }
    }
    return std::make_unique<QApplication>(argc, argv);
}

//=============================================================================================================

std::unique_ptr<SettingsControllerCl> AppHandler::createController(const QStringList& args)
{
    if (m_bGuiMode) {
        return std::make_unique<SettingsControllerGui>(args);
    } else {
        return std::make_unique<SettingsControllerCl>(args);
    }
}
