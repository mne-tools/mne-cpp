//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
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
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QSplashScreen>
#include <QThread>
#include <QTimer>


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

    if (!mainWindow)
        return;

    QString dt  = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);

    LogKind  kind  = _LogKndMessage;
    LogLevel level = _LogLvMax;
    bool doAbort   = false;

    switch (type) {
    case QtDebugMsg:
        txt  += QString("{Debug} \t\t %1").arg(msg);
        kind  = _LogKndMessage; level = _LogLvMax;
        break;
    case QtWarningMsg:
        txt  += QString("{Warning} \t %1").arg(msg);
        kind  = _LogKndWarning; level = _LogLvNormal;
        break;
    case QtCriticalMsg:
        txt  += QString("{Critical} \t %1").arg(msg);
        kind  = _LogKndError;   level = _LogLvMin;
        break;
    case QtFatalMsg:
        txt  += QString("{Fatal} \t\t %1").arg(msg);
        kind  = _LogKndError;   level = _LogLvMin;
        doAbort = true;
        break;
    default:
        return;
    }

    // writeToLog() modifies a QTextEdit — safe only from the GUI thread.
    // Messages emitted on background threads are queued to the main thread.
    if (QThread::currentThread() == qApp->thread()) {
        mainWindow->writeToLog(txt, kind, level);
    } else {
        QString capturedTxt   = txt;
        LogKind  capturedKind  = kind;
        LogLevel capturedLevel = level;
        QMetaObject::invokeMethod(mainWindow, [capturedTxt, capturedKind, capturedLevel]() {
            mainWindow->writeToLog(capturedTxt, capturedKind, capturedLevel);
        }, Qt::QueuedConnection);
    }

    if (doAbort)
        abort();
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
    QCoreApplication::setApplicationVersion(CInfo::AppVersion());

    // -------------------------------------------------------------------------
    // Command-line argument parsing
    // -------------------------------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "MNE Browse — raw FIFF data viewer and processor.\n"
        "Qt equivalent of the MNE C tool mne_browse_raw.");
    parser.addHelpOption();
    parser.addVersionOption();

    // --cd <dir>  Change the initial working directory
    QCommandLineOption cdOption("cd",
        "Change the initial working directory to <dir>.",
        "dir");
    parser.addOption(cdOption);

    // --raw <file>  Open a raw FIFF file on startup
    QCommandLineOption rawOption("raw",
        "Raw FIFF input file to open on startup.",
        "file");
    parser.addOption(rawOption);

    // --events <file>  Load an event/trigger file on startup
    QCommandLineOption eventsOption("events",
        "FIFF event file (*-eve.fif) to load alongside the raw data.",
        "file");
    parser.addOption(eventsOption);

    // --highpass <Hz>  High-pass corner frequency
    QCommandLineOption highpassOption(QStringList() << "highpass" << "hp",
        "High-pass filter corner frequency in Hz.",
        "Hz");
    parser.addOption(highpassOption);

    // --lowpass <Hz>  Low-pass corner frequency
    QCommandLineOption lowpassOption(QStringList() << "lowpass" << "lp",
        "Low-pass filter corner frequency in Hz.",
        "Hz");
    parser.addOption(lowpassOption);

    parser.process(a);

    // Apply --cd before anything else
    if(parser.isSet(cdOption)) {
        const QString dir = parser.value(cdOption);
        if(!QDir::setCurrent(dir)) {
            qWarning("[mne_browse] --cd: could not change to directory: %s",
                     dir.toUtf8().data());
        }
    }

    // Collect remaining options to hand to the window after it is shown
    const QString rawFile    = parser.isSet(rawOption)      ? parser.value(rawOption)    : QString();
    const QString eventsFile = parser.isSet(eventsOption)   ? parser.value(eventsOption) : QString();

    bool hpOk = false, lpOk = false;
    double highpass = parser.isSet(highpassOption) ? parser.value(highpassOption).toDouble(&hpOk) : -1.0;
    double lowpass  = parser.isSet(lowpassOption)  ? parser.value(lowpassOption).toDouble(&lpOk)  : -1.0;
    if(parser.isSet(highpassOption) && !hpOk) {
        qWarning("[mne_browse] --highpass: invalid value '%s', ignoring.",
                 parser.value(highpassOption).toUtf8().data());
        highpass = -1.0;
    }
    if(parser.isSet(lowpassOption) && !lpOk) {
        qWarning("[mne_browse] --lowpass: invalid value '%s', ignoring.",
                 parser.value(lowpassOption).toUtf8().data());
        lowpass = -1.0;
    }

    // -------------------------------------------------------------------------
    // Launch main window
    // -------------------------------------------------------------------------
    //show splash screen for 1 second
    QPixmap pixmap(":/Resources/Images/splashscreen_mne_browse.png");
    QSplashScreen splash(pixmap);
    splash.show();
    QThread::sleep(1);

    mainWindow = new MainWindow();
    mainWindow->show();

    splash.finish(mainWindow);

    // Defer CLI option application until after the first event-loop iteration.
    // This ensures the window is fully laid out (widgets have non-zero sizes) before
    // ChannelDataView::updateSamplesPerPixel() is called, which needs a valid width.
    if(!rawFile.isEmpty() || highpass >= 0.0 || lowpass >= 0.0) {
        MainWindow *win = mainWindow;
        QTimer::singleShot(0, win, [win, rawFile, eventsFile, highpass, lowpass]() {
            win->applyCommandLineOptions(rawFile, eventsFile, highpass, lowpass);
        });
    }

    return a.exec();
}
