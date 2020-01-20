//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
 * @brief    Implements the mne_browse GUI application.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <stdio.h>
#include "Windows/mainwindow.h"
#include "Utils/info.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>
#include <QDateTime>
#include <QSplashScreen>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

MainWindow* mainWindow = NULL;


//*************************************************************************************************************

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);

    if(mainWindow) {
        switch (type) {
        case QtDebugMsg:
           txt += QString("{Debug} \t\t %1").arg(msg);
           mainWindow->writeToLog(txt,_LogKndMessage, _LogLvMax);
           break;
        case QtWarningMsg:
           txt += QString("{Warning} \t %1").arg(msg);
           mainWindow->writeToLog(txt,_LogKndWarning, _LogLvNormal);
           break;
        case QtCriticalMsg:
           txt += QString("{Critical} \t %1").arg(msg);
           mainWindow->writeToLog(txt,_LogKndError, _LogLvMin);
           break;
        case QtFatalMsg:
           txt += QString("{Fatal} \t\t %1").arg(msg);
           mainWindow->writeToLog(txt,_LogKndError, _LogLvMin);
           abort();
           break;
        }
    }
}


//=============================================================================================================
// MAIN
int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageHandler);
    QApplication a(argc, argv);

    //set application settings
    QCoreApplication::setOrganizationName(CInfo::OrganizationName());
    QCoreApplication::setApplicationName(CInfo::AppNameShort());

    //show splash screen for 1 second
    QPixmap pixmap(":/Resources/Images/splashscreen_mne_browse.png");
    QSplashScreen splash(pixmap);
    splash.show();
    QThread::sleep(1);

    mainWindow = new MainWindow();
    mainWindow->show();

    splash.finish(mainWindow);

    return a.exec();
}
