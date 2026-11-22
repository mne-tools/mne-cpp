//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert old MNE ASCII noise covariance (ncov) format to FIFF.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QFileInfo>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_convert_ncov");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Convert old MNE ASCII noise covariance (ncov) format to FIFF.\n\n"
        "The ncov format uses a header line '#!ascii' followed by a dimensions\n"
        "line 'nmeg neeg nfree' and a full symmetric matrix in row-major order.\n"
        "The tool can selectively include MEG-only, EEG-only, or combined\n"
        "channels.\n\n"
        "Port of the MNE-C mne_convert_ncov utility.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption ncovOpt("ncov", "Input ncov ASCII covariance file.", "file");
    QCommandLineOption covOpt("cov", "Output FIFF covariance file (default: <ncov-stem>.fif).", "file");
    QCommandLineOption measOpt("meas", "Measurement FIFF file (for channel names).", "file");
    QCommandLineOption megOpt("meg", "Include MEG channels.");
    QCommandLineOption eegOpt("eeg", "Include EEG channels.");

    parser.addOption(ncovOpt);
    parser.addOption(covOpt);
    parser.addOption(measOpt);
    parser.addOption(megOpt);
    parser.addOption(eegOpt);

    parser.process(app);

    QString ncovFile = parser.value(ncovOpt);
    QString covFile = parser.value(covOpt);
    QString measFile = parser.value(measOpt);
    bool includeMeg = parser.isSet(megOpt);
    bool includeEeg = parser.isSet(eegOpt);

    if (ncovFile.isEmpty()) {
        Log::critical("--ncov is required.");
        return 1;
    }
    if (measFile.isEmpty()) {
        Log::critical("--meas is required.");
        return 1;
    }

    // Default: include both if neither specified
    if (!includeMeg && !includeEeg) {
        includeMeg = true;
        includeEeg = true;
    }

    // Default output name
    if (covFile.isEmpty()) {
        QFileInfo fi(ncovFile);
        covFile = fi.path() + "/" + fi.completeBaseName() + ".fif";
    }

    //=========================================================================
    // Read measurement info for channel names and types
    //=========================================================================
    FiffRawData raw(measFile.toStdU16String());
    if (raw.info.empty()) {
        Log::critical("Cannot read measurement info from: {}", measFile.toStdString());
        return 1;
    }
    const FiffInfo& info = raw.info;

    // Classify channels
    QList<int> megIdx, eegIdx;
    for (int i = 0; i < info.nchan; ++i) {
        int kind = info.chs[i].kind;
        if (kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH) {
            megIdx.append(i);
        } else if (kind == FIFFV_EEG_CH) {
            eegIdx.append(i);
        }
    }

    Log::info("Measurement: {} MEG channels, {} EEG channels, {} total", static_cast<long long>(megIdx.size()), static_cast<long long>(eegIdx.size()), info.nchan);

    //=========================================================================
    // Read ncov ASCII file
    //=========================================================================
    QFile ncFile(ncovFile);
    if (!ncFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open ncov file: {}", ncovFile.toStdString());
        return 1;
    }
    QTextStream in(&ncFile);

    // Check header
    QString header = in.readLine().trimmed();
    if (header != "#!ascii") {
        Log::critical("Invalid ncov header: expected '#!ascii', got '{}'.", header.toStdString());
        return 1;
    }

    // Read dimensions: nmeg neeg nfree
    QString dimLine = in.readLine().trimmed();
    QStringList dims = dimLine.split(QRegularExpression("\\s+"));
    if (dims.size() < 3) {
        Log::critical("Invalid dimension line: '{}'.", dimLine.toStdString());
        return 1;
    }

    int nMegFile = dims[0].toInt();
    int nEegFile = dims[1].toInt();
    int nFree = dims[2].toInt();
    int nTotal = nMegFile + nEegFile;

    Log::info("Ncov file: {} MEG + {} EEG = {} channels, nfree = {}", nMegFile, nEegFile, nTotal, nFree);

    // Validate dimensions
    if (nMegFile != megIdx.size()) {
        Log::critical("MEG channel count mismatch: file says {}, measurement has {}.", nMegFile, static_cast<long long>(megIdx.size()));
        return 1;
    }
    if (nEegFile != eegIdx.size()) {
        Log::critical("EEG channel count mismatch: file says {}, measurement has {}.", nEegFile, static_cast<long long>(eegIdx.size()));
        return 1;
    }

    // Read full covariance matrix
    MatrixXd fullCov(nTotal, nTotal);
    for (int i = 0; i < nTotal; ++i) {
        for (int j = 0; j < nTotal; ++j) {
            if (in.atEnd()) {
                Log::critical("Unexpected end of file at element ({}, {}).", i, j);
                return 1;
            }
            in >> fullCov(i, j);
        }
    }
    ncFile.close();

    Log::info("Read {}x{} covariance matrix", nTotal, nTotal);

    //=========================================================================
    // Extract requested submatrix
    //=========================================================================
    QList<int> selectedFileIdx; // Indices into the file's matrix
    QList<int> selectedMeasIdx; // Indices into the measurement info

    if (includeMeg) {
        for (int i = 0; i < nMegFile; ++i) {
            selectedFileIdx.append(i);
            selectedMeasIdx.append(megIdx[i]);
        }
    }
    if (includeEeg) {
        for (int i = 0; i < nEegFile; ++i) {
            selectedFileIdx.append(nMegFile + i);
            selectedMeasIdx.append(eegIdx[i]);
        }
    }

    int nSel = selectedFileIdx.size();
    MatrixXd selCov(nSel, nSel);
    for (int i = 0; i < nSel; ++i) {
        for (int j = 0; j < nSel; ++j) {
            selCov(i, j) = fullCov(selectedFileIdx[i], selectedFileIdx[j]);
        }
    }

    // Symmetrize
    selCov = (selCov + selCov.transpose()) / 2.0;

    Log::info("Selected {} channels ({}{})", nSel, includeMeg ? "MEG" : "", (includeMeg && includeEeg) ? "+EEG" : (includeEeg ? "EEG" : ""));

    //=========================================================================
    // Build and write FIFF covariance
    //=========================================================================
    StringList chNames;
    for (int i = 0; i < nSel; ++i) {
        chNames.push_back(info.chs[selectedMeasIdx[i]].ch_name);
    }

    FiffCov cov;
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.diag = false;
    cov.dim = nSel;
    cov.names = chNames;
    cov.data = selCov;
    cov.nfree = nFree;

    if (!cov.save(covFile.toStdU16String())) {
        Log::critical("Cannot write output file: {}", covFile.toStdString());
        return 1;
    }

    Log::info("Written FIFF covariance ({} channels) to: {}", nSel, covFile.toStdString());
    return 0;
}
