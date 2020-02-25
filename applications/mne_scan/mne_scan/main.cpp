//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Implements the main() application function.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainsplashscreen.h"
#include "mainwindow.h"

#include <scMeas/measurementtypes.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/numeric.h>

#include <scShared/Management/pluginconnectorconnection.h>
#include <scShared/Management/pluginoutputdata.h>
#include <scShared/Management/plugininputdata.h>
#include <scShared/Interfaces/IPlugin.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>
#include <QSharedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace MNESCAN;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// GLOBAL DEFINTIONS
//=============================================================================================================

QSharedPointer<MainWindow> mainWin;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    #ifdef STATICLIB
    Q_INIT_RESOURCE(disp3d);
    Q_INIT_RESOURCE(mne);
    Q_INIT_RESOURCE(averaging);
    Q_INIT_RESOURCE(babymeg);
    Q_INIT_RESOURCE(covariance);
    Q_INIT_RESOURCE(ecgsimulator);
    Q_INIT_RESOURCE(fiffsimulator);
    Q_INIT_RESOURCE(neuromag);
    Q_INIT_RESOURCE(noisereduction);
    Q_INIT_RESOURCE(rtcmusic);
    Q_INIT_RESOURCE(reference);
    Q_INIT_RESOURCE(ssvepbci);
    Q_INIT_RESOURCE(scDisp);
    #endif

    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication app(argc, argv);

    //Store application info to use QSettings
    QCoreApplication::setOrganizationName("MNE-CPP");
    QCoreApplication::setOrganizationDomain("www.mne-cpp.org");
    QCoreApplication::setApplicationName(CInfo::AppNameShort());

    SCMEASLIB::MeasurementTypes::registerTypes();

    QPixmap pixmap(":/images/splashscreen.png");
    MainSplashScreen::SPtr splashscreen(new MainSplashScreen(pixmap));
    splashscreen->show();

    //ToDo Debug Some waiting stuff to see splash screen -> remove this in final release
    int time = 100;
    for(int i=0; i < time;++i)
    {
        int p = (i*100)/time;
        splashscreen->showMessage("Loading modules.."+ QString::number(p)+"%");
    }

    mainWin = QSharedPointer<MainWindow>(new MainWindow);
    mainWin->show();

    splashscreen->finish(mainWin.data());

    QSurfaceFormat fmt;
    fmt.setSamples(10);
    QSurfaceFormat::setDefaultFormat(fmt);

    return app.exec();
}
