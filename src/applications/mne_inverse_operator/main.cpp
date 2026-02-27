//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Matti Hamalainen. All rights reserved.
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
 * @brief    Implements mne_inverse_operator application.
 *           Port of the original MNE-C mne_inverse_operator by Matti Hamalainen.
 *
 *           Assembles the inverse operator from a forward solution and a noise
 *           covariance matrix.  Supports MEG-only, EEG-only, or combined
 *           MEG + EEG solutions with fixed, loose, or free source orientations
 *           and depth weighting.
 *
 *           Like the original MNE-C tool, no measurement file is needed.
 *           Channel information is obtained from the forward solution, and
 *           SSP projectors are read from the noise covariance file (or from
 *           explicit --proj files).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_proj.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_forwardsolution.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "1.0"

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Build a FiffInfo from the forward solution's FiffInfoBase.
 * Copies all base-class fields (chs, ch_names, bads, coordinate transforms, etc.)
 * and populates SSP projectors from the noise covariance and/or --proj files.
 * This mirrors the original MNE-C approach where no measurement file is needed.
 */
static FiffInfo buildInfoFromForward(const MNEForwardSolution &forward,
                                     const FiffCov &noiseCov,
                                     const QList<FiffProj> &extraProjs)
{
    FiffInfo info;

    // Copy all FiffInfoBase members from the forward solution
    info.chs        = forward.info.chs;
    info.nchan      = forward.info.nchan;
    info.ch_names   = forward.info.ch_names;
    info.bads       = forward.info.bads;
    info.meas_id    = forward.info.meas_id;
    info.dev_head_t = forward.info.dev_head_t;
    info.ctf_head_t = forward.info.ctf_head_t;

    // --- SSP projectors ---
    // Priority (matching SVN MNE-C):
    //   1. Projectors embedded in the noise covariance file
    //   2. Projectors from explicit --proj files (only if noise cov had none)
    if (!noiseCov.projs.isEmpty()) {
        printf("  Using %lld SSP projectors from noise covariance file.\n",
               noiseCov.projs.size());
        info.projs = noiseCov.projs;
        FiffProj::activate_projs(info.projs);
        if (!extraProjs.isEmpty()) {
            printf("  NOTE: Noise covariance already contains projectors; "
                   "ignoring --proj files.\n");
        }
    } else if (!extraProjs.isEmpty()) {
        printf("  Using %lld SSP projectors from --proj files.\n",
               extraProjs.size());
        info.projs = extraProjs;
        FiffProj::activate_projs(info.projs);
    }

    // Auto-add average EEG reference projector if EEG channels are present
    // and no EEG average ref projector was found (matches SVN MNE-C behavior)
    bool hasEeg = false;
    bool hasEegAvRef = false;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH)
            hasEeg = true;
    }
    for (int i = 0; i < info.projs.size(); ++i) {
        if (info.projs[i].kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF)
            hasEegAvRef = true;
    }
    if (hasEeg && !hasEegAvRef) {
        printf("  Adding average EEG reference projector.\n");
        // Build EEG average reference projector
        QStringList eegNames;
        for (int i = 0; i < info.nchan; ++i) {
            if (info.chs[i].kind == FIFFV_EEG_CH)
                eegNames << info.chs[i].ch_name;
        }
        if (eegNames.size() > 0) {
            RowVectorXd vecOnes = RowVectorXd::Ones(eegNames.size());
            MatrixXd data = vecOnes / sqrt((double)eegNames.size());
            FiffNamedMatrix projData(1, eegNames.size(),
                                     QStringList(),
                                     eegNames,
                                     data);
            FiffProj eegAvRef;
            eegAvRef.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF;
            eegAvRef.active = true;
            eegAvRef.desc = QString("Average EEG reference");
            eegAvRef.data = FiffNamedMatrix::SDPtr(new FiffNamedMatrix(projData));
            info.projs.append(eegAvRef);
        }
    }

    return info;
}

/**
 * Read SSP projectors from a FIFF file.
 */
static QList<FiffProj> readProjFile(const QString &fileName)
{
    QList<FiffProj> projs;
    QFile file(fileName);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        qWarning() << "  WARNING: Could not open projection file" << fileName;
        return projs;
    }
    projs = stream->read_proj(stream->dirtree());
    stream->close();
    return projs;
}

/**
 * Read bad channel names from a text file (one channel name per line).
 */
