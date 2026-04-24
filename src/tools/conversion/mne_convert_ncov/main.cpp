//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Convert old MNE ASCII noise covariance (ncov) format to FIFF.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <utils/generics/mne_logger.h>

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
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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

    if (ncovFile.isEmpty()) { qCritical("--ncov is required."); return 1; }
    if (measFile.isEmpty()) { qCritical("--meas is required."); return 1; }

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
    QFile fMeas(measFile);
    FiffRawData raw(fMeas);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read measurement info from: %s", qPrintable(measFile));
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

    printf("Measurement: %lld MEG channels, %lld EEG channels, %d total\n",
           static_cast<long long>(megIdx.size()), static_cast<long long>(eegIdx.size()), info.nchan);

    //=========================================================================
    // Read ncov ASCII file
    //=========================================================================
    QFile ncFile(ncovFile);
    if (!ncFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open ncov file: %s", qPrintable(ncovFile));
        return 1;
    }
    QTextStream in(&ncFile);

    // Check header
    QString header = in.readLine().trimmed();
    if (header != "#!ascii") {
        qCritical("Invalid ncov header: expected '#!ascii', got '%s'.", qPrintable(header));
        return 1;
    }

    // Read dimensions: nmeg neeg nfree
    QString dimLine = in.readLine().trimmed();
    QStringList dims = dimLine.split(QRegularExpression("\\s+"));
    if (dims.size() < 3) {
        qCritical("Invalid dimension line: '%s'.", qPrintable(dimLine));
        return 1;
    }

    int nMegFile = dims[0].toInt();
    int nEegFile = dims[1].toInt();
    int nFree = dims[2].toInt();
    int nTotal = nMegFile + nEegFile;

    printf("Ncov file: %d MEG + %d EEG = %d channels, nfree = %d\n",
           nMegFile, nEegFile, nTotal, nFree);

    // Validate dimensions
    if (nMegFile != megIdx.size()) {
        qCritical("MEG channel count mismatch: file says %d, measurement has %lld.",
                  nMegFile, static_cast<long long>(megIdx.size()));
        return 1;
    }
    if (nEegFile != eegIdx.size()) {
        qCritical("EEG channel count mismatch: file says %d, measurement has %lld.",
                  nEegFile, static_cast<long long>(eegIdx.size()));
        return 1;
    }

    // Read full covariance matrix
    MatrixXd fullCov(nTotal, nTotal);
    for (int i = 0; i < nTotal; ++i) {
        for (int j = 0; j < nTotal; ++j) {
            if (in.atEnd()) {
                qCritical("Unexpected end of file at element (%d, %d).", i, j);
                return 1;
            }
            in >> fullCov(i, j);
        }
    }
    ncFile.close();

    printf("Read %dx%d covariance matrix\n", nTotal, nTotal);

    //=========================================================================
    // Extract requested submatrix
    //=========================================================================
    QList<int> selectedFileIdx;   // Indices into the file's matrix
    QList<int> selectedMeasIdx;   // Indices into the measurement info

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

    printf("Selected %d channels (%s%s)\n", nSel,
           includeMeg ? "MEG" : "",
           (includeMeg && includeEeg) ? "+EEG" : (includeEeg ? "EEG" : ""));

    //=========================================================================
    // Build and write FIFF covariance
    //=========================================================================
    QStringList chNames;
    for (int i = 0; i < nSel; ++i) {
        chNames << info.chs[selectedMeasIdx[i]].ch_name;
    }

    FiffCov cov;
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.diag = false;
    cov.dim = nSel;
    cov.names = chNames;
    cov.data = selCov;
    cov.nfree = nFree;

    if (!cov.save(covFile)) {
        qCritical("Cannot write output file: %s", qPrintable(covFile));
        return 1;
    }

    printf("Written FIFF covariance (%d channels) to: %s\n", nSel, qPrintable(covFile));
    return 0;
}
