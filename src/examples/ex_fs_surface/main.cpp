//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Example of an FreeSurfer Surface application
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/viewers/abstractview.h>
#include <disp3D/engine/model/data3Dtreemodel.h>

#include <fs/surfaceset.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace DISP3DLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

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
    #ifdef STATICBUILD
    // Q_INIT_RESOURCE(mne_disp3d);
    #endif
    
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Fs Surface Example");
    parser.addHelpOption();

    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");

    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);

    parser.process(a);

    int hemi = parser.value(hemiOption).toInt();
    QString subject = parser.value(subjectOption);
    QString subjectPath = parser.value(subjectPathOption);

    //
    // pial
    //
    SurfaceSet tSurfSetPial (subject, hemi, "pial", subjectPath);

    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();

    p3DDataModel->addSurfaceSet(subject, "pial", tSurfSetPial);

    //
    // inflated
    //
    SurfaceSet tSurfSetInflated (subject, hemi, "inflated", subjectPath);
    p3DDataModel->addSurfaceSet(subject, "inflated", tSurfSetInflated);

    //
    // orig
    //
    SurfaceSet tSurfSetOrig (subject, hemi, "orig", subjectPath);
    p3DDataModel->addSurfaceSet(subject, "orig", tSurfSetOrig);

    //
    // white
    //
    SurfaceSet tSurfSetWhite (subject, hemi, "white", subjectPath);
    p3DDataModel->addSurfaceSet(subject, "white", tSurfSetWhite);

    p3DAbstractView->show();

    return a.exec();
}
