//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2001
 * @brief    Simulate MEG/EEG sensor data using a forward model and optional noise.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_forward_solution.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Cholesky>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <random>
#include <cmath>
#include <numbers>
#include <print>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Simulate sensor data from a forward model.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"fwd"}, "Forward solution FIFF file.", "file", {}});
    parser.add_option({{"raw"}, "Raw FIFF file (for channel info & sampling rate).", "file", {}});
    parser.add_option({{"cov"}, "Noise covariance FIFF file.", "file", {}});
    parser.add_option({{"source"}, "Source index to activate.", "idx", "0"});
    parser.add_option({{"snr"}, "Signal-to-noise ratio in dB.", "value", "20"});
    parser.add_option({{"duration"}, "Duration of simulation in seconds.", "sec", "1.0"});
    parser.add_option({{"freq"}, "Signal frequency in Hz.", "Hz", "10.0"});
    parser.add_option({{"out"}, "Output simulated raw FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_simu", PROGRAM_VERSION);

    const std::string fwdFile = parser.value("fwd");
    const std::string rawFile = parser.value("raw");
    const std::string covFile = parser.value("cov");
    const std::string outFile = parser.value("out");
    int sourceIdx = to_int(parser.value("source")).value_or(0);
    double snrDb = to_double(parser.value("snr")).value_or(0.0);
    double duration = to_double(parser.value("duration")).value_or(0.0);
    double signalFreq = to_double(parser.value("freq")).value_or(0.0);

    if (fwdFile.empty()) {
        Log::critical("--fwd is required.");
        std::print("{}", parser.help_text("mne_simu"));
        return 1;
    }
    if (rawFile.empty()) {
        Log::critical("--raw is required.");
        std::print("{}", parser.help_text("mne_simu"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_simu"));
        return 1;
    }

    // Load forward solution
    auto fwdRead = MNEForwardSolution::read(path_from_utf8(fwdFile));
    if (!fwdRead) {
        Log::critical("{}", fwdRead.error().what());
        return 1;
    }
    MNEForwardSolution& fwd = *fwdRead;

    int nChan = fwd.sol.data.rows();
    int nSrc = fwd.sol.data.cols();
    Log::info("Forward: {} channels x {} sources", nChan, nSrc);

    if (sourceIdx < 0 || sourceIdx >= nSrc) {
        Log::critical("Source index {} out of range [0, {})", sourceIdx, nSrc);
        return 1;
    }

    // Load raw file for channel info
    FiffRawData raw(path_from_utf8(rawFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read raw: {}", rawFile);
        return 1;
    }

    double sfreq = raw.info.sfreq;
    int nSamples = static_cast<int>(duration * sfreq);
    Log::info("Sampling rate: {:.1f} Hz, duration: {:.1f} s, samples: {}", sfreq, duration, nSamples);

    // Get gain vector for selected source
    VectorXd gainCol = fwd.sol.data.col(sourceIdx);

    // Create source signal: sinusoid
    VectorXd sourceSignal(nSamples);
    for (int t = 0; t < nSamples; ++t) {
        double time = t / sfreq;
        sourceSignal(t) = sin(2.0 * std::numbers::pi * signalFreq * time);
    }

    // Compute simulated sensor data: data = gain * signal
    MatrixXd data = gainCol * sourceSignal.transpose();
    Log::info("Signal: source={}, freq={:.1f} Hz, amplitude=1 nAm", sourceIdx, signalFreq);

    // Add noise if covariance provided
    if (!covFile.empty()) {
        FiffCov cov(path_from_utf8(covFile));
        if (!cov.empty() && cov.dim == nChan) {
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
            Log::info("Added noise: SNR={:.1f} dB, noise scale={:g}", snrDb, noiseScale);
        } else {
            Log::info("Warning: Covariance dimensions mismatch, skipping noise.");
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
        Log::info("Added white noise: SNR={:.1f} dB", snrDb);
    }

    // Write output
    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(path_from_utf8(outFile), raw.info, cals);
    if (!stream) {
        Log::critical("Cannot open output file: {}", outFile);
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
    Log::info("Written simulated data to: {}", outFile);
    return 0;
}
