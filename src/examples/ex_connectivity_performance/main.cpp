//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Example of using the MNE-CPP Connectivity library
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <connectivity/metrics/abstractmetric.h>

#include <fiff/fiff_raw_data.h>

#include <utils/ioutils.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <stdio.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QHostInfo>
#include <QElapsedTimer>
#include <QThreadPool>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;
using namespace MNELIB;

//=============================================================================================================
// Global Defines
//=============================================================================================================

QString m_sCurrentDir;
int m_iCurrentIteration = 0;
int m_iNumberTrials;
int m_iNumberChannels;
int m_iNumberSamples;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt;// = QString("[%1] ").arg(dt);

    bool writeToLog = false;

    switch (type) {
        case QtWarningMsg:
            //txt += QString("{Warning} \t %1").arg(msg);
            txt += QString("%1").arg(msg);
            writeToLog=true;
            break;
    }

    if(!writeToLog) {
        return;
    }

    QString sFileName = m_sCurrentDir + "/" + QString::number(m_iNumberChannels) + "_" + QString::number(m_iNumberSamples) + "_" + QString::number(m_iNumberTrials) + ".log";

    QFile outFile(sFileName);

    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream textStream(&outFile);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        using Qt::endl;
#endif
        textStream << txt << endl;
    } else {
        qDebug() << "Unable to read data from file";
    }
}

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

    qInstallMessageHandler(customMessageHandler);

