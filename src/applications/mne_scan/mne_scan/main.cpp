//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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
#include <scShared/Plugins/abstractplugin.h>

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

//=============================================================================================================
// GLOBAL DEFINTIONS
//=============================================================================================================

//=============================================================================================================
// MAIN
//=============================================================================================================

#ifdef STATICBUILD
Q_IMPORT_PLUGIN(BabyMEG)
Q_IMPORT_PLUGIN(FiffSimulator)
Q_IMPORT_PLUGIN(Natus)
Q_IMPORT_PLUGIN(Covariance)
Q_IMPORT_PLUGIN(NoiseReduction)
Q_IMPORT_PLUGIN(RtcMne)
Q_IMPORT_PLUGIN(Averaging)
Q_IMPORT_PLUGIN(NeuronalConnectivity)
Q_IMPORT_PLUGIN(FtBuffer)
Q_IMPORT_PLUGIN(WriteToFile)
Q_IMPORT_PLUGIN(Hpi)
// Q_IMPORT_PLUGIN(Fieldline)
//Q_IMPORT_PLUGIN(DummyToolbox)
#ifdef WITHGUSBAMP
Q_IMPORT_PLUGIN(GUSBAmp)
#endif
#ifdef WITHBRAINAMP
Q_IMPORT_PLUGIN(BrainAMP)
#endif
#ifdef WITHEEGOSPORTS
Q_IMPORT_PLUGIN(EEGoSports)
#endif
#ifdef WITHLSL
Q_IMPORT_PLUGIN(LSLAdapter)
#endif
#ifdef WITHTMSI
Q_IMPORT_PLUGIN(TMSI)
#endif
#ifdef WITHBRAINFLOW
Q_IMPORT_PLUGIN(BrainFlowBoard)
#endif
#endif

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    // When building a static version of MNE Scan we have to init all resource (.qrc) files here manually
    #ifdef STATICBUILD
    Q_INIT_RESOURCE(babymeg);
    Q_INIT_RESOURCE(fiffsimulator);
    Q_INIT_RESOURCE(covariance);
    Q_INIT_RESOURCE(noisereduction);
    Q_INIT_RESOURCE(rtcmne);
    Q_INIT_RESOURCE(averaging);
    Q_INIT_RESOURCE(writetofile);
    Q_INIT_RESOURCE(hpi);
    // Q_INIT_RESOURCE(fieldline);
    #ifdef WITHBRAINAMP
    Q_INIT_RESOURCE(brainamp);
    #endif
    #ifdef WITHEEGOSPORTS
    Q_INIT_RESOURCE(eegosports);
    #endif
    #ifdef WITHGUSBAMP
    Q_INIT_RESOURCE(gusbamp);
    #endif
    #ifdef WITHTMSI
    Q_INIT_RESOURCE(tmsi);
    #endif
    #endif

    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QApplication app(argc, argv);
    //app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);

    // Set default font
    int id = QFontDatabase::addApplicationFont(":/fonts/Roboto-Light.ttf");

    if(id != -1){
        app.setFont(QFont(QFontDatabase::applicationFontFamilies(id).at(0)));
    }

    //Store application info to use QSettings
    QCoreApplication::setOrganizationName(CInfo::OrganizationName());
    QCoreApplication::setApplicationName(CInfo::AppNameShort());
    QCoreApplication::setOrganizationDomain("www.mne-cpp.org");

    SCMEASLIB::MeasurementTypes::registerTypes();

    MainWindow mainWin;

    QSurfaceFormat fmt;
    fmt.setSamples(10);
    QSurfaceFormat::setDefaultFormat(fmt);

    int returnValue(app.exec());

    return returnValue;
}
