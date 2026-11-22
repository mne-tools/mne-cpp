//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December, 2016
 * @brief    Implements the mne_dipole_fit application.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/inv/dipole_fit/inv_dipole_fit_settings.h>
#include <mne/inv/dipole_fit/inv_dipole_fit.h>
#include <mne/base/log.h>
#include <mne/mne/mne_bem.h>
#include <mne/disp3D/view/brainview.h>
#include <mne/disp3D/model/braintreemodel.h>
#include <mne/disp3D/model/items/dipoletreeitem.h>
#include <mne/fs/fs_label.h>
#include <mne/fs/fs_surfaceset.h>
#include <mne/fs/fs_annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileInfo>
#include <QApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_dipole_fit application.
 * By default, main has the storage class extern.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
    // #ifdef STATICBUILD
    // Q_INIT_RESOURCE(mne_disp3d);
    // #endif

    bool guiRequested = false;
    for (int index = 1; index < argc; ++index) {
        if (qstrcmp(argv[index], "--gui") == 0) {
            guiRequested = true;
            break;
        }
    }
    std::unique_ptr<QCoreApplication> app;
    if (guiRequested) {
        app = std::make_unique<QApplication>(argc, argv);
    } else {
        app = std::make_unique<QCoreApplication>(argc, argv);
    }
    QCoreApplication::setApplicationName("mne_dipole_fit");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    InvDipoleFitSettings settings(&argc, argv);
    if (settings.measname.empty() || (!settings.include_meg && !settings.include_eeg)) {
        return 1;
    }
    if (settings.dipname.empty() && settings.bdipname.empty()) {
        Log::critical("Output file name missing. Please use the --dip or --bdip options to do this.");
        return 1;
    }
    if (!QFileInfo(QString::fromStdString(settings.measname)).isReadable()) {
        Log::critical("Measurement data file is not readable: {}", settings.measname);
        return 1;
    }

    InvDipoleFit dipFit(&settings);
    InvEcdSet set = dipFit.calculateFit();
    if (set.size() == 0) {
        Log::critical("Dipole fitting failed: no dipoles were fitted.");
        return 1;
    }

    BrainView* pViewer = nullptr;
    if (settings.gui) {
        pViewer = new BrainView();
        BrainTreeModel* pModel = new BrainTreeModel();
        pModel->addDipoles(set);

        // Load brain surface if available
        if (!settings.bemname.empty()) {
            MNEBem bem(mne::path_from_utf8(settings.bemname));
            for (std::size_t i = 0; i < bem.size(); ++i) {
                pModel->addBemSurface("Subject", "BEM", bem[i]);
            }
        }

        pViewer->setModel(pModel);
        pViewer->resize(800, 600);
        pViewer->show();
    }

    /*
     * Saving...
     */
    if (!settings.dipname.empty() && !set.save_dipoles_dip(mne::path_from_utf8(settings.dipname))) {
        Log::critical("Dipoles could not be saved to {}.", settings.dipname);
        return 1;
    }
    if (!settings.bdipname.empty() && !set.save_dipoles_bdip(mne::path_from_utf8(settings.bdipname))) {
        Log::critical("Dipoles could not be saved to {}.", settings.bdipname);
        return 1;
    }

    return settings.gui ? app->exec() : 0;
}
