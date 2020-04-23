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
using namespace UTILSLIB;
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
 * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    QElapsedTimer timer;

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Example name");
    parser.addHelpOption();

    QCommandLineOption parameterOption("parameter", "The first parameter description.");

    qInfo() << "Please download the mne-cpp-test-data folder from Github (mne-tools) into mne-cpp/bin.";
    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/test_hpiFit_raw.fif");

    parser.addOption(inputOption);
    parser.addOption(parameterOption);

    parser.process(a);

    // read data
    // Init data loading and writing
    QFile t_fileIn(parser.value(inputOption));
    FiffRawData raw(t_fileIn);
    QSharedPointer<FiffInfo> pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));

    FiffCoordTransOld meg_head_t = pFiffInfo->dev_head_t.toOld();

    // specify necessary information for forward computation
    ComputeFwdSettings settings;

    settings.include_meg = true;
    settings.include_eeg = true;
    settings.accurate = true;
    settings.srcname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    settings.measname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    settings.mriname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif";
    settings.transname.clear();
    settings.bemname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    settings.mindist = 5.0f/1000.0f;
    settings.solname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

    // bring in dev_head transformation and FiffInfo
    settings.meg_head_t = &meg_head_t;
    settings.pFiffInfo = pFiffInfo;

    settings.checkIntegrity();

    QSharedPointer<FWDLIB::ComputeFwd> pFwdMEGEEG = QSharedPointer<FWDLIB::ComputeFwd>(new FWDLIB::ComputeFwd(&settings));

    // perform the actual computation
    timer.start();
    pFwdMEGEEG->calculateFwd();
    qInfo() << "The computation took: " << timer.elapsed() << " ms.";

    // update head position to forward solution and only recompute necessary part
    timer.start();
    pFwdMEGEEG->updateHeadPos(&meg_head_t);
    qInfo() << "The recomputation took: " << timer.elapsed() << " ms.";

    qInfo() << "pFwdMEGEEG->coord_frame: " << pFwdMEGEEG->coord_frame;
    qInfo() << "pFwdMEGEEG->nchan: " << pFwdMEGEEG->nchan;
    qInfo() << "pFwdMEGEEG->nsource: " << pFwdMEGEEG->nsource;

    std::cout << pFwdMEGEEG->mri_head_t.trans <<std::endl;

    // store calculated forward solution in settings.solname specified file
    pFwdMEGEEG->storeFwd();
    qInfo() << "pFwdMEGEEG->coord_frame: " << pFwdMEGEEG->coord_frame;
}
