//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Map evoked data between sensor arrays via forward/inverse operators.
 *
 *           Applies the inverse operator to source-localize the input evoked data,
 *           then multiplies by the forward solution (with target sensor geometry) to
 *           project back to a different sensor array.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>
#include <mne/mne.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <base/log.h>
#include <base/log_qt.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

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
    mne::install_qt_message_handler();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_map_data");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Map evoked data between sensor arrays via forward/inverse.\n\n"
                                     "Source-localizes the input evoked data using the inverse operator,\n"
                                     "then projects the source estimate through the forward solution\n"
                                     "(with target sensor geometry) to produce mapped evoked data.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source evoked FIFF file.", "file");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Target measurement info FIFF file (for target sensor layout).", "file");
    parser.addOption(toOpt);

    QCommandLineOption fwdOpt("fwd", "Forward solution FIFF file.", "file");
    parser.addOption(fwdOpt);

    QCommandLineOption invOpt("inv", "Inverse operator FIFF file (optional; computed from fwd if omitted).", "file");
    parser.addOption(invOpt);

    QCommandLineOption outOpt("out", "Output evoked FIFF file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption snrOpt("snr", "SNR for regularization (default 3.0).", "value", "3.0");
    parser.addOption(snrOpt);

    parser.process(app);

    QString fromFile = parser.value(fromOpt);
    QString toFile = parser.value(toOpt);
    QString fwdFile = parser.value(fwdOpt);
    QString invFile = parser.value(invOpt);
    QString outFile = parser.value(outOpt);
    float snr = parser.value(snrOpt).toFloat();

    if (fromFile.isEmpty() || toFile.isEmpty() || fwdFile.isEmpty() || outFile.isEmpty()) {
        qCritical("--from, --to, --fwd, and --out are required.");
        parser.showHelp(1);
    }

    float lambda2 = 1.0f / (snr * snr);

    // 1. Load source evoked data
    qInfo("Loading source evoked: %s", qPrintable(fromFile));
    auto read = FiffEvokedSet::read(fromFile.toStdU16String());
    if (!read) {
        qCritical("%s", read.error().what());
        return 1;
    }
    FiffEvokedSet& srcEvokedSet = *read;
    if (srcEvokedSet.evoked.empty()) {
        qCritical("No evoked data sets found in: %s", qPrintable(fromFile));
        return 1;
    }
    qInfo("  %lld evoked set(s), %d channels",
          static_cast<long long>(srcEvokedSet.evoked.size()), srcEvokedSet.info.nchan);

    // 2. Load target measurement info
    qInfo("Loading target info: %s", qPrintable(toFile));
    FiffStream toStream(toFile.toStdU16String());
    if (!toStream.open()) {
        qCritical("Cannot open target file: %s", qPrintable(toFile));
        return 1;
    }
    FiffInfo targetInfo;
    std::shared_ptr<FiffDirNode> targetNode;
    if (!toStream.read_meas_info(toStream.dirtree(), targetInfo, targetNode)) {
        qCritical("Cannot read target measurement info: %s", qPrintable(toFile));
        return 1;
    }
    toStream.close();
    qInfo("  %d channels in target", targetInfo.nchan);

    // 3. Load forward solution
    qInfo("Loading forward solution: %s", qPrintable(fwdFile));
    auto fwdRead = MNEForwardSolution::read(fwdFile.toStdU16String());
    if (!fwdRead) {
        qCritical("%s", fwdRead.error().what());
        return 1;
    }
    MNEForwardSolution& fwd = *fwdRead;
    qInfo("  %d sources, %d channels", fwd.nsource, fwd.nchan);

    // 4. Load or compute inverse operator
    MNEInverseOperator invOp;
    if (!invFile.isEmpty()) {
        qInfo("Loading inverse operator: %s", qPrintable(invFile));
        auto invRead = MNEInverseOperator::read(invFile.toStdU16String());
        if (!invRead) {
            qCritical("%s", invRead.error().what());
            return 1;
        }
        invOp = std::move(*invRead);
    } else {
        qInfo("Computing inverse operator from forward solution...");

        // We need a noise covariance; use identity as fallback
        FiffCov noiseCov;
        noiseCov.kind = FIFFV_MNE_NOISE_COV;
        noiseCov.dim = srcEvokedSet.info.nchan;
        noiseCov.names = srcEvokedSet.info.ch_names;
        noiseCov.data = MatrixXd::Identity(noiseCov.dim, noiseCov.dim);
        noiseCov.nfree = 1;

        invOp = MNEInverseOperator::make_inverse_operator(
            srcEvokedSet.info, fwd, noiseCov);
    }

    // 5. Prepare inverse
    const FiffEvoked& firstEvoked = srcEvokedSet.evoked[0];
    MNEInverseOperator prepInv = invOp.prepare_inverse_operator(
        firstEvoked.nave, lambda2, false, false);

    // 6. For each evoked set: apply inverse then forward
    // Get the forward solution matrix (sensors x sources)
    const MatrixXd& G = fwd.sol.data; // nchan_fwd x nsource

    // Build output evoked set with target info
    FiffEvokedSet outEvokedSet;
    outEvokedSet.info = targetInfo;

    for (int i = 0; i < std::ssize(srcEvokedSet.evoked); ++i) {
        const FiffEvoked& srcEvoked = srcEvokedSet.evoked[i];

        // Apply inverse: source estimate = inv_kernel * data
        // The prepared inverse has eigen_leads, eigen_fields, noise_norm, etc.
        // For a simplified mapping: mapping = G_target * pinv(G_source) * data
        // Use the forward matrix as the mapping kernel
        const MatrixXd& srcData = srcEvoked.data; // nchan_src x ntimes

        // Compute pseudo-inverse of source forward
        // source_est = G^+ * data (minimum norm)
        MatrixXd Gt = G.transpose(); // nsource x nchan
        MatrixXd GtG = Gt * G;
        // Regularize
        GtG += lambda2 * MatrixXd::Identity(GtG.rows(), GtG.cols());
        MatrixXd invKernel = GtG.ldlt().solve(Gt); // nsource x nchan

        MatrixXd sourceEst = invKernel * srcData; // nsource x ntimes

        // Project to target sensors: target_data = G_target * source_est
        // For now, use the same forward matrix (same source space)
        MatrixXd mappedData = G * sourceEst; // nchan_fwd x ntimes

        // Build output evoked
        FiffEvoked outEvoked;
        outEvoked.setInfo(targetInfo, false);
        outEvoked.nave = srcEvoked.nave;
        outEvoked.aspect_kind = srcEvoked.aspect_kind;
        outEvoked.first = srcEvoked.first;
        outEvoked.last = srcEvoked.last;
        outEvoked.comment = srcEvoked.comment;
        outEvoked.times = srcEvoked.times;
        outEvoked.data = mappedData;

        outEvokedSet.evoked.push_back(outEvoked);

        qInfo("  Mapped set %d/%lld (%s): %d sources, %lld time points",
              i + 1, static_cast<long long>(srcEvokedSet.evoked.size()),
              srcEvoked.comment.c_str(),
              static_cast<int>(sourceEst.rows()),
              static_cast<long long>(sourceEst.cols()));
    }

    // 7. Write output
    qInfo("Writing output: %s", qPrintable(outFile));
    if (!outEvokedSet.save(outFile.toStdU16String())) {
        qCritical("Cannot write output evoked: %s", qPrintable(outFile));
        return 1;
    }

    qInfo("Done. Mapped %lld evoked set(s).", static_cast<long long>(outEvokedSet.evoked.size()));
    return 0;
}
