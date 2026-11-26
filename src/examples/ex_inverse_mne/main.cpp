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
 * @brief    L2 minimum-norm or dSPM estimate of the sample evoked response from a precomputed inverse operator.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/mne/mne_inverse_operator.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>
#include <mne/base/log.h>
#include <mne/viz/backends/qt/plots/helpers/figureexporter.h>
#include <mne/viz/backends/qt/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace Eigen;
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
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Inverse MNE Example");
    parser.addHelpOption();

    QCommandLineOption evokedFileOption("ave", "Path to evoked <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption invFileOption("inv", "Path to inverse operator <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif");
    QCommandLineOption snrOption("snr", "The <snr> value used for computation.", "snr", "3.0");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");

    parser.addOption(evokedFileOption);
    parser.addOption(invFileOption);
    parser.addOption(snrOption);
    parser.addOption(methodOption);
    parser.addOption(stcFileOption);
    parser.process(app);

    const std::filesystem::path t_fileInv = parser.value(invFileOption).toStdU16String();

    float snr = parser.value(snrOption).toFloat();
    QString method(parser.value(methodOption));
    QString t_sFileNameStc(parser.value(stcFileOption));

    double lambda2 = 1.0 / pow(snr, 2);
    Log::debug("Start calculation with: SNR {} ; Lambda {} ; Method {} ; stc: {}", snr, lambda2, method.toStdString(), t_sFileNameStc.toStdString());

    //
    //   Read the data first
    //
    // Without a baseline the DC offset of the average dominates the estimate
    FiffEvoked evoked(parser.value(evokedFileOption).toStdU16String(), 0, {{-0.2f, 0.0f}});

    //
    //   Then the inverse operator
    //
    MNEInverseOperator inverse_operator(t_fileInv);

    //
    // Compute inverse solution
    //
    InvMinimumNorm minimumNorm(inverse_operator, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    //
    //Results
    //
    std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n"
              << sourceEstimate.data.block(0, 0, 10, 10) << std::endl;
    printf("tmin = %f s\n", sourceEstimate.tmin);
    printf("tstep = %f s\n", sourceEstimate.tstep);

    // Time courses of the five sources with the largest peak
    std::vector<double> times(sourceEstimate.data.cols());
    for (int s = 0; s < std::ssize(times); ++s) {
        times[s] = 1000.0 * (sourceEstimate.tmin + s * sourceEstimate.tstep);
    }
    VectorXd peaks = sourceEstimate.data.cwiseAbs().rowwise().maxCoeff();
    LinePlot courses;
    courses.setTitle(QString("%1 source time courses of '%2'").arg(method, evoked.comment));
    courses.setXLabel("Time [ms]");
    courses.setYLabel(method);
    for (int i = 0; i < 5; ++i) {
        Index source;
        peaks.maxCoeff(&source);
        peaks(source) = -1.0;
        const VectorXd row = sourceEstimate.data.row(source).transpose();
        const QString hemi = sourceEstimate.nVerticesLh < 0 ? QString("source") : source < sourceEstimate.nVerticesLh ? QString("lh vertex")
                                                                                                                      : QString("rh vertex");
        courses.addSeries(times, std::vector<double>(row.data(), row.data() + row.size()), QString("%1 %2").arg(hemi).arg(sourceEstimate.vertices(source)));
    }
    FigureExporter::save(courses, "time_courses", QString("%1 time courses of the five sources with the largest peak, SNR %2, baseline -200 to 0 ms.").arg(method).arg(snr).toStdString());

    if (!t_sFileNameStc.isEmpty()) {
        const std::filesystem::path stcPath = t_sFileNameStc.toStdU16String();
        if (auto written = sourceEstimate.write(stcPath); !written) {
            Log::warning("{}", written.error().what());
            return 1;
        }

        //test if everything was written correctly
        const InvSourceEstimate readSourceEstimate(stcPath);

        std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n"
                  << readSourceEstimate.data.block(0, 0, 10, 10) << std::endl;
        printf("tmin = %f s\n", readSourceEstimate.tmin);
        printf("tstep = %f s\n", readSourceEstimate.tstep);
    }

    return 0;
}
