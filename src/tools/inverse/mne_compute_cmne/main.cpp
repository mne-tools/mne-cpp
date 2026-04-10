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
 *
 * @brief    Implements mne_compute_cmne application.
 *
 *           Computes Contextual Minimum-Norm Estimate (CMNE) source time
 *           courses from evoked data, or trains / fine-tunes the LSTM model.
 *
 *           Three modes of operation:
 *             compute   – Apply dSPM + LSTM correction → write STC files
 *             train     – Train CMNE LSTM from FIFF files → export ONNX
 *             finetune  – Continue training from an existing model
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_cov.h>

#include <mne/mne_forward_solution.h>

#include <inv/inv_source_estimate.h>
#include <inv/minimum_norm/inv_cmne.h>
#include <inv/minimum_norm/inv_cmne_settings.h>

#include <ml/ml_trainer.h>
#include <utils/python_runner.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
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
using namespace INVLIB;
using namespace UTILSLIB;
using namespace MLLIB;

//=============================================================================================================
// HELPERS
//=============================================================================================================

static int methodStringToInt(const QString& method)
{
    if (method.compare(QLatin1String("MNE"),     Qt::CaseInsensitive) == 0) return 0;
    if (method.compare(QLatin1String("dSPM"),    Qt::CaseInsensitive) == 0) return 1;
    if (method.compare(QLatin1String("sLORETA"), Qt::CaseInsensitive) == 0) return 2;
    if (method.compare(QLatin1String("eLORETA"), Qt::CaseInsensitive) == 0) return 3;
    return 1; // default dSPM
}

//=============================================================================================================

static QString resolveScriptDir()
{
    const QString relPath = QStringLiteral("scripts/ml/training/cmne");
    const QString marker  = QStringLiteral("pyproject.toml");

    // 1) Binary lives in <root>/out/<config>/bin/ → go up 4 levels to repo root
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    for (int i = 0; i < 5; ++i) {
        QString candidate = dir.absoluteFilePath(relPath);
        if (QFile::exists(QDir(candidate).absoluteFilePath(marker)))
            return candidate;
        if (!dir.cdUp())
            break;
    }

    // 2) Relative to working directory (works when cwd is repo root)
    if (QFile::exists(QDir(relPath).absoluteFilePath(marker)))
        return QDir(relPath).absolutePath();

    return {};
}

//=============================================================================================================
// COMPUTE MODE
//=============================================================================================================

