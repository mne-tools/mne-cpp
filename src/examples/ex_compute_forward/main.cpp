//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <ruben.deorfel@tu-ilmenau.de>
 * @version  dev
 * @date     April, 2020
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
 * @brief     This example shows how to compute the forward solution.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include <fiff/fiff_info.h>

#include <fwd/computeFwd/compute_fwd_settings.h>
#include <fwd/computeFwd/compute_fwd.h>

#include <mne/mne_forwardsolution.h>

#include <mne/mne.h>
//=============================================================================================================
// Eigen
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

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
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    QElapsedTimer timer0, timer1, timer2, timer3, timer4;
    float fTime0, fTime1, fTime2, fTime3, fTime4;

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Example name");
    parser.addHelpOption();

    QCommandLineOption parameterOption("parameter", "The first parameter description.");
    parser.addOption(parameterOption);

    parser.process(a);

    // specify necessary information for forward computation
    ComputeFwdSettings::SPtr pSettings = ComputeFwdSettings::SPtr(new ComputeFwdSettings);

    QFile t_fileBemName(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    QFile t_fileSrcName(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif");

    pSettings->include_meg = true;
    pSettings->include_eeg = true;
    pSettings->accurate = true;
    pSettings->srcname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif";
    pSettings->measname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    pSettings->mriname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif";
    pSettings->transname.clear();
    pSettings->bemname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    pSettings->mindist = 5.0f/1000.0f;
    pSettings->solname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

    // bring in dev_head transformation and FiffInfo
    // Init data loading and writing
    QFile t_fileIn(pSettings->measname);
    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));

    FiffCoordTransOld meg_head_t = pFiffInfo->dev_head_t.toOld();

    pSettings->meg_head_t = &meg_head_t;
    pSettings->pFiffInfo = pFiffInfo;

    pSettings->checkIntegrity();

    timer0.start();
    QSharedPointer<FWDLIB::ComputeFwd> pComputeFwd = QSharedPointer<FWDLIB::ComputeFwd>(new FWDLIB::ComputeFwd(pSettings));
    fTime0 = timer0.elapsed();

    // perform the actual computation
    timer1.start();
    pComputeFwd->calculateFwd();
    fTime1 = timer1.elapsed();

    // ToDo: Refactor fwd-lib and make MNEForwardSolution a member of computeForward,
    // so next two steps will not be necessary

    // store calculated forward solution in pSettings->solname specified file
    timer2.start();
    pComputeFwd->storeFwd();
    fTime2 = timer2.elapsed();

    // read as MNEForwardSolution
    timer3.start();
    QFile t_solution(pSettings->solname);
    QSharedPointer<MNEForwardSolution> pFwdSolution = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(t_solution));
    fTime3 = timer3.elapsed();

    // update head position to forward solution and only recompute necessary part
    timer4.start();
    pComputeFwd->updateHeadPos(&meg_head_t);
    fTime4 = timer4.elapsed();

    // get updated solution
    pFwdSolution->sol = pComputeFwd->sol;
    pFwdSolution->sol_grad = pComputeFwd->sol_grad;

    // Print timer results
    qInfo() << "The initialization took: " << fTime0  << " ms.";
    qInfo() << "The computation took: " << fTime1  << " ms.";
    qInfo() << "Storing the fwd solution took: " << fTime2 << " ms.";
    qInfo() << "Reading the fwd solution took: " << fTime3 << " ms.";
    qInfo() << "The recomputation took: " << fTime4 << " ms.";
}
