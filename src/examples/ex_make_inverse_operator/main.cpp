//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Assemble MEG+EEG, MEG-only and EEG-only inverse operators from a forward solution and a regularized noise covariance, then apply them.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <inv/inv_source_estimate.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <base/log.h>
#include <base/log_qt.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QFile>
#include <QtCore/QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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
int main(int argc, char* argv[])
{
    mne::install_qt_message_handler();
    QCoreApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Make Inverse Operator Example");
    parser.addHelpOption();
    QCommandLineOption fwdMEGOption("fwdMEG", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption fwdEEGOption("fwdEEG", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-eeg-oct-6-fwd.fif");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "3.0"); //3.0;//0.1;//3.0;

    parser.addOption(fwdMEGOption);
    parser.addOption(fwdEEGOption);
    parser.addOption(covFileOption);
    parser.addOption(evokedFileOption);
    parser.addOption(methodOption);
    parser.addOption(snrOption);

    parser.process(a);


    double snr = parser.value(snrOption).toDouble();
    double lambda2 = 1.0 / pow(snr, 2);
    QString method(parser.value(methodOption));

    // Load data
    FiffEvoked evoked(parser.value(evokedFileOption).toStdU16String(), 0);

    MNEForwardSolution t_forwardMeeg(parser.value(fwdMEGOption).toStdU16String(), {.surf_ori = true});

    FiffCov noise_cov(parser.value(covFileOption).toStdString());

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    // Restrict forward solution as necessary for MEG
    MNEForwardSolution t_forwardMeg = t_forwardMeeg.pick_types(true, false);
    // Alternatively, you can just load a forward solution that is restricted
    MNEForwardSolution t_forwardEeg(parser.value(fwdEEGOption).toStdU16String(), {.surf_ori = true});

    // make an M/EEG, MEG-only, and EEG-only inverse operators
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator_meeg(info, t_forwardMeeg, noise_cov, 0.2f, 0.8f);
    MNEInverseOperator inverse_operator_meg(info, t_forwardMeg, noise_cov, 0.2f, 0.8f);
    MNEInverseOperator inverse_operator_eeg(info, t_forwardEeg, noise_cov, 0.2f, 0.8f);

    // Compute inverse solution
    InvMinimumNorm minimumNorm_meeg(inverse_operator_meeg, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate_meeg = minimumNorm_meeg.calculateInverse(evoked);

    InvMinimumNorm minimumNorm_meg(inverse_operator_meg, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate_meg = minimumNorm_meg.calculateInverse(evoked);

    InvMinimumNorm minimumNorm_eeg(inverse_operator_eeg, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate_eeg = minimumNorm_eeg.calculateInverse(evoked);

    if (sourceEstimate_meeg.empty() || sourceEstimate_meg.empty() || sourceEstimate_eeg.empty())
        return 1;

    // View activation time-series
    std::cout << "\nsourceEstimate_meeg:\n"
              << sourceEstimate_meeg.data.block(0, 0, 10, 10) << std::endl;
    std::cout << "time\n"
              << sourceEstimate_meeg.times.block(0, 0, 1, 10) << std::endl;

    std::cout << "\nsourceEstimate_meg:\n"
              << sourceEstimate_meg.data.block(0, 0, 10, 10) << std::endl;
    std::cout << "time\n"
              << sourceEstimate_meg.times.block(0, 0, 1, 10) << std::endl;

    std::cout << "\nsourceEstimate_eeg:\n"
              << sourceEstimate_eeg.data.block(0, 0, 10, 10) << std::endl;
    std::cout << "time\n"
              << sourceEstimate_eeg.times.block(0, 0, 1, 10) << std::endl;

    return 0;
}
