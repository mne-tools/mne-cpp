//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Simulate MEG/EEG sensor data using a forward model and optional noise.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Cholesky>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_simu");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Simulate sensor data from a forward model.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fwdOpt("fwd", "Forward solution FIFF file.", "file");
    parser.addOption(fwdOpt);

    QCommandLineOption rawOpt("raw", "Raw FIFF file (for channel info & sampling rate).", "file");
    parser.addOption(rawOpt);

    QCommandLineOption covOpt("cov", "Noise covariance FIFF file.", "file");
    parser.addOption(covOpt);

    QCommandLineOption sourceOpt("source", "Source index to activate.", "idx", "0");
    parser.addOption(sourceOpt);

    QCommandLineOption snrOpt("snr", "Signal-to-noise ratio in dB.", "value", "20");
    parser.addOption(snrOpt);

    QCommandLineOption durationOpt("duration", "Duration of simulation in seconds.", "sec", "1.0");
    parser.addOption(durationOpt);

    QCommandLineOption freqOpt("freq", "Signal frequency in Hz.", "Hz", "10.0");
    parser.addOption(freqOpt);

    QCommandLineOption outOpt("out", "Output simulated raw FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString fwdFile = parser.value(fwdOpt);
    QString rawFile = parser.value(rawOpt);
    QString covFile = parser.value(covOpt);
    QString outFile = parser.value(outOpt);
    int sourceIdx = parser.value(sourceOpt).toInt();
    double snrDb = parser.value(snrOpt).toDouble();
    double duration = parser.value(durationOpt).toDouble();
    double signalFreq = parser.value(freqOpt).toDouble();

    if (fwdFile.isEmpty()) { qCritical("--fwd is required."); parser.showHelp(1); }
    if (rawFile.isEmpty()) { qCritical("--raw is required."); parser.showHelp(1); }
    if (outFile.isEmpty()) { qCritical("--out is required."); parser.showHelp(1); }

    // Load forward solution
    QFile fFwd(fwdFile);
    MNEForwardSolution fwd(fFwd);
    if (fwd.sol->data.size() == 0) {
        qCritical("Cannot read forward solution: %s", qPrintable(fwdFile));
        return 1;
    }

    int nChan = fwd.sol->data.rows();
    int nSrc = fwd.sol->data.cols();
    printf("Forward: %d channels x %d sources\n", nChan, nSrc);

    if (sourceIdx < 0 || sourceIdx >= nSrc) {
        qCritical("Source index %d out of range [0, %d)", sourceIdx, nSrc);
        return 1;
    }

    // Load raw file for channel info
    QFile fRaw(rawFile);
    FiffRawData raw(fRaw);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw: %s", qPrintable(rawFile));
        return 1;
    }

    double sfreq = raw.info.sfreq;
    int nSamples = static_cast<int>(duration * sfreq);
    printf("Sampling rate: %.1f Hz, duration: %.1f s, samples: %d\n", sfreq, duration, nSamples);

    // Get gain vector for selected source
    VectorXd gainCol = fwd.sol->data.col(sourceIdx);

    // Create source signal: sinusoid
    VectorXd sourceSignal(nSamples);
    for (int t = 0; t < nSamples; ++t) {
        double time = t / sfreq;
        sourceSignal(t) = sin(2.0 * M_PI * signalFreq * time);
    }

    // Compute simulated sensor data: data = gain * signal
    MatrixXd data = gainCol * sourceSignal.transpose();
    printf("Signal: source=%d, freq=%.1f Hz, amplitude=1 nAm\n", sourceIdx, signalFreq);

    // Add noise if covariance provided
    if (!covFile.isEmpty()) {
        QFile fCov(covFile);
        FiffCov cov(fCov);
        if (!cov.isEmpty() && cov.dim == nChan) {
            // Cholesky decomposition for correlated noise
            LLT<MatrixXd> llt(cov.data);
            MatrixXd L = llt.matrixL();

            // Calculate noise scaling from SNR
            double snrLinear = pow(10.0, snrDb / 20.0);
            double signalPower = data.squaredNorm() / (nChan * nSamples);
            double noiseScale = sqrt(signalPower) / snrLinear;

            // Generate noise
            std::mt19937 rng(42);
            std::normal_distribution<double> dist(0.0, 1.0);

            MatrixXd noise(nChan, nSamples);
            for (int i = 0; i < nChan; ++i)
                for (int t = 0; t < nSamples; ++t)
                    noise(i, t) = dist(rng);

            noise = noiseScale * L * noise;
            data += noise;
            printf("Added noise: SNR=%.1f dB, noise scale=%g\n", snrDb, noiseScale);
        } else {
            printf("Warning: Covariance dimensions mismatch, skipping noise.\n");
        }
    } else {
        // Add simple white noise based on SNR
        double snrLinear = pow(10.0, snrDb / 20.0);
        double signalPower = data.squaredNorm() / (nChan * nSamples);
        double noiseScale = sqrt(signalPower) / snrLinear;

        std::mt19937 rng(42);
        std::normal_distribution<double> dist(0.0, noiseScale);

        for (int i = 0; i < nChan; ++i)
            for (int t = 0; t < nSamples; ++t)
                data(i, t) += dist(rng);
        printf("Added white noise: SNR=%.1f dB\n", snrDb);
    }

    // Write output
    QFile outF(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(outF, raw.info, cals);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    // Write in chunks
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    printf("Written simulated data to: %s\n", qPrintable(outFile));
    return 0;
}
