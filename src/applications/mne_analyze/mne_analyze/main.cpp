//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2017
 * @brief    Implements the mne_analyze GUI application.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <stdio.h>
#include <utils/generics/mne_logger.h>

#include "info.h"
#include "analyzecore.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QFontDatabase>
#include <QtPlugin>
#include <QSurfaceFormat>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// MAIN
//=============================================================================================================

#ifdef STATICBUILD
Q_IMPORT_PLUGIN(DataLoader)
Q_IMPORT_PLUGIN(DataManager)
Q_IMPORT_PLUGIN(RawDataViewer)
Q_IMPORT_PLUGIN(Events)
Q_IMPORT_PLUGIN(Filtering)
Q_IMPORT_PLUGIN(Averaging)
Q_IMPORT_PLUGIN(SourceLocalization)
Q_IMPORT_PLUGIN(ControlManager)
Q_IMPORT_PLUGIN(ChannelSelection)
Q_IMPORT_PLUGIN(CoRegistration)
#ifdef MNE_DISP3D
    Q_IMPORT_PLUGIN(View3D)
#endif
#endif

//=============================================================================================================

int main(int argc, char *argv[])
{
    // When building a static version of MNE Analyze we have to init all resource (.qrc) files here manually
    #ifdef STATICBUILD
        #ifdef MNE_DISP3D
            // Q_INIT_RESOURCE(mne_disp3d);
            // Q_INIT_RESOURCE(analyze_view3d);
            // Q_INIT_RESOURCE(analyze_dipolefit);
            // Q_INIT_RESOURCE(analyze_coregistration);
            // Q_INIT_RESOURCE(analyze_sourcelocalization);
        #endif
        // Q_INIT_RESOURCE(analyze_averaging);
        // Q_INIT_RESOURCE(analyze_channelselection);
        // Q_INIT_RESOURCE(analyze_controlmanager);
        // Q_INIT_RESOURCE(analyze_dataloader);
        // Q_INIT_RESOURCE(analyze_datamanager);
        // Q_INIT_RESOURCE(analyze_events);
        // Q_INIT_RESOURCE(analyze_filtering);
        // Q_INIT_RESOURCE(analyze_rawdataviewer);
    #endif

    // Enable crisp fractional scaling on high-DPI monitors. Must be set before
    // the QApplication is constructed. (Qt6 enables high-DPI pixmaps by default.)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);

    //set application settings
    QCoreApplication::setOrganizationName(CInfo::OrganizationName());
    QCoreApplication::setApplicationName(CInfo::AppNameShort());
    QCoreApplication::setOrganizationDomain("www.mne-cpp.org");

    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    //New AnalyzeCore instance
    QScopedPointer<AnalyzeCore> pAnalyzeCore (new AnalyzeCore);
    pAnalyzeCore->showMainWindow();

    return app.exec();
}