static QStringList readBadFile(const QString &fileName)
{
    QStringList bads;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "  WARNING: Could not open bad channel file" << fileName;
        return bads;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#'))
            bads << line;
    }
    file.close();
    return bads;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_inverse_operator application.
 * This is a port of the original MNE-C mne_inverse_operator by Matti Hamalainen.
 *
 * It reads a forward solution and a noise covariance matrix, then assembles
 * the inverse operator decomposition and writes it to a FIFF file.
 *
 * Like the original MNE-C tool, no measurement file is required. Channel
 * information is obtained from the forward solution, SSP projectors from the
 * noise covariance file or explicit --proj files.
 *
 * @param[in] argc  (argument count)
 * @param[in] argv  (argument vector)
 * @return exit code (0 on success).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_inverse_operator");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Assemble an inverse operator from a forward solution and noise covariance.\n"
        "Port of the original MNE-C mne_inverse_operator by Matti Hamalainen.\n\n"
        "Supports MEG-only, EEG-only, or combined MEG+EEG solutions with\n"
        "fixed, loose, or free source orientations and depth weighting.\n\n"
        "Channel information is obtained from the forward solution.\n"
        "SSP projectors are read from the noise covariance file (or --proj files).\n"
        "No measurement file is required (same as the original MNE-C tool)."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    // --fwd: Forward solution file
    QCommandLineOption fwdOpt(QStringList() << "fwd",
        "The forward solution file.", "file");
    parser.addOption(fwdOpt);

    // --noisecov / --senscov: Noise covariance matrix file
    QCommandLineOption noisecovOpt(QStringList() << "noisecov" << "senscov",
        "The noise (sensor) covariance matrix file.", "file");
    parser.addOption(noisecovOpt);

    // --srccov: Source covariance matrix file (optional, defaults to identity)
    QCommandLineOption srccovOpt(QStringList() << "srccov",
        "Source covariance matrix file (default: identity).", "file");
    parser.addOption(srccovOpt);

    // --meg: Use MEG channels
    QCommandLineOption megOpt(QStringList() << "meg",
        "Use MEG channels.");
    parser.addOption(megOpt);

    // --eeg: Use EEG channels
    QCommandLineOption eegOpt(QStringList() << "eeg",
        "Use EEG channels (specify both --meg and --eeg for a combined solution).");
    parser.addOption(eegOpt);

    // --fixed: Use fixed source orientations
    QCommandLineOption fixedOpt(QStringList() << "fixed",
        "Use fixed source orientations normal to the cortical surface.");
    parser.addOption(fixedOpt);

    // --loose: Loose orientation constraint amount
    QCommandLineOption looseOpt(QStringList() << "loose",
        "Amount of loose orientation constraint (0.0-1.0, default: 0.2).",
        "amount", "0.2");
    parser.addOption(looseOpt);

    // --depth: Enable depth weighting (flag, like SVN MNE-C)
    QCommandLineOption depthOpt(QStringList() << "depth",
        "Enable depth weighting.");
    parser.addOption(depthOpt);

    // --weightexp: Depth weighting exponent
    QCommandLineOption weightexpOpt(QStringList() << "weightexp",
        "Depth weighting exponent (0.0-1.0, default: 0.8).",
        "value", "0.8");
    parser.addOption(weightexpOpt);

    // --weightlimit: Depth weighting maximum
    QCommandLineOption weightlimitOpt(QStringList() << "weightlimit",
        "Depth weighting upper limit (default: 10.0).",
        "value", "10.0");
    parser.addOption(weightlimitOpt);

    // --nodepth: Disable depth weighting
    QCommandLineOption noDepthOpt(QStringList() << "nodepth",
        "Disable depth weighting.");
    parser.addOption(noDepthOpt);

    // --reg: Regularization factor (same for all channel types)
    QCommandLineOption regOpt(QStringList() << "reg",
        "Regularization factor for all channel types.", "amount");
    parser.addOption(regOpt);

    // --magreg: Regularization for magnetometers
    QCommandLineOption magregOpt(QStringList() << "magreg",
        "Regularization factor for magnetometers (default: 0.1).",
        "amount", "0.1");
    parser.addOption(magregOpt);

    // --gradreg: Regularization for gradiometers
    QCommandLineOption gradregOpt(QStringList() << "gradreg",
        "Regularization factor for gradiometers (default: 0.1).",
        "amount", "0.1");
    parser.addOption(gradregOpt);

    // --eegreg: Regularization for EEG channels
    QCommandLineOption eegregOpt(QStringList() << "eegreg",
        "Regularization factor for EEG channels (default: 0.1).",
        "amount", "0.1");
    parser.addOption(eegregOpt);

    // --diagnoise: Use only diagonal noise covariance
    QCommandLineOption diagnoiseOpt(QStringList() << "diagnoise",
        "Omit off-diagonal terms from the noise covariance matrix.");
    parser.addOption(diagnoiseOpt);

    // --proj: SSP projection file (can be repeated)
    QCommandLineOption projOpt(QStringList() << "proj",
        "Load SSP projectors from file (can be repeated).", "file");
    parser.addOption(projOpt);

    // --bad: Bad channels file (one channel name per line)
    QCommandLineOption badOpt(QStringList() << "bad",
        "Bad channels file (one name per line, can be repeated).", "file");
    parser.addOption(badOpt);

    // --inv: Output inverse operator file
    QCommandLineOption invOpt(QStringList() << "inv",
        "Output file for the inverse operator (default: <fwd>-inv.fif).", "file");
    parser.addOption(invOpt);

    parser.process(app);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.isSet(fwdOpt)) {
        qCritical() << "Error: --fwd option is required.";
        parser.showHelp(1);
    }
    if (!parser.isSet(noisecovOpt)) {
        qCritical() << "Error: --noisecov option is required.";
        parser.showHelp(1);
    }

    // Default to MEG+EEG if neither --meg nor --eeg is specified
    bool useMeg = parser.isSet(megOpt);
    bool useEeg = parser.isSet(eegOpt);
    if (!useMeg && !useEeg) {
        printf("Neither --meg nor --eeg specified; using all available channels.\n");
        useMeg = true;
        useEeg = true;
    }

    bool useFixed = parser.isSet(fixedOpt);
    float loose = parser.value(looseOpt).toFloat();
    float depth = parser.isSet(noDepthOpt) ? 0.0f
                : parser.isSet(depthOpt)   ? parser.value(weightexpOpt).toFloat()
                :                            0.8f;
    bool diagNoise = parser.isSet(diagnoiseOpt);

    // Regularization parameters
    float magReg = parser.value(magregOpt).toFloat();
    float gradReg = parser.value(gradregOpt).toFloat();
    float eegReg = parser.value(eegregOpt).toFloat();
    if (parser.isSet(regOpt)) {
        float regAll = parser.value(regOpt).toFloat();
        magReg = regAll;
        gradReg = regAll;
        eegReg = regAll;
    }

    // Output file name
    QString fwdName = parser.value(fwdOpt);
    QString invName;
    if (parser.isSet(invOpt)) {
        invName = parser.value(invOpt);
    } else {
        // Derive from forward solution file name
        QFileInfo fi(fwdName);
        QString base = fi.completeBaseName();
        if (base.endsWith("-fwd")) {
            base.chop(4);
        }
        invName = fi.path() + "/" + base + "-inv.fif";
    }

    //=========================================================================================================
    // Read forward solution
    //=========================================================================================================

    printf("\n");
    printf("========================================\n");
    printf("Reading forward solution from %s...\n", fwdName.toUtf8().constData());

    QFile fwdFile(fwdName);
    MNEForwardSolution forward(fwdFile, false, true);

    if (forward.isEmpty()) {
        qCritical() << "Error: Could not read forward solution from" << fwdName;
        return 1;
    }

    printf("  Forward solution: %d sources, %d channels\n",
           forward.nsource, forward.nchan);
    printf("  Source orientation: %s\n",
           forward.isFixedOrient() ? "fixed" : "free");
    printf("  Source space type: %d hemispheres\n", forward.src.size());

    // Pick channel types according to --meg/--eeg options
    if (!(useMeg && useEeg)) {
        printf("Restricting forward solution to %s channels...\n",
               useMeg ? "MEG" : "EEG");
        forward = forward.pick_types(useMeg, useEeg);
        if (forward.isEmpty()) {
            qCritical() << "Error: No channels remaining after channel type restriction.";
            return 1;
        }
        printf("  After restriction: %d channels\n", forward.nchan);
    }

    //=========================================================================================================
    // Read noise covariance
    //=========================================================================================================

    printf("\nReading noise covariance from %s...\n",
           parser.value(noisecovOpt).toUtf8().constData());

    QFile covFile(parser.value(noisecovOpt));
    FiffCov noiseCov(covFile);

    if (noiseCov.data.rows() == 0) {
        qCritical() << "Error: Could not read noise covariance from"
                     << parser.value(noisecovOpt);
        return 1;
    }

    printf("  Noise covariance: %d x %d\n",
           (int)noiseCov.data.rows(), (int)noiseCov.data.cols());
    if (!noiseCov.projs.isEmpty()) {
        printf("  Noise covariance contains %lld SSP projectors.\n",
               noiseCov.projs.size());
    }

    //=========================================================================================================
    // Read optional SSP projection files
    //=========================================================================================================

    QList<FiffProj> extraProjs;
    QStringList projFiles = parser.values(projOpt);
    for (const QString &projFileName : projFiles) {
        printf("Reading SSP projectors from %s...\n",
               projFileName.toUtf8().constData());
        QList<FiffProj> fileProjs = readProjFile(projFileName);
        printf("  Found %lld projectors.\n", fileProjs.size());
        extraProjs.append(fileProjs);
    }

    //=========================================================================================================
    // Build FiffInfo from forward solution + noise covariance projectors
    // (same approach as the original MNE-C tool — no measurement file needed)
    //=========================================================================================================

    printf("\nBuilding measurement info from forward solution...\n");
    FiffInfo info = buildInfoFromForward(forward, noiseCov, extraProjs);
    printf("  Info: %d channels, %lld projectors\n",
           info.nchan, info.projs.size());

    //=========================================================================================================
    // Merge bad channels from --bad files
    //=========================================================================================================

    QStringList badFiles = parser.values(badOpt);
    for (const QString &badFileName : badFiles) {
        printf("Reading bad channels from %s...\n",
               badFileName.toUtf8().constData());
        QStringList fileBads = readBadFile(badFileName);
        for (const QString &bad : fileBads) {
            if (!info.bads.contains(bad))
                info.bads << bad;
        }
    }
    // Also merge bad channels from the noise covariance
    for (const QString &bad : noiseCov.bads) {
        if (!info.bads.contains(bad))
            info.bads << bad;
    }
    if (!info.bads.isEmpty()) {
        printf("  Bad channels (%lld):", info.bads.size());
        for (const QString &bad : info.bads)
            printf(" %s", bad.toUtf8().constData());
        printf("\n");
    }

    //=========================================================================================================
    // Regularize noise covariance
    //=========================================================================================================

    printf("\nRegularizing noise covariance (mag=%.3f, grad=%.3f, eeg=%.3f)...\n",
           magReg, gradReg, eegReg);
    noiseCov = noiseCov.regularize(info, magReg, gradReg, eegReg, true);

    // If --diagnoise, zero out off-diagonal elements
    if (diagNoise) {
        printf("Using only diagonal noise covariance.\n");
        MatrixXd diagCov = MatrixXd::Zero(noiseCov.data.rows(), noiseCov.data.cols());
        diagCov.diagonal() = noiseCov.data.diagonal();
        noiseCov.data = diagCov;
    }

    //=========================================================================================================
    // Assemble the inverse operator
    //=========================================================================================================

    printf("\nAssembling inverse operator...\n");
    printf("  Fixed orientation: %s\n", useFixed ? "yes" : "no");
    if (!useFixed) {
        printf("  Loose constraint: %.2f\n", loose);
    }
    printf("  Depth weighting:  %s", depth > 0.0f ? "yes" : "no");
    if (depth > 0.0f) {
        printf(" (%.2f)", depth);
    }
    printf("\n");

    MNEInverseOperator invOp(info, forward, noiseCov, loose, depth, useFixed, true);

    if (invOp.nsource <= 0) {
        qCritical() << "Error: Failed to assemble inverse operator.";
        return 1;
    }

    printf("  Inverse operator: %d sources, %d channels\n",
           invOp.nsource, invOp.nchan);

    //=========================================================================================================
    // Write the inverse operator
    //=========================================================================================================

    printf("\nWriting inverse operator to %s...\n", invName.toUtf8().constData());

    QFile invFile(invName);
    invOp.write(invFile);

    printf("Done.\n\n");
    printf("The inverse operator is ready.\n");
    printf("You can now use mne_compute_mne or mne_compute_raw_inverse\n");
    printf("to compute source estimates.\n\n");

    return 0;
}
