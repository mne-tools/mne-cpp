//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Graphical user interface of mne_anonymize, which removes personal health information from a fiff file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_anonymize.h"
#include "settingscontrollergui.h"
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <string>
#include <vector>

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * Starts the GUI. The options of mne_anonymize preset the GUI's fields.
 *
 * @param[in] argc   Argument count.
 * @param[in] argv   Arguments.
 *
 * @return The exit code of the event loop, or 1 if --no-gui is given.
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(APPLICATION_ORG);
    app.setApplicationName(APPLICATION_NAME);
    app.setApplicationVersion(APPLICATION_VERSION);

    std::vector<std::string> arguments;
    for (const QString& argument : app.arguments()) {
        arguments.push_back(argument.toStdString());
    }

    if (std::find(arguments.begin() + 1, arguments.end(), "--no-gui") != arguments.end()) {
        mne::Log::critical("mne_anonymize_gui is the graphical user interface; for the command-line version run mne_anonymize.");
        return 1;
    }

    MNEANONYMIZE::SettingsControllerGui controller(arguments);

    return app.exec();
}