//    printf("globalInstance()->maxThreadCount(): %d\n",QThreadPool::globalInstance()->maxThreadCount());
//    QThreadPool::globalInstance()->setMaxThreadCount(24);

    //Parameters for performance test
    QStringList sConnectivityMethodList = QStringList() << "COR" << "XCOR" << "COH" << "IMAGCOH" << "PLI" << "WPLI" << "USPLI" << "DSWPLI" << "PLV";
    QList<int> lNumberTrials = QList<int>() << 1 << 5 << 10 << 20 << 50 << 100 << 200;
    QList<int> lNumberChannels = QList<int>() << 32 << 64 << 128 << 256;
    QList<int> lNumberSamples = QList<int>() << 100 << 200 << 300 << 400 << 500 << 600 << 700 << 800 << 900 << 1000 << 2000 << 3000 << 4000 << 5000 << 6000 << 7000 << 8000 << 9000 << 10000 << 20000 << 30000 << 40000 << 50000 << 60000 << 70000 << 80000 << 90000 << 100000;

    int iNumberRepeats = 5;
    int iStorageModeActive = 0;

    AbstractMetric::m_bStorageModeIsActive = iStorageModeActive;
    AbstractMetric::m_iNumberBinStart = 8;
    AbstractMetric::m_iNumberBinAmount = 4;

    // Create sensor level data
    QElapsedTimer timer;
    qint64 iTime = 0;
    timer.start();

    QString sRaw = QCoreApplication::applicationDirPath() + "../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
    MatrixXd matDataOrig, matData;
    MatrixXd times;
    QFile t_fileRaw(sRaw);
    FiffRawData raw(t_fileRaw);

    bool readSuccessful = false;

    readSuccessful = raw.read_raw_segment(matDataOrig, times, raw.first_samp, raw.first_samp+100001);

    if (!readSuccessful) {
        qDebug() << "Could not read raw segment.";
    }

    //Perform connectivity performance tests
    Connectivity connectivityObj;

    ConnectivitySettings connectivitySettings;
    connectivitySettings.setSamplingFrequency(raw.info.sfreq);
    connectivitySettings.setWindowType("hanning");

    QMap<int, QMap<int, MatrixXd > > matInputData;

    for(int j = 0; j < lNumberChannels.size(); ++j) {
        QMap<int, MatrixXd > mapChannelsSamples;

        for(int i = 0; i < lNumberSamples.size(); ++i) {
            mapChannelsSamples[lNumberSamples.at(i)] = matDataOrig.block(0,0,lNumberChannels.at(j), lNumberSamples.at(i));
        }

        matInputData[lNumberChannels.at(j)] = mapChannelsSamples;
    }

    iTime = timer.elapsed();
    qDebug() << "Prepare data " << iTime;
    timer.restart();

    qWarning() << "matInputData[256][40000].first().rows()" << matInputData[256][40000].rows();
    qWarning() << "matInputData[256][40000].first().cols()" << matInputData[256][40000].cols();
    qWarning() << "matInputData[32][500].rows()" << matInputData[32][500].rows();
    qWarning() << "matInputData[32][500].cols()" << matInputData[32][500].cols();
    qWarning() << "matInputData[64][100000].rows()" << matInputData[64][100000].rows();
    qWarning() << "matInputData[64][100000].cols()" << matInputData[64][100000].cols();

    for(int j = 0; j < lNumberSamples.size(); ++j) {
        for(int k = 0; k < lNumberChannels.size(); ++k) {
            connectivitySettings.clearAllData();

            matData = matInputData[lNumberChannels.at(k)][lNumberSamples.at(j)];

            RowVectorXi picks = RowVectorXi::LinSpaced(lNumberChannels.at(k),1,lNumberChannels.at(k)+1);
            connectivitySettings.setNodePositions(raw.info, picks);

            for(int l = 0; l < lNumberTrials.size(); ++l) {
                //Create data to work on

                while(connectivitySettings.size() < lNumberTrials.at(l)) {
                    connectivitySettings.append(matData);
                }

                for(int i = 0; i < sConnectivityMethodList.size(); ++i) {
                    m_iNumberTrials = lNumberTrials.at(l);
                    m_iNumberChannels = lNumberChannels.at(k);
                    m_iNumberSamples = lNumberSamples.at(j);

                    //Create new folder
                    m_sCurrentDir = QCoreApplication::applicationDirPath() + QString("./connectivity_performance_%1_%2_%3/%4/%5_%6_%7").arg(QHostInfo::localHostName()).arg(AbstractMetric::m_iNumberBinAmount).arg(iStorageModeActive).arg(sConnectivityMethodList.at(i)).arg(QString::number(lNumberChannels.at(k))).arg(QString::number(lNumberSamples.at(j))).arg(QString::number(lNumberTrials.at(l)));
                    QDir().mkpath(m_sCurrentDir);

                    //Write basic information to file
                    qWarning() << "sConnectivityMethod" << sConnectivityMethodList.at(i);
                    qWarning() << "sRaw" << sRaw;
                    qWarning() << "storageModeActive" << iStorageModeActive;
                    qWarning() << "iNumberSamples" << lNumberSamples.at(j);
                    qWarning() << "iNumberChannels" << lNumberChannels.at(k);
                    qWarning() << "iNumberTrials" << lNumberTrials.at(l);
                    qWarning() << "iNumberCSDFreqBins" << AbstractMetric::m_iNumberBinAmount;
                    qWarning() << "rows" << matData.rows();
                    qWarning() << "cols" << matData.cols();
                    qWarning() << "numberNodes" << connectivitySettings.getNodePositions().rows();

                    // Check that iNfft >= signal length
                    int iSignalLength = lNumberSamples.at(j);
                    int iNfft = int(raw.info.sfreq/1.0);
                    if(iNfft > iSignalLength) {
                        iNfft = iSignalLength;
                    }

                    qWarning() << "iNfft" << iNfft;

                    int iNFreqs = int(floor(iNfft / 2.0)) + 1;
                    qWarning() << "iNFreqs" << iNFreqs;

                    connectivitySettings.setConnectivityMethods(QStringList() << sConnectivityMethodList.at(i));

                    m_iCurrentIteration = 0;
                    for(int u = 0; u < iNumberRepeats; ++u) {
                        connectivitySettings.clearIntermediateData();

                        qWarning() << "iteration" << m_iCurrentIteration;

                        //Do connectivity estimation
                        connectivityObj.calculate(connectivitySettings);

                        printf("Iteration %d: Calculating %s for %d trials, %d channels, %d samples\n", m_iCurrentIteration, sConnectivityMethodList.at(i).toLatin1().data(), connectivitySettings.size(), lNumberChannels.at(k), lNumberSamples.at(j));

                        m_iCurrentIteration++;
                    }
                }
            }
        }
    }

    return 0;
}