static int doCompute(const QCommandLineParser& parser)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    // ── Validate required arguments ────────────────────────────────────
    const QString fwdPath    = parser.value(QStringLiteral("fwd"));
    const QString covPath    = parser.value(QStringLiteral("cov"));
    const QString evokedPath = parser.value(QStringLiteral("evoked"));
    const QString outPrefix  = parser.value(QStringLiteral("out"));

    if (fwdPath.isEmpty() || covPath.isEmpty() || evokedPath.isEmpty()) {
        err << "Error: --fwd, --cov, and --evoked are required for compute mode.\n";
        return 1;
    }
    if (outPrefix.isEmpty()) {
        err << "Error: --out is required for compute mode.\n";
        return 1;
    }

    // ── Read forward solution ──────────────────────────────────────────
    out << "Reading forward solution: " << fwdPath << "\n";
    out.flush();
    QFile fwdFile(fwdPath);
    MNEForwardSolution fwd(fwdFile, true, true);  // force_fixed=true, surf_ori=true
    if (fwd.isEmpty()) {
        err << "Error: Failed to read forward solution from " << fwdPath << "\n";
        return 1;
    }

    const MatrixXd matGain = fwd.sol->data;
    const int nChannels = matGain.rows();
    const int nSources  = matGain.cols();
    out << "  " << nChannels << " channels x " << nSources << " sources\n";

    // ── Read noise covariance ──────────────────────────────────────────
    out << "Reading noise covariance: " << covPath << "\n";
    out.flush();
    QFile covFile(covPath);
    FiffCov noiseCov(covFile);
    if (noiseCov.isEmpty()) {
        err << "Error: Failed to read noise covariance from " << covPath << "\n";
        return 1;
    }

    const MatrixXd matNoiseCov = noiseCov.data;
    out << "  " << matNoiseCov.rows() << " x " << matNoiseCov.cols() << "\n";

    // ── Read evoked data ───────────────────────────────────────────────
    const int setno = parser.value(QStringLiteral("setno")).toInt();
    out << "Reading evoked data: " << evokedPath << " (set " << setno << ")\n";
    out.flush();
    QFile evokedFile(evokedPath);
    FiffEvoked evoked(evokedFile, setno);
    if (evoked.isEmpty()) {
        err << "Error: Failed to read evoked data from " << evokedPath << "\n";
        return 1;
    }

    const float tmin  = evoked.times(0);
    const float tstep = (evoked.times.size() > 1)
                        ? (evoked.times(1) - evoked.times(0))
                        : 1.0f;

    // ── Pick channels to match forward solution ────────────────────────
    // The evoked file may contain extra channels (STIM, EOG, ECG, …)
    // that are not in the forward solution.  Subset to forward channels.
    const QStringList fwdChNames = fwd.sol->row_names;
    FiffEvoked evokedPicked = evoked.pick_channels(fwdChNames);
    FiffCov    noiseCovPicked = noiseCov.pick_channels(fwdChNames);

    const MatrixXd matEvoked   = evokedPicked.data;
    const MatrixXd matNoiseCovPicked = noiseCovPicked.data;

    out << "  " << evoked.data.rows() << " channels in file, "
        << matEvoked.rows() << " channels after picking ("
        << matEvoked.cols() << " samples, tmin=" << tmin
        << " s, tstep=" << tstep * 1000.0f << " ms)\n";

    // ── Source covariance (identity) ───────────────────────────────────
    MatrixXd matSrcCov = MatrixXd::Identity(nSources, nSources);

    // ── Configure CMNE settings ────────────────────────────────────────
    const double snr = parser.value(QStringLiteral("snr")).toDouble();
    const QString methodStr = parser.value(QStringLiteral("method"));
    const int lookBack = parser.value(QStringLiteral("look-back")).toInt();
    const QString onnxPath = parser.value(QStringLiteral("onnx"));

    InvCMNESettings settings;
    settings.lambda2     = 1.0 / (snr * snr);
    settings.method      = methodStringToInt(methodStr);
    settings.lookBack    = lookBack;
    settings.numSources  = nSources;
    settings.onnxModelPath = onnxPath;

    out << "Settings: method=" << methodStr << ", SNR=" << snr
        << ", lambda2=" << settings.lambda2
        << ", look-back=" << lookBack << "\n";

    if (!onnxPath.isEmpty()) {
        if (!QFile::exists(onnxPath)) {
            err << "Warning: ONNX model not found: " << onnxPath
                << " — falling back to moving-average correction.\n";
        } else {
            out << "ONNX model: " << onnxPath << "\n";
        }
    } else {
        out << "\n"
            << "NOTE: No ONNX model specified (--onnx).\n"
            << "  The CMNE correction will use a moving-average approximation instead\n"
            << "  of the trained LSTM. For proper CMNE results, train a model first:\n"
            << "\n"
            << "    mne_compute_cmne --mode train --fwd <fwd.fif> --cov <cov.fif> \\\n"
            << "                     --epochs <epo.fif> --onnx-out cmne_lstm.onnx\n"
            << "\n"
            << "  Then re-run compute with:\n"
            << "\n"
            << "    mne_compute_cmne --mode compute --fwd <fwd.fif> --cov <cov.fif> \\\n"
            << "                     --evoked <ave.fif> --onnx cmne_lstm.onnx --out <prefix>\n"
            << "\n";
    }

    // ── Compute CMNE ───────────────────────────────────────────────────
    out << "Computing CMNE inverse solution …\n";
    out.flush();

    InvCMNEResult result = InvCMNE::compute(
        matEvoked, matGain, matNoiseCovPicked, matSrcCov, settings);

    // Propagate timing from evoked data
    result.stcDspm.tmin  = tmin;
    result.stcDspm.tstep = tstep;
    result.stcCmne.tmin  = tmin;
    result.stcCmne.tstep = tstep;

    out << "  dSPM: " << result.stcDspm.data.rows() << " sources x "
        << result.stcDspm.data.cols() << " samples\n";
    out << "  CMNE: " << result.stcCmne.data.rows() << " sources x "
        << result.stcCmne.data.cols() << " samples\n";

    // ── Write STC files ────────────────────────────────────────────────
    const QString dspmStcPath = outPrefix + QStringLiteral("-dspm.stc");
    const QString cmneStcPath = outPrefix + QStringLiteral("-cmne.stc");

    QFile dspmFile(dspmStcPath);
    if (!result.stcDspm.write(dspmFile)) {
        err << "Error: Failed to write dSPM STC.\n";
        return 1;
    }
    out << "Wrote dSPM STC: " << dspmStcPath << "\n";

    QFile cmneFile(cmneStcPath);
    if (!result.stcCmne.write(cmneFile)) {
        err << "Error: Failed to write CMNE STC.\n";
        return 1;
    }
    out << "Wrote CMNE STC: " << cmneStcPath << "\n";

    out << "Done.\n";
    return 0;
}

