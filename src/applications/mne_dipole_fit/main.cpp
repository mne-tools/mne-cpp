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

#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_dipole_fit.h>

#include <utils/generics/mne_logger.h>

#include <mne/mne_bem.h>

#include <QFile>
#include <QFileInfo>

#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>
#include <disp3D/model/items/dipoletreeitem.h>

#include <fs/fs_label.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotationset.h>

#include <iostream>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace FSLIB;
using namespace MNELIB;
using namespace UTILSLIB;

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
int main(int argc, char *argv[])
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
    qInstallMessageHandler(MNELogger::customLogWriter);
    std::unique_ptr<QCoreApplication> app;
    if (guiRequested) {
        app = std::make_unique<QApplication>(argc, argv);
    } else {
        app = std::make_unique<QCoreApplication>(argc, argv);
    }
    QCoreApplication::setApplicationName("mne_dipole_fit");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    InvDipoleFitSettings settings(&argc,argv);
    if (settings.measname.isEmpty()
        || (settings.dipname.isEmpty() && settings.bdipname.isEmpty())
        || (!settings.include_meg && !settings.include_eeg)) {
        return 1;
    }
    if (!QFileInfo(settings.measname).isReadable()) {
        qCritical("Measurement data file is not readable: %s", settings.measname.toUtf8().constData());
        return 1;
    }

    InvDipoleFit dipFit(&settings);
    InvEcdSet set = dipFit.calculateFit();
    if (set.size() == 0) {
        qCritical("Dipole fitting failed: no dipoles were fitted.");
        return 1;
    }

    BrainView *pViewer = nullptr;
    if(settings.gui) {
        pViewer = new BrainView();
        BrainTreeModel *pModel = new BrainTreeModel();
        pModel->addDipoles(set);

        // Load brain surface if available
        if(!settings.bemname.isEmpty()) {
            QFile bemFile(settings.bemname);
            MNEBem bem(bemFile);
            for(int i = 0; i < bem.size(); ++i) {
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
    if (!settings.dipname.isEmpty() && !set.save_dipoles_dip(settings.dipname)) {
        qCritical("Dipoles could not be saved to %s.", settings.dipname.toUtf8().data());
        return 1;
    }
    if (!settings.bdipname.isEmpty() && !set.save_dipoles_bdip(settings.bdipname)) {
        qCritical("Dipoles could not be saved to %s.", settings.bdipname.toUtf8().data());
        return 1;
    }

    /*
     * Test - Reading again
     */
    if (!settings.dipname.isEmpty()) {
        InvEcdSet::read_dipoles_dip(settings.dipname);
    }

    return settings.gui ? app->exec() : 0;
}
