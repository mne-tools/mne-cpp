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
 * @brief    Example of making an inverse operator
 *
 */

//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>
#include <utils/generics/applicationlogger.h>

#include <iostream>
#include <QCommandLineParser>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace UTILSLIB;

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
    
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Make Inverse Operator Example");
    parser.addHelpOption();
    QCommandLineOption fwdMEGOption("fwdMEG", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption fwdEEGOption("fwdEEG", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-eeg-oct-6-fwd.fif");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "3.0");//3.0;//0.1;//3.0;

    parser.addOption(fwdMEGOption);
    parser.addOption(fwdEEGOption);
    parser.addOption(covFileOption);
    parser.addOption(evokedFileOption);
    parser.addOption(methodOption);
    parser.addOption(snrOption);

    parser.process(a);

    QFile t_fileFwdMeeg(parser.value(fwdMEGOption));
    QFile t_fileFwdEeg(parser.value(fwdEEGOption));
    QFile t_fileCov(parser.value(covFileOption));
    QFile t_fileEvoked(parser.value(evokedFileOption));

    double snr = parser.value(snrOption).toDouble();
    double lambda2 = 1.0 / pow(snr, 2);
    QString method(parser.value(methodOption));

    // Load data
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    MNEForwardSolution t_forwardMeeg(t_fileFwdMeeg, false, true);

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    // Restrict forward solution as necessary for MEG
    MNEForwardSolution t_forwardMeg = t_forwardMeeg.pick_types(true, false);
    // Alternatively, you can just load a forward solution that is restricted
    MNEForwardSolution t_forwardEeg(t_fileFwdEeg, false, true);

    // make an M/EEG, MEG-only, and EEG-only inverse operators
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator_meeg(info, t_forwardMeeg, noise_cov, 0.2f, 0.8f);
    MNEInverseOperator inverse_operator_meg(info, t_forwardMeg, noise_cov, 0.2f, 0.8f);
    MNEInverseOperator inverse_operator_eeg(info, t_forwardEeg, noise_cov, 0.2f, 0.8f);

    // Compute inverse solution
    MinimumNorm minimumNorm_meeg(inverse_operator_meeg, lambda2, method);
    MNESourceEstimate sourceEstimate_meeg = minimumNorm_meeg.calculateInverse(evoked);

    MinimumNorm minimumNorm_meg(inverse_operator_meg, lambda2, method);
    MNESourceEstimate sourceEstimate_meg = minimumNorm_meg.calculateInverse(evoked);

    MinimumNorm minimumNorm_eeg(inverse_operator_eeg, lambda2, method);
    MNESourceEstimate sourceEstimate_eeg = minimumNorm_eeg.calculateInverse(evoked);

    if(sourceEstimate_meeg.isEmpty() || sourceEstimate_meg.isEmpty() || sourceEstimate_eeg.isEmpty())
        return 1;

    // View activation time-series
    std::cout << "\nsourceEstimate_meeg:\n" << sourceEstimate_meeg.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimate_meeg.times.block(0,0,1,10) << std::endl;

    std::cout << "\nsourceEstimate_meg:\n" << sourceEstimate_meg.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimate_meg.times.block(0,0,1,10) << std::endl;

    std::cout << "\nsourceEstimate_eeg:\n" << sourceEstimate_eeg.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimate_eeg.times.block(0,0,1,10) << std::endl;

    return a.exec();
}
