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

#include <iostream>
#include <vector>
#include <math.h>

//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <mne/mne_inverse_operator.h>

#include <fiff/fiff_evoked.h>
#include <inv/inv_source_estimate.h>
#include <inv/minimum_norm/inv_minimum_norm.h>

#include <utils/generics/mne_logger.h>

#include <disp/plots/helpers/figureexporter.h>
#include <disp/plots/lineplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
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
int main(int argc, char* argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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

    QFile t_fileEvoked(parser.value(evokedFileOption));
    QFile t_fileInv(parser.value(invFileOption));

    float snr = parser.value(snrOption).toFloat();
    QString method(parser.value(methodOption));
    QString t_sFileNameStc(parser.value(stcFileOption));

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    //
    //   Read the data first
    //
    fiff_int_t setno = 0;
    // Without a baseline the DC offset of the average dominates the estimate
    QPair<float, float> baseline(-0.2f, 0.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if (evoked.isEmpty())
        return 1;

    //
    //   Then the inverse operator
    //
    MNEInverseOperator inverse_operator(t_fileInv);

    //
    // Compute inverse solution
    //
    InvMinimumNorm minimumNorm(inverse_operator, lambda2, method);
    InvSourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    //
    //Results
    //
    std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n"
              << sourceEstimate.data.block(0, 0, 10, 10) << std::endl;
    printf("tmin = %f s\n", sourceEstimate.tmin);
    printf("tstep = %f s\n", sourceEstimate.tstep);

    // Time courses of the five sources with the largest peak
    QVector<double> times(sourceEstimate.data.cols());
    for (int s = 0; s < times.size(); ++s) {
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
        courses.addSeries(times, QVector<double>(row.data(), row.data() + row.size()), QString("%1 %2").arg(hemi).arg(sourceEstimate.vertices(source)));
    }
    FigureExporter::save(courses, "time_courses", QString("%1 time courses of the five sources with the largest peak, SNR %2, baseline -200 to 0 ms.").arg(method).arg(snr));

    if (!t_sFileNameStc.isEmpty()) {
        QFile t_fileStc(t_sFileNameStc);
        sourceEstimate.write(t_fileStc);

        //test if everything was written correctly
        InvSourceEstimate readSourceEstimate(t_fileStc);

        std::cout << "\npart ( block( 0, 0, 10, 10) ) of the inverse solution:\n"
                  << readSourceEstimate.data.block(0, 0, 10, 10) << std::endl;
        printf("tmin = %f s\n", readSourceEstimate.tmin);
        printf("tstep = %f s\n", readSourceEstimate.tstep);
    }

    return 0;
}
