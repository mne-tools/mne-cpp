//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     Example for cHPI fitting on raw data with SSP. The result is written to a .txt file for comparison with MaxFilter's .pos file.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_dig_point_set.h>

#include <inverse/hpiFit/hpifit.h>

#include <utils/ioutils.h>
#include <utils/generics/applicationlogger.h>
#include <utils/mnemath.h>

#include <fwd/fwd_coil_set.h>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <QGenericMatrix>
#include <QElapsedTimer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * Hpi fitting example to facilitade the use of the HPI fitting class. This example can further be used as CL tool
 * to get an impression of head movements and the quality of the fitting.
 *
 * Example:
 *
 * ex_hpiFit --fileIn C:/Git/mne-cpp/bin/MNE-sample-data/chpi/raw/phantom/2khz_3.fif --freqs 293,307,314,321 --verbose 1 --fileOut 2k_3 --buffer 600 --save 1
 *
 * By default, the example uses the mne-cpp-test-data set.
 *
 */

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QElapsedTimer timer;
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("hpiFit Example");
    parser.addHelpOption();
    qInfo() << "Please download the mne-cpp-test-data folder from Github (mne-tools) into mne-cpp/bin.";

    QCommandLineOption inFile("fileIn", "The input file.", "in", QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");
    QCommandLineOption inBuffer("buffer", "The buffer size in samples.", "in","200");
    QCommandLineOption inStep("step", "The step size in seconds.", "in","0.1");
    QCommandLineOption inFreqs("freqs", "The frequencies used.", "in","154,158,161,166");
    QCommandLineOption inSave("save", "Store the fitting results [0,1].", "in","0");
    QCommandLineOption inVerbose("verbose", "Print to command line [0,1].", "in","0");
    QCommandLineOption inDebug("debug", "Save debug info during HPI fit [0,1].", "in","0");
    QCommandLineOption inFast("fast", "Do fast fits [0,1].", "in","0");
    QCommandLineOption outName("fileOut", "The output file name for movement data.", "out","position.txt");

    parser.addOption(inFile);
    parser.addOption(inBuffer);
    parser.addOption(inStep);
    parser.addOption(inFreqs);
    parser.addOption(inSave);
    parser.addOption(inVerbose);
    parser.addOption(inDebug);
    parser.addOption(inFast);
    parser.addOption(outName);

    parser.process(a);

    int iQuantum = parser.value(inBuffer).toInt();
    if(iQuantum <= 0) {
        // check for proper buffer size
        qWarning() << "Buffer <= 0. Buffer was set to 200 samples.";
        iQuantum = 200;
    }
    float fStep = parser.value(inStep).toFloat();
    if(fStep <= 0.0) {
        // check for proper step size
        qWarning() << "Step <= 0. Step size was set to 0.1 seconds.";
        fStep = 0.1f;
    }
    QStringList lFreqs = parser.value(inFreqs).split(",");
    QFile t_fileIn(parser.value(inFile));
    bool bSave = parser.value(inSave).toInt();
    bool bVerbose = parser.value(inVerbose).toInt();
    bool bDoDebug = parser.value(inDebug).toInt();
    bool bFast = parser.value(inFast).toInt();
    QString sNameOut(parser.value(outName));

    // Init data loading and writing
    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));

    // Setup comparison of transformation matrices
    FiffCoordTrans transDevHead = pFiffInfo->dev_head_t;    // transformation that only updates after big head movements
    float fThreshRot = 5.0f;          // in degree
    float fThreshTrans = 0.005f;    // in m

    // Set up the reading parameters
    RowVectorXi vecPicks = pFiffInfo->pick_types(true, false, false);

    MatrixXd matData, matTimes;

    fiff_int_t from, to;
    fiff_int_t first = raw.first_samp;
    fiff_int_t last = raw.last_samp;

    // create time vector that specifies when to fit
    int iQuantumT = floor(fStep*pFiffInfo->sfreq);      // samples between fits
    int iN = floor((last-first)/iQuantumT)-floor(iQuantum/iQuantumT);
    RowVectorXf vecTime = RowVectorXf::LinSpaced(iN, 0, iN-1);

    // matPosition matrix to save hpi fit results
    MatrixXd matPosition;

    // setup informations for HPI fit (VectorView)
    QVector<int> vecFreqs(lFreqs.size());
    for(int i = 0; i < lFreqs.size(); i++) {
        vecFreqs[i] = lFreqs[i].toInt();
    }

    QVector<double> vecError;
    VectorXd vecGoF;
    FiffDigPointSet fittedPointSet;

    // setup Projectors
    // Use SSP + SGM + calibration
    MatrixXd matProjectors = MatrixXd::Identity(pFiffInfo->chs.size(), pFiffInfo->chs.size());

    //Do a copy here because we are going to change the activity flags of the SSP's
    FiffInfo infoTemp = *(pFiffInfo.data());

    //Turn on all SSP
    for(int i = 0; i < infoTemp.projs.size(); ++i) {
        infoTemp.projs[i].active = true;
    }

    //Create the projector for all SSP's on
    infoTemp.make_projector(matProjectors);

    //set columns of matrix to zero depending on bad channels indexes
    for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
        matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
    }

    // if debugging files are necessary set bDoDebug = true;
    QString sHPIResourceDir = QCoreApplication::applicationDirPath() + "/HPIFittingDebug";

    HPIFit HPI = HPIFit(pFiffInfo,bFast);

    // ordering of frequencies
    from = first + vecTime(0);
    to = from + iQuantum;
    if(!raw.read_raw_segment(matData, matTimes, from, to)) {
        qCritical("error during read_raw_segment");
        return -1;
    }

    // order frequencies
    timer.start();
    HPI.findOrder(matData,
                  matProjectors,
                  pFiffInfo->dev_head_t,
                  vecFreqs,
                  vecError,
                  vecGoF,
                  fittedPointSet,
                  pFiffInfo);

    float fTimer = 0.0;

    // read and fit
    for(int i = 0; i < vecTime.size(); i++) {
        from = first + vecTime(i)*iQuantumT;
        to = from + iQuantum;
        if (to > last) {
            to = last;
            qWarning() << "Block size < iQuantum " << iQuantum;
        }
        // Reading
        if(!raw.read_raw_segment(matData, matTimes, from, to)) {
            qCritical("error during read_raw_segment");
            return -1;
        }

        timer.start();
        HPI.fitHPI(matData,
                   matProjectors,
                   pFiffInfo->dev_head_t,
                   vecFreqs,
                   vecError,
                   vecGoF,
                   fittedPointSet,
                   pFiffInfo,
                   bDoDebug,
                   sHPIResourceDir);
        fTimer = timer.elapsed();

        HPIFit::storeHeadPosition(vecTime(i), pFiffInfo->dev_head_t.trans, matPosition, vecGoF, vecError);
        matPosition(i,9) = fTimer;
        // if big head displacement occures, update debHeadTrans
        if(MNEMath::compareTransformation(transDevHead.trans, pFiffInfo->dev_head_t.trans, fThreshRot, fThreshTrans)) {
            transDevHead = pFiffInfo->dev_head_t;
            qInfo() << "dev_head_t has been updated.";
        }
        if(bVerbose) {
            qInfo() << "Iteration" << i << "Of" << vecTime.size()
                    << " Duration " << fTimer << "ms"
                    << " Error" << vecError[0]*1000 << vecError[1]*1000 << vecError[2]*1000 << vecError[3]*1000
                    << " GoF" << vecGoF[0] << vecGoF[1] << vecGoF[2] << vecGoF[3];
        }
    }
    qInfo() << "Iterations:" << vecTime.size()
            << "Average GoF:" << matPosition.col(7).mean() * 100 << "%"
            << "Average Error:" << matPosition.col(8).mean() * 1000 << "mm"
            << "Average Duration:" << matPosition.col(9).mean() << "ms";

    if(bSave) {
        IOUtils::write_eigen_matrix(matPosition, QCoreApplication::applicationDirPath() + "/MNE-sample-data/" + sNameOut);
    }
}
