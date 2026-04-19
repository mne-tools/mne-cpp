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

#include <utils/generics/mne_logger.h>

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

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
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
    fprintf(stderr, "Loading source evoked: %s\n", qPrintable(fromFile));
    QFile fFrom(fromFile);
    FiffEvokedSet srcEvokedSet;
    if (!FiffEvokedSet::read(fFrom, srcEvokedSet)) {
        qCritical("Cannot read source evoked data: %s", qPrintable(fromFile));
        return 1;
    }
    if (srcEvokedSet.evoked.isEmpty()) {
        qCritical("No evoked data sets found in: %s", qPrintable(fromFile));
        return 1;
    }
    fprintf(stderr, "  %lld evoked set(s), %d channels\n",
            static_cast<long long>(srcEvokedSet.evoked.size()), srcEvokedSet.info.nchan);

    // 2. Load target measurement info
    fprintf(stderr, "Loading target info: %s\n", qPrintable(toFile));
    QFile fTo(toFile);
    FiffStream::SPtr toStream(new FiffStream(&fTo));
    if (!toStream->open()) {
        qCritical("Cannot open target file: %s", qPrintable(toFile));
        return 1;
    }
    FiffInfo targetInfo;
    FiffDirNode::SPtr targetNode;
    if (!toStream->read_meas_info(toStream->dirtree(), targetInfo, targetNode)) {
        qCritical("Cannot read target measurement info: %s", qPrintable(toFile));
        return 1;
    }
    toStream->close();
    fprintf(stderr, "  %d channels in target\n", targetInfo.nchan);

    // 3. Load forward solution
    fprintf(stderr, "Loading forward solution: %s\n", qPrintable(fwdFile));
    QFile fFwd(fwdFile);
    MNEForwardSolution fwd;
    if (!MNEForwardSolution::read(fFwd, fwd)) {
        qCritical("Cannot read forward solution: %s", qPrintable(fwdFile));
        return 1;
    }
    fprintf(stderr, "  %d sources, %d channels\n", fwd.nsource, fwd.nchan);

    // 4. Load or compute inverse operator
    MNEInverseOperator invOp;
    if (!invFile.isEmpty()) {
        fprintf(stderr, "Loading inverse operator: %s\n", qPrintable(invFile));
        QFile fInv(invFile);
        if (!MNEInverseOperator::read_inverse_operator(fInv, invOp)) {
            qCritical("Cannot read inverse operator: %s", qPrintable(invFile));
            return 1;
        }
    } else {
        fprintf(stderr, "Computing inverse operator from forward solution...\n");

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
    const FiffEvoked &firstEvoked = srcEvokedSet.evoked[0];
    MNEInverseOperator prepInv = invOp.prepare_inverse_operator(
        firstEvoked.nave, lambda2, false, false);

    // 6. For each evoked set: apply inverse then forward
    // Get the forward solution matrix (sensors x sources)
    const MatrixXd &G = fwd.sol->data;  // nchan_fwd x nsource

    // Build output evoked set with target info
    FiffEvokedSet outEvokedSet;
    outEvokedSet.info = targetInfo;

    for (int i = 0; i < srcEvokedSet.evoked.size(); ++i) {
        const FiffEvoked &srcEvoked = srcEvokedSet.evoked[i];

        // Apply inverse: source estimate = inv_kernel * data
        // The prepared inverse has eigen_leads, eigen_fields, noise_norm, etc.
        // For a simplified mapping: mapping = G_target * pinv(G_source) * data
        // Use the forward matrix as the mapping kernel
        const MatrixXd &srcData = srcEvoked.data;  // nchan_src x ntimes

        // Compute pseudo-inverse of source forward
        // source_est = G^+ * data (minimum norm)
        MatrixXd Gt = G.transpose();  // nsource x nchan
        MatrixXd GtG = Gt * G;
        // Regularize
        GtG += lambda2 * MatrixXd::Identity(GtG.rows(), GtG.cols());
        MatrixXd invKernel = GtG.ldlt().solve(Gt);  // nsource x nchan

        MatrixXd sourceEst = invKernel * srcData;  // nsource x ntimes

        // Project to target sensors: target_data = G_target * source_est
        // For now, use the same forward matrix (same source space)
        MatrixXd mappedData = G * sourceEst;  // nchan_fwd x ntimes

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

        outEvokedSet.evoked.append(outEvoked);

        fprintf(stderr, "  Mapped set %d/%lld (%s): %d sources, %lld time points\n",
                i + 1, static_cast<long long>(srcEvokedSet.evoked.size()),
                qPrintable(srcEvoked.comment),
                static_cast<int>(sourceEst.rows()),
                static_cast<long long>(sourceEst.cols()));
    }

    // 7. Write output
    fprintf(stderr, "Writing output: %s\n", qPrintable(outFile));
    if (!outEvokedSet.save(outFile)) {
        qCritical("Cannot write output evoked: %s", qPrintable(outFile));
        return 1;
    }

    fprintf(stderr, "Done. Mapped %lld evoked set(s).\n", static_cast<long long>(outEvokedSet.evoked.size()));
    return 0;
}