//=============================================================================================================
// TRAIN / FINETUNE MODE
//=============================================================================================================

static int doTrain(const QCommandLineParser& parser, bool finetune)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    // ── Validate required arguments ────────────────────────────────────
    const QString fwdPath     = parser.value(QStringLiteral("fwd"));
    const QString covPath     = parser.value(QStringLiteral("cov"));
    const QString epochsPath  = parser.value(QStringLiteral("epochs"));
    const QString onnxOutPath = parser.value(QStringLiteral("onnx-out"));

    if (fwdPath.isEmpty() || covPath.isEmpty() || epochsPath.isEmpty()) {
        err << "Error: --fwd, --cov, and --epochs are required for "
            << (finetune ? "finetune" : "train") << " mode.\n";
        return 1;
    }
    if (onnxOutPath.isEmpty()) {
        err << "Error: --onnx-out is required for "
            << (finetune ? "finetune" : "train") << " mode.\n";
        return 1;
    }

    const QString finetuneOnnxPath = finetune
        ? parser.value(QStringLiteral("finetune"))
        : QString();

    if (finetune && finetuneOnnxPath.isEmpty()) {
        err << "Error: --finetune <onnx_path> is required in finetune mode.\n";
        return 1;
    }

    if (finetune && !QFile::exists(finetuneOnnxPath)) {
        err << "Error: Fine-tune model not found: " << finetuneOnnxPath << "\n";
        return 1;
    }

    // ── Resolve training script ────────────────────────────────────────
    QString cmneDir = resolveScriptDir();
    if (cmneDir.isEmpty()) {
        err << "Error: Cannot find CMNE training package (scripts/ml/training/cmne/).\n";
        return 1;
    }

    QString scriptPath = QDir(cmneDir).absoluteFilePath(
        QStringLiteral("train_cmne_lstm.py"));
    if (!QFile::exists(scriptPath)) {
        err << "Error: Training script not found: " << scriptPath << "\n";
        return 1;
    }

    // ── Read settings from CLI ─────────────────────────────────────────
    const double snr         = parser.value(QStringLiteral("snr")).toDouble();
    const QString methodStr  = parser.value(QStringLiteral("method"));
    const int lookBack       = parser.value(QStringLiteral("look-back")).toInt();
    const int hiddenSize     = parser.value(QStringLiteral("hidden")).toInt();
    const int numLayers      = parser.value(QStringLiteral("layers")).toInt();
    const int trainEpochs    = parser.value(QStringLiteral("train-epochs")).toInt();
    const double lr          = parser.value(QStringLiteral("lr")).toDouble();
    const int batchSize      = parser.value(QStringLiteral("batch")).toInt();
    const QString gtStcPfx   = parser.value(QStringLiteral("gt-stc"));
    const QString pythonExe  = parser.value(QStringLiteral("python"));

    // ── Build Python script arguments ──────────────────────────────────
    QStringList args;
    args << QStringLiteral("--fwd")          << fwdPath
         << QStringLiteral("--cov")          << covPath
         << QStringLiteral("--epochs")       << epochsPath
         << QStringLiteral("--out")          << onnxOutPath
         << QStringLiteral("--look-back")    << QString::number(lookBack)
         << QStringLiteral("--method")       << methodStr
         << QStringLiteral("--snr")          << QString::number(snr, 'g', 6)
         << QStringLiteral("--hidden")       << QString::number(hiddenSize)
         << QStringLiteral("--layers")       << QString::number(numLayers)
         << QStringLiteral("--train-epochs") << QString::number(trainEpochs)
         << QStringLiteral("--lr")           << QString::number(lr, 'g', 6)
         << QStringLiteral("--batch")        << QString::number(batchSize);

    if (!gtStcPfx.isEmpty()) {
        args << QStringLiteral("--gt-stc") << gtStcPfx;
    }

    if (finetune) {
        args << QStringLiteral("--finetune") << finetuneOnnxPath;
    }

    // ── Configure PythonRunner with venv ───────────────────────────────
    PythonRunnerConfig config;
    config.pythonExe  = pythonExe;
    config.venvDir    = QDir(cmneDir).absoluteFilePath(QStringLiteral(".venv"));
    config.packageDir = cmneDir;

    MLTrainer trainer(config);

    // ── Set up live progress reporting ─────────────────────────────────
    trainer.runner().setLineCallback([&out, &err](int channel, const QString& line) {
        if (channel == 0) {
            out << line << "\n";
            out.flush();
        } else {
            err << line << "\n";
            err.flush();
        }
    });

    trainer.runner().setProgressCallback([&out](float pct, const QString& msg) {
        out << "\r[" << QString::number(static_cast<double>(pct), 'f', 1) << "%] " << msg;
        out.flush();
    });

    // ── Print summary ──────────────────────────────────────────────────
    out << "=== CMNE LSTM " << (finetune ? "Fine-tuning" : "Training") << " ===\n";
    out << "  Forward   : " << fwdPath << "\n";
    out << "  Covariance: " << covPath << "\n";
    out << "  Epochs    : " << epochsPath << "\n";
    out << "  Output    : " << onnxOutPath << "\n";
    out << "  Method    : " << methodStr << "  SNR=" << snr << "\n";
    out << "  LSTM      : hidden=" << hiddenSize << "  layers=" << numLayers
        << "  look-back=" << lookBack << "\n";
    out << "  Training  : epochs=" << trainEpochs << "  lr=" << lr
        << "  batch=" << batchSize << "\n";
    if (finetune) {
        out << "  Fine-tune : " << finetuneOnnxPath << "\n";
    }
    if (!gtStcPfx.isEmpty()) {
        out << "  Ground truth: " << gtStcPfx << "\n";
    }
    out << "  Script    : " << scriptPath << "\n";
    out << "  Venv      : " << config.venvDir << "\n";
    out << "\n";
    out.flush();

    // ── Run training ───────────────────────────────────────────────────
    PythonRunnerResult result = trainer.run(scriptPath, args);

    out << "\n";

    if (result.success) {
        out << "Training completed successfully.\n";
        if (QFile::exists(onnxOutPath)) {
            QFileInfo fi(onnxOutPath);
            out << "ONNX model: " << fi.absoluteFilePath()
                << " (" << fi.size() / 1024 << " KB)\n";
        }
    } else {
        err << "Training failed";
        if (result.timedOut) {
            err << " (timed out)";
        }
        err << " with exit code " << result.exitCode << ".\n";

        if (!result.stdErr.isEmpty()) {
            err << "--- stderr ---\n" << result.stdErr << "\n";
        }
    }

    return result.success ? 0 : 1;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mne_compute_cmne"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MNE_CPP_VERSION));

    // ── Command-line parser ────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Contextual MNE (CMNE) — compute source estimates or train the LSTM model.\n\n"
                       "Modes:\n"
                       "  compute   Compute dSPM + CMNE source estimates from evoked data (default)\n"
                       "  train     Train the CMNE LSTM from FIFF files and export to ONNX\n"
                       "  finetune  Continue training from an existing model"));
    parser.addHelpOption();
    parser.addVersionOption();

    // -- Mode --
    parser.addOption({QStringLiteral("mode"),
        QStringLiteral("Operation mode: compute, train, or finetune."),
        QStringLiteral("mode"), QStringLiteral("compute")});

    // -- Common data inputs --
    parser.addOption({QStringLiteral("fwd"),
        QStringLiteral("Forward solution FIFF file."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("cov"),
        QStringLiteral("Noise covariance FIFF file."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("snr"),
        QStringLiteral("Signal-to-noise ratio (default: 3.0)."),
        QStringLiteral("value"), QStringLiteral("3.0")});
    parser.addOption({QStringLiteral("method"),
        QStringLiteral("Inverse method: MNE, dSPM, sLORETA, eLORETA (default: dSPM)."),
        QStringLiteral("name"), QStringLiteral("dSPM")});
    parser.addOption({QStringLiteral("look-back"),
        QStringLiteral("Number of past time steps k (default: 80)."),
        QStringLiteral("k"), QStringLiteral("80")});

    // -- Compute-mode options --
    parser.addOption({QStringLiteral("evoked"),
        QStringLiteral("Evoked data FIFF file (compute mode)."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("onnx"),
        QStringLiteral("ONNX model for LSTM correction (compute mode)."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("out"),
        QStringLiteral("Output STC prefix; writes <prefix>-dspm.stc and <prefix>-cmne.stc."),
        QStringLiteral("prefix")});
    parser.addOption({QStringLiteral("setno"),
        QStringLiteral("Evoked data set number (default: 0)."),
        QStringLiteral("n"), QStringLiteral("0")});

    // -- Train / finetune options --
    parser.addOption({QStringLiteral("epochs"),
        QStringLiteral("MNE Epochs FIFF file (train/finetune mode)."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("gt-stc"),
        QStringLiteral("Ground-truth STC prefix (optional; omit for pseudo-GT mode)."),
        QStringLiteral("prefix")});
    parser.addOption({QStringLiteral("onnx-out"),
        QStringLiteral("Output ONNX model path (train/finetune mode)."),
        QStringLiteral("file"), QStringLiteral("cmne_lstm.onnx")});
    parser.addOption({QStringLiteral("hidden"),
        QStringLiteral("LSTM hidden dimension (default: 256)."),
        QStringLiteral("n"), QStringLiteral("256")});
    parser.addOption({QStringLiteral("layers"),
        QStringLiteral("Number of LSTM layers (default: 1)."),
        QStringLiteral("n"), QStringLiteral("1")});
    parser.addOption({QStringLiteral("train-epochs"),
        QStringLiteral("Number of training epochs (default: 50)."),
        QStringLiteral("n"), QStringLiteral("50")});
    parser.addOption({QStringLiteral("lr"),
        QStringLiteral("Learning rate (default: 0.001)."),
        QStringLiteral("value"), QStringLiteral("0.001")});
    parser.addOption({QStringLiteral("batch"),
        QStringLiteral("Batch size (default: 64)."),
        QStringLiteral("n"), QStringLiteral("64")});
    parser.addOption({QStringLiteral("finetune"),
        QStringLiteral("Existing ONNX model to fine-tune from."),
        QStringLiteral("file")});
    parser.addOption({QStringLiteral("python"),
        QStringLiteral("Python interpreter (default: python3)."),
        QStringLiteral("exe"), QStringLiteral("python3")});

    parser.process(app);

    // ── Dispatch ───────────────────────────────────────────────────────
    const QString mode = parser.value(QStringLiteral("mode")).toLower();

    if (mode == QLatin1String("compute")) {
        return doCompute(parser);
    } else if (mode == QLatin1String("train")) {
        return doTrain(parser, false);
    } else if (mode == QLatin1String("finetune")) {
        return doTrain(parser, true);
    } else {
        QTextStream(stderr) << "Error: Unknown mode '" << mode
                            << "'. Use compute, train, or finetune.\n";
        return 1;
    }
}
