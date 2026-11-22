//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements mne_compute_mne application.
 *           Port of the original MNE-C mne_compute_mne by Matti Hamalainen.
 *
 *           Computes MNE, dSPM, or sLORETA source estimates from evoked data
 *           using a pre-computed inverse operator.  Outputs STC files.
 *
 *           Supports advanced options from the original SVN MNE-C tool including
 *           time-collapsing (max/L1/L2), output scaling, baseline correction,
 *           forward-as-data mode, label-based output, and measurement ID matching.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <base/string_utils.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_forward_solution.h>
#include <inv/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>

#include <inv/minimum_norm/inv_minimum_norm.h>

#include <fs/fs_label.h>

#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

// Collapse modes (matching SVN MNE-C)
constexpr auto COLLAPSE_MAX = 1;
constexpr auto COLLAPSE_L1 = 2;
constexpr auto COLLAPSE_L2 = 3;

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * @brief Collapse source estimate data across time to a single frame.
 *
 * Matches SVN MNE-C collapse_data() semantics:
 *   COLLAPSE_MAX: keep value with maximum absolute value (preserving sign)
 *   COLLAPSE_L1:  mean absolute value across time
 *   COLLAPSE_L2:  RMS (root mean square) across time
 *
 * The collapsed single-column result replaces all columns (replicated).
 *
 * @param[in,out] data   Source data matrix (nSources x nTimes), modified in place.
 * @param[in]     mode   Collapse mode: COLLAPSE_MAX, COLLAPSE_L1, or COLLAPSE_L2.
 */
static void collapseData(MatrixXd& data, int mode)
{
    int nSources = data.rows();
    int nTimes = data.cols();
    VectorXd collapsed(nSources);

    for (int s = 0; s < nSources; ++s) {
        switch (mode) {
            case COLLAPSE_MAX: {
                // Keep value with maximum absolute value (preserving sign)
                int maxIdx = 0;
                double maxAbs = 0.0;
                for (int t = 0; t < nTimes; ++t) {
                    double absVal = std::abs(data(s, t));
                    if (absVal > maxAbs) {
                        maxAbs = absVal;
                        maxIdx = t;
                    }
                }
                collapsed(s) = data(s, maxIdx);
                break;
            }
            case COLLAPSE_L1:
                // Mean absolute value
                collapsed(s) = data.row(s).cwiseAbs().sum() / nTimes;
                break;
            case COLLAPSE_L2:
                // Root mean square
                collapsed(s) = std::sqrt(data.row(s).squaredNorm() / nTimes);
                break;
        }
    }

    // Replicate collapsed column to all time points
    for (int t = 0; t < nTimes; ++t) {
        data.col(t) = collapsed;
    }
}

/**
 * @brief Scale source estimate data.
 *
 * Matches SVN MNE-C scale_currents():
 *   scaleTo > 0: scale so global max abs = scaleTo
 *   scaleBy != 0: multiply all data by scaleBy
 *   siCurrents: no scaling (raw SI-unit currents)
 *
 * @param[in,out] data        Source data matrix, modified in place.
 * @param[in]     scaleTo     Scale so max abs value = this (default 50.0, 0 = disabled).
 * @param[in]     scaleBy     Multiply all data by this factor (0 = disabled).
 * @param[in]     siCurrents  If true, no scaling at all.
 */
static void scaleData(MatrixXd& data, double scaleTo, double scaleBy, bool siCurrents)
{
    if (siCurrents) {
        Log::info("  Output: SI-unit currents (no scaling).");
        return;
    }
    if (scaleBy != 0.0) {
        Log::info("  Scaling data by {:.6e}", scaleBy);
        data *= scaleBy;
        return;
    }
    if (scaleTo > 0.0) {
        double maxAbs = data.cwiseAbs().maxCoeff();
        if (maxAbs > 0.0) {
            double factor = scaleTo / maxAbs;
            Log::info("  Scaling data so max = {:.1f} (factor = {:.6e})", scaleTo, factor);
            data *= factor;
        }
    }
}

/**
 * @brief Read per-channel baseline values from a text file.
 *
 * File format: one line per channel with "channel_name baseline_value".
 * Lines starting with '#' are ignored.
 *
 * @param[in]  fileName    Path to the baselines file.
 * @param[in]  ch_names    Channel names from the evoked data.
 * @param[out] baselines   Baseline values per channel (0 for channels not found).
 * @return true on success.
 */
static bool readBaselines(const QString& fileName,
                          const StringList& ch_names,
                          VectorXd& baselines)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::warning("  WARNING: Could not open baselines file {}", fileName.toStdString());
        return false;
    }

    baselines = VectorXd::Zero(static_cast<Eigen::Index>(ch_names.size()));
    QTextStream in(&file);
    int nFound = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 2) {
            const int idx = mne::channel_index(ch_names, parts[0].toStdString());
            if (idx >= 0) {
                baselines(idx) = parts[1].toDouble();
                nFound++;
            }
        }
    }
    file.close();
    Log::info("  Read {} baseline values from {}", nFound, fileName.toStdString());
    return true;
}

/**
 * @brief Write label-based ASCII output for source estimates.
 *
 * Matches SVN MNE-C process_labels() output format.
 *
 * @param[in] label       The label with vertex indices and positions.
 * @param[in] stcData     Full source estimate data (nSources x nTimes).
 * @param[in] vertno      Vertex numbers for the relevant hemisphere.
 * @param[in] hemiOffset  Row offset in stcData for this hemisphere.
 * @param[in] times       Time vector (in seconds).
 * @param[in] outputFile  Output file path.
 * @param[in] labelCoords Include source coordinates.
 * @param[in] timeBytime  Time-by-time layout (transpose).
 * @return true on success.
 */
static bool writeLabelOutput(const FsLabel& label,
                             const MatrixXd& stcData,
                             const VectorXi& vertno,
                             int hemiOffset,
                             const VectorXd& times,
                             const QString& outputFile,
                             bool labelCoords,
                             bool timeBytime)
{
    // Find label vertices in the source space
    QVector<int> srcIdx;       // indices into stcData rows
    QVector<int> labelVertIdx; // indices into label.vertices

    for (int lv = 0; lv < label.vertices.size(); ++lv) {
        int vno = label.vertices(lv);
        for (int sv = 0; sv < vertno.size(); ++sv) {
            if (vertno(sv) == vno) {
                srcIdx.append(hemiOffset + sv);
                labelVertIdx.append(lv);
                break;
            }
        }
    }

    if (srcIdx.isEmpty()) {
        Log::info("  WARNING: No vertices found in label {} for this hemisphere.", label.name);
        return false;
    }

    QFile file(outputFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::warning("  WARNING: Could not open label output file {}", outputFile.toStdString());
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(6);

    int nTimes = stcData.cols();

    if (timeBytime) {
        // Time-by-time layout: rows = time, columns = vertices
        // Header with vertex numbers
        if (labelCoords) {
            out << "# Vertex:";
            for (int i = 0; i < srcIdx.size(); ++i)
                out << "\t" << label.vertices(labelVertIdx[i]);
            out << "\n";
            out << "# X(mm):";
            for (int i = 0; i < srcIdx.size(); ++i)
                out << "\t" << label.pos(labelVertIdx[i], 0) * 1000.0;
            out << "\n";
            out << "# Y(mm):";
            for (int i = 0; i < srcIdx.size(); ++i)
                out << "\t" << label.pos(labelVertIdx[i], 1) * 1000.0;
            out << "\n";
            out << "# Z(mm):";
            for (int i = 0; i < srcIdx.size(); ++i)
                out << "\t" << label.pos(labelVertIdx[i], 2) * 1000.0;
            out << "\n";
        }
        // Data rows: time value1 value2 ...
        for (int t = 0; t < nTimes; ++t) {
            out << times(t) * 1000.0;
            for (int i = 0; i < srcIdx.size(); ++i)
                out << "\t" << stcData(srcIdx[i], t);
            out << "\n";
        }
    } else {
        // Point-by-point layout (default): rows = vertices, columns = time
        // First row: times
        out << "# Time(ms):";
        for (int t = 0; t < nTimes; ++t)
            out << "\t" << times(t) * 1000.0;
        out << "\n";

        // Data rows: vertex [x y z] value1 value2 ...
        for (int i = 0; i < srcIdx.size(); ++i) {
            out << label.vertices(labelVertIdx[i]);
            if (labelCoords) {
                out << "\t" << label.pos(labelVertIdx[i], 0) * 1000.0
                    << "\t" << label.pos(labelVertIdx[i], 1) * 1000.0
                    << "\t" << label.pos(labelVertIdx[i], 2) * 1000.0;
            }
            for (int t = 0; t < nTimes; ++t)
                out << "\t" << stcData(srcIdx[i], t);
            out << "\n";
        }
    }

    file.close();
    Log::info("  FsLabel output: {} vertices, {} times -> {}", static_cast<int>(srcIdx.size()), nTimes, outputFile.toStdString());
    return true;
}

/**
 * @brief Write a dipole snapshot file at a specific time.
 *
 * Matches SVN MNE-C write_mne_dip() output format:
 * Each source is a dipole with position, orientation, and amplitude.
 *
 * @param[in] fileName    Output file path.
 * @param[in] stcData     Full source estimate data (nSources x nTimes).
 * @param[in] src          Source space (for vertex positions and normals).
 * @param[in] vertno      Vertex numbers per hemisphere.
 * @param[in] timeIdx     Time index to extract.
 * @param[in] time_ms     Time value in ms (for header).
 * @return true on success.
 */
static bool writeDipFile(const QString& fileName,
                         const MatrixXd& stcData,
                         const MNESourceSpaces& src,
                         const std::vector<VectorXi>& vertno,
                         int timeIdx,
                         double time_ms)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::warning("  WARNING: Could not open dipole output file {}", fileName.toStdString());
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(6);

    out << "# Dipole snapshot at " << time_ms << " ms\n";
    out << "# vertex x(m) y(m) z(m) nx ny nz amplitude\n";

    int row = 0;
    for (int h = 0; h < std::ssize(vertno) && h < std::ssize(src); ++h) {
        const auto& hemi = src[h];
        for (int v = 0; v < vertno[h].size(); ++v, ++row) {
            int vno = vertno[h](v);
            double amp = stcData(row, timeIdx);

            // Get vertex position and normal from the hemisphere
            float px = 0, py = 0, pz = 0;
            float nx = 0, ny = 0, nz = 1;
            if (vno < hemi.rr.rows()) {
                px = hemi.rr(vno, 0);
                py = hemi.rr(vno, 1);
                pz = hemi.rr(vno, 2);
                if (vno < hemi.nn.rows()) {
                    nx = hemi.nn(vno, 0);
                    ny = hemi.nn(vno, 1);
                    nz = hemi.nn(vno, 2);
                }
            }

            out << vno << "\t"
                << px << "\t" << py << "\t" << pz << "\t"
                << nx << "\t" << ny << "\t" << nz << "\t"
                << amp << "\n";
        }
    }

    file.close();
    Log::info("  Dipole snapshot: {} sources at {:.1f} ms -> {}", row, time_ms, fileName.toStdString());
    return true;
}

/**
 * @brief Write predicted (forward-modeled) sensor data.
 *
 * Computes predicted sensor data = G * J (forward matrix * source estimate)
 * and writes it as a text file.
 *
 * @param[in] fileName    Output file path.
 * @param[in] forward     Forward solution (for the gain matrix G).
 * @param[in] stcData     Source estimate data (nSources x nTimes).
 * @param[in] timesVec    Time vector in seconds.
 * @return true on success.
 */
static bool writePredictedData(const QString& fileName,
                               const MNEForwardSolution& forward,
                               const MatrixXd& stcData,
                               const VectorXd& timesVec)
{
    if (forward.sol.data.rows() == 0) {
        Log::warning("  WARNING: Forward solution has no gain matrix.");
        return false;
    }

    // Predicted data = G * J  (nChannels x nSources) * (nSources x nTimes)
    MatrixXd predicted = forward.sol.data * stcData;

    Log::info("  Predicted data: {} channels, {} time points", static_cast<int>(predicted.rows()), static_cast<int>(predicted.cols()));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::warning("  WARNING: Could not open predicted data output file {}", fileName.toStdString());
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(6);
    out << "# Predicted sensor data (forward * sources)\n";
    out << "# " << predicted.rows() << " channels x " << predicted.cols() << " time points\n";
    out << "# Time(ms):";
    for (int t = 0; t < static_cast<int>(predicted.cols()); ++t)
        out << "\t" << timesVec(t) * 1000.0;
    out << "\n";
    for (int c = 0; c < static_cast<int>(predicted.rows()); ++c) {
        for (int t = 0; t < static_cast<int>(predicted.cols()); ++t) {
            if (t > 0)
                out << "\t";
            out << predicted(c, t);
        }
        out << "\n";
    }
    file.close();

    Log::info("  Predicted data written to {}", fileName.toStdString());
    return true;
}

//=============================================================================================================
// HELPER: compose output STC file name
//=============================================================================================================

[[maybe_unused]] static QString composeStcName(const QString& measFile, const QString& method,
                                               int setNo, const QString& hemi)
{
    QFileInfo fi(measFile);
    QString base = fi.completeBaseName();
    QString dir = fi.path();

    // Remove -ave or -evoked suffix if present
    if (base.endsWith("-ave") || base.endsWith("-evoked")) {
        base = base.left(base.lastIndexOf('-'));
    }

    QString suffix;
    if (method == "MNE") {
        suffix = "-mne";
    } else if (method == "dSPM") {
        suffix = "-dspm";
    } else if (method == "sLORETA") {
        suffix = "-sloreta";
    } else {
        suffix = "-" + method.toLower();
    }

    if (setNo > 0) {
        suffix += QString("-set%1").arg(setNo + 1);
    }

    return dir + "/" + base + suffix + "-" + hemi + ".stc";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_compute_mne application.
 * This is a port of the original MNE-C mne_compute_mne by Matti Hamalainen.
 *
 * It reads evoked data and an inverse operator, then computes minimum norm
 * (MNE/dSPM/sLORETA) source estimates. Results are written as STC files.
 *
 * @param[in] argc  (argument count)
 * @param[in] argv  (argument vector)
 * @return exit code (0 on success).
 */
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_compute_mne");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Compute MNE/dSPM/sLORETA source estimates from evoked data.\n"
        "Port of the original MNE-C mne_compute_mne by Matti Hamalainen.\n\n"
        "Reads evoked data and an inverse operator, applies the minimum\n"
        "norm estimate, and writes STC output files.\n\n"
        "Default method is MNE (bare minimum norm estimate).\n"
        "Use --spm for dSPM or --sLORETA for sLORETA noise normalization.");
    parser.addHelpOption();
    parser.addVersionOption();

    // --- Core options ---

    // --inv: Inverse operator file
    QCommandLineOption invOpt(QStringList() << "inv",
                              "The inverse operator file.", "file");
    parser.addOption(invOpt);

    // --meas: Input measurement (evoked data) file
    QCommandLineOption measOpt(QStringList() << "meas",
                               "The evoked data (measurement) file.", "file");
    parser.addOption(measOpt);

    // --fwd: Forward solution file (use forward as synthetic data instead of --meas)
    QCommandLineOption fwdOpt(QStringList() << "fwd",
                              "Forward solution file. Uses forward solution columns as synthetic data\n"
                              "instead of evoked measurements.",
                              "file");
    parser.addOption(fwdOpt);

    // --fwdamp: Source amplitude for forward-as-data mode (nAm, default 50)
    QCommandLineOption fwdampOpt(QStringList() << "fwdamp",
                                 "Source amplitude in nAm for --fwd mode (default: 50).", "nAm", "50");
    parser.addOption(fwdampOpt);

    // --set: Data set number (1-based, default 1)
    QCommandLineOption setOpt(QStringList() << "set",
                              "Data set number to use (1-based, default: 1).", "number", "1");
    parser.addOption(setOpt);

    // --snr: SNR value
    QCommandLineOption snrOpt(QStringList() << "snr",
                              "Assumed SNR value (default: 3.0).", "value", "3.0");
    parser.addOption(snrOpt);

    // --nave: Override number of averages
    QCommandLineOption naveOpt(QStringList() << "nave",
                               "Override number of averages (default: from data).", "number");
    parser.addOption(naveOpt);

    // --- Method options ---

    // --method: Estimation method
    QCommandLineOption methodOpt(QStringList() << "method",
                                 "Estimation method: MNE, dSPM, or sLORETA (default: MNE).",
                                 "method", "MNE");
    parser.addOption(methodOpt);

    // --spm (flag for dSPM, matching SVN MNE-C)
    QCommandLineOption spmOpt(QStringList() << "spm",
                              "Compute dSPM (statistical parametric map).");
    parser.addOption(spmOpt);

    // --sLORETA (flag for sLORETA, matching SVN MNE-C)
    QCommandLineOption sloretaOpt(QStringList() << "sLORETA",
                                  "Use sLORETA noise normalization.");
    parser.addOption(sloretaOpt);

    // --- Sign and orientation options ---

    // --abs: Output absolute value of current estimates
    QCommandLineOption absOpt(QStringList() << "abs",
                              "Output absolute value of current estimates.");
    parser.addOption(absOpt);

    // --signed: Preserve current direction sign (into/out of cortex)
    QCommandLineOption signedOpt(QStringList() << "signed",
                                 "Preserve signed current direction (into/out of cortex).");
    parser.addOption(signedOpt);

    // --picknormalcomp: Pick normal component only
    QCommandLineOption pickNormalOpt(QStringList() << "picknormalcomp",
                                     "Pick the source component normal to the cortex.");
    parser.addOption(pickNormalOpt);

    // --- Time window ---

    // --tmin: Start time for analysis (ms)
    QCommandLineOption tminOpt(QStringList() << "tmin",
                               "Start time for analysis in ms.", "time/ms");
    parser.addOption(tminOpt);

    // --tmax: End time for analysis (ms)
    QCommandLineOption tmaxOpt(QStringList() << "tmax",
                               "End time for analysis in ms.", "time/ms");
    parser.addOption(tmaxOpt);

    // --- Baseline correction ---

    // --bmin: Baseline start time (ms)
    QCommandLineOption bminOpt(QStringList() << "bmin",
                               "Baseline start time in ms.", "time/ms");
    parser.addOption(bminOpt);

    // --bmax: Baseline end time (ms)
    QCommandLineOption bmaxOpt(QStringList() << "bmax",
                               "Baseline end time in ms.", "time/ms");
    parser.addOption(bmaxOpt);

    // --baselines: Per-channel baselines from file (overrides --bmin/--bmax)
    QCommandLineOption baselinesOpt(QStringList() << "baselines",
                                    "Per-channel baselines file (one line per channel: name value).", "file");
    parser.addOption(baselinesOpt);

    // --- Output options ---

    // --out / --stcout: Output STC base name (optional override)
    QCommandLineOption outOpt(QStringList() << "out" << "stcout",
                              "Base name for STC output files (default: derived from --meas).", "basename");
    parser.addOption(outOpt);

    // --- Collapse options (collapse time axis to single frame) ---

    // --collapse: Collapse with maximum absolute value
    QCommandLineOption collapseOpt(QStringList() << "collapse",
                                   "Collapse time axis: keep value with max absolute value (per source).");
    parser.addOption(collapseOpt);

    // --collapse1: Collapse with L1 norm (mean absolute value)
    QCommandLineOption collapse1Opt(QStringList() << "collapse1",
                                    "Collapse time axis with L1 norm (mean absolute value per source).");
    parser.addOption(collapse1Opt);

    // --collapse2: Collapse with L2 norm (RMS per source)
    QCommandLineOption collapse2Opt(QStringList() << "collapse2",
                                    "Collapse time axis with L2 norm (RMS per source).");
    parser.addOption(collapse2Opt);

    // --- Scaling options ---

    // --scaleto: Scale output so max = value (default 50.0 when not in SI mode)
    QCommandLineOption scaletoOpt(QStringList() << "scaleto",
                                  "Scale output so global maximum = value (default: 50.0).", "value", "50.0");
    parser.addOption(scaletoOpt);

    // --scaleby: Multiply all output by a factor
    QCommandLineOption scalebyOpt(QStringList() << "scaleby",
                                  "Multiply all output values by this factor.", "factor");
    parser.addOption(scalebyOpt);

    // --SIcurrents: Output raw SI-unit currents (no scaling)
    QCommandLineOption siCurrentsOpt(QStringList() << "SIcurrents",
                                     "Output raw SI-unit currents (no scaling applied).");
    parser.addOption(siCurrentsOpt);

    // --- FsLabel options ---

    // --label: Restrict to label region(s) (can be specified multiple times)
    QCommandLineOption labelOpt(QStringList() << "label",
                                "FsLabel file to restrict the output (can be repeated).", "file");
    parser.addOption(labelOpt);

    // --labeltag: Write label-based ASCII output (appends .label to output)
    QCommandLineOption labeltagOpt(QStringList() << "labeltag",
                                   "FsLabel tag for ASCII output file. Written as <out>-<tag>-<hemi>.label.", "tag");
    parser.addOption(labeltagOpt);

    // --labelcoords: Include vertex coordinates in label ASCII output
    QCommandLineOption labelcoordsOpt(QStringList() << "labelcoords",
                                      "Include source coordinates (x,y,z) in label ASCII output.");
    parser.addOption(labelcoordsOpt);

    // --labeltimebytime: Time-by-time layout for label ASCII output
    QCommandLineOption labeltimeOpt(QStringList() << "labeltimebytime",
                                    "Use time-by-time layout for label ASCII output.");
    parser.addOption(labeltimeOpt);

    // --- InvDipole output options ---

    // --dip: Output dipole snapshot file
    QCommandLineOption dipOpt(QStringList() << "dip",
                              "Write a dipole snapshot file at the time specified by --diptime.", "file");
    parser.addOption(dipOpt);

    // --diptime: Time for dipole snapshot (ms)
    QCommandLineOption diptimeOpt(QStringList() << "diptime",
                                  "Time in ms for the dipole snapshot (requires --dip).", "time/ms");
    parser.addOption(diptimeOpt);

    // --- Predicted data output ---

    // --pred: Write predicted sensor data
    QCommandLineOption predOpt(QStringList() << "pred",
                               "Write predicted sensor data (forward * sources) to file.\n"
                               "Requires the forward solution (read from inverse operator).",
                               "file");
    parser.addOption(predOpt);

    // --predfwd: Forward solution file for --pred (if different from inverse)
    QCommandLineOption predfwdOpt(QStringList() << "predfwd",
                                  "Forward solution file for --pred output (default: from inverse operator).", "file");
    parser.addOption(predfwdOpt);

    // --- Time-point extraction ---

    // --pick: Extract source estimate at specific time(s) in ms
    QCommandLineOption pickOpt(QStringList() << "pick",
                               "Extract source estimate at this time in ms (can be repeated).", "time/ms");
    parser.addOption(pickOpt);

    // --- Matching options ---

    // --nomatch: Do not check measurement ID between inverse operator and evoked
    QCommandLineOption nomatchOpt(QStringList() << "nomatch",
                                  "Do not check that inverse operator and data have matching measurement IDs.");
    parser.addOption(nomatchOpt);

    parser.process(app);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.isSet(invOpt)) {
        Log::critical("Error: --inv option is required.");
        parser.showHelp(1);
    }
    bool useFwdAsData = parser.isSet(fwdOpt);
    if (!parser.isSet(measOpt) && !useFwdAsData) {
        Log::critical("Error: --meas option is required (unless --fwd is used).");
        parser.showHelp(1);
    }

    QString invName = parser.value(invOpt);
    QString measName = parser.isSet(measOpt) ? parser.value(measOpt) : QString();
    int setNo = parser.value(setOpt).toInt() - 1; // Convert to 0-based
    double snr = parser.value(snrOpt).toDouble();
    double lambda2 = 1.0 / (snr * snr);

    QString method = parser.value(methodOpt);
    if (parser.isSet(sloretaOpt)) {
        method = "sLORETA";
    }
    if (parser.isSet(spmOpt)) {
        method = "dSPM";
    }

    // Validate method
    if (method != "MNE" && method != "dSPM" && method != "sLORETA") {
        Log::critical("Error: Unknown method {} (must be MNE, dSPM, or sLORETA).", method.toStdString());
        return 1;
    }

    bool pickNormal = parser.isSet(pickNormalOpt);
    bool doAbs = parser.isSet(absOpt);
    bool doSigned = parser.isSet(signedOpt);
    bool nomatch = parser.isSet(nomatchOpt);
    bool siCurrents = parser.isSet(siCurrentsOpt);

    // Parse collapse mode
    int collapseMode = 0;
    if (parser.isSet(collapseOpt))
        collapseMode = COLLAPSE_MAX;
    if (parser.isSet(collapse1Opt))
        collapseMode = COLLAPSE_L1;
    if (parser.isSet(collapse2Opt))
        collapseMode = COLLAPSE_L2;

    // Parse scaling
    double scaleTo = parser.value(scaletoOpt).toDouble();
    double scaleBy = 0.0;
    if (parser.isSet(scalebyOpt)) {
        scaleBy = parser.value(scalebyOpt).toDouble();
        scaleTo = 0.0; // scaleby overrides scaleto
    }

    // FsLabel output options
    bool doLabelTag = parser.isSet(labeltagOpt);
    bool doLabelCoords = parser.isSet(labelcoordsOpt);
    bool doLabelTimeByTime = parser.isSet(labeltimeOpt);
    QString labelTag = doLabelTag ? parser.value(labeltagOpt) : QString();

    //=========================================================================================================
    // Read inverse operator
    //=========================================================================================================

    Log::info("{}", "");
    Log::info("========================================");
    Log::info("Reading inverse operator from {}...", invName.toStdString());

    auto invOpRead = MNEInverseOperator::read(invName.toStdU16String());
    if (!invOpRead) {
        Log::critical("{}", invOpRead.error().what());
        return 1;
    }
    MNEInverseOperator& invOp = *invOpRead;

    Log::info("  Inverse operator: {} sources, {} channels", invOp.nsource, invOp.nchan);
    Log::info("  Source orientation: {}", invOp.isFixedOrient() ? "fixed" : "free");

    //=========================================================================================================
    // Read evoked data (or use forward solution as synthetic data)
    //=========================================================================================================

    MatrixXd data;
    float tmin = 0.0f;
    float tstep = 1.0f;
    int nave = 1;
    int nTimes = 0;
    VectorXd timesVec; // Time vector for label output

    if (useFwdAsData) {
        //---------------------------------------------------------------------
        // Forward-as-data mode: use forward matrix columns as synthetic data
        //---------------------------------------------------------------------
        QString fwdName = parser.value(fwdOpt);
        double fwdAmp = parser.value(fwdampOpt).toDouble() * 1e-9; // nAm -> Am

        Log::info("\nUsing forward solution as synthetic data from {}...", fwdName.toStdString());
        Log::info("  Source amplitude: {:.1f} nAm", fwdAmp * 1e9);

        auto fwdRead = MNEForwardSolution::read(fwdName.toStdU16String());
        if (!fwdRead) {
            Log::critical("{}", fwdRead.error().what());
            return 1;
        }
        MNEForwardSolution& fwd = *fwdRead;

        MatrixXd pickedData(invOp.nchan, fwd.sol.data.cols());
        for (int channel = 0; channel < static_cast<int>(invOp.noise_cov.names.size()); ++channel) {
            const int fwdChannel = mne::channel_index(fwd.sol.row_names, invOp.noise_cov.names[channel]);
            if (fwdChannel < 0) {
                Log::critical("Error: Forward solution is missing inverse-operator channel {}.", invOp.noise_cov.names[channel]);
                return 1;
            }
            pickedData.row(channel) = fwd.sol.data.row(fwdChannel) * fwdAmp;
        }

        tstep = 0.001f; // 1 ms per source
        int firstSource = 0;
        int lastSource = pickedData.cols() - 1;
        if (parser.isSet(tminOpt)) {
            firstSource = qBound(0, parser.value(tminOpt).toInt(), lastSource);
        }
        if (parser.isSet(tmaxOpt)) {
            lastSource = qBound(firstSource, parser.value(tmaxOpt).toInt(), lastSource);
        }
        nTimes = lastSource - firstSource + 1;
        data = pickedData.middleCols(firstSource, nTimes);
        tmin = firstSource * tstep;
        nave = 1;

        Log::info("  Forward synthetic data: {} channels, {} selected sources (time points)", static_cast<int>(data.rows()), nTimes);

        // Time vector for label output
        timesVec.resize(nTimes);
        for (int t = 0; t < nTimes; ++t)
            timesVec(t) = tmin + t * tstep;

    } else {
        //---------------------------------------------------------------------
        // Normal evoked data mode
        //---------------------------------------------------------------------
        Log::info("\nReading evoked data from {} (set {})...", measName.toStdString(), setNo + 1);

        FiffEvoked::Baseline baseline;
        if (parser.isSet(bminOpt) || parser.isSet(bmaxOpt)) {
            float bmin = parser.isSet(bminOpt) ? parser.value(bminOpt).toFloat() / 1000.0f : -1.0f;
            float bmax = parser.isSet(bmaxOpt) ? parser.value(bmaxOpt).toFloat() / 1000.0f : -1.0f;
            baseline = std::pair(bmin, bmax);
            Log::info("  Baseline: {:.1f} - {:.1f} ms", bmin * 1000.0f, bmax * 1000.0f);
        }
        auto read = FiffEvoked::read(measName.toStdU16String(), setNo, baseline);
        if (!read) {
            Log::critical("Error: {}", read.error().what());
            return 1;
        }
        FiffEvoked& evoked = *read;

        Log::info("  Evoked data: {} channels, {} time points", static_cast<int>(evoked.data.rows()), static_cast<int>(evoked.data.cols()));
        Log::info("  Comment: {}", evoked.comment);
        Log::info("  Nave: {}", evoked.nave);
        Log::info("  Time range: {:.1f} - {:.1f} ms", evoked.times(0) * 1000.0f, evoked.times(evoked.times.size() - 1) * 1000.0f);

        // Measurement ID matching check (SVN MNE-C --nomatch feature)
        // Compare measurement IDs between inverse operator info and evoked data
        if (!nomatch) {
            if (invOp.info.meas_id.version > 0 && evoked.info.meas_id.version > 0) {
                if (invOp.info.meas_id.version != evoked.info.meas_id.version ||
                    invOp.info.meas_id.machid[0] != evoked.info.meas_id.machid[0] ||
                    invOp.info.meas_id.machid[1] != evoked.info.meas_id.machid[1]) {
                    Log::info("  WARNING: Measurement IDs differ between inverse operator and data.");
                    Log::info("           Use --nomatch to suppress this check.");
                }
            }
        } else {
            Log::info("  Measurement ID matching skipped (--nomatch).");
        }

        // Apply per-channel baselines from file if specified
        if (parser.isSet(baselinesOpt)) {
            StringList chNames;
            for (int c = 0; c < static_cast<int>(evoked.info.ch_names.size()); ++c)
                chNames.push_back(evoked.info.ch_names[c]);

            VectorXd baselines;
            if (readBaselines(parser.value(baselinesOpt), chNames, baselines)) {
                for (int c = 0; c < evoked.data.rows(); ++c) {
                    evoked.data.row(c).array() -= baselines(c);
                }
                Log::info("  Applied per-channel baselines from file.");
            }
        }

        // Override nave if specified
        nave = evoked.nave;
        if (parser.isSet(naveOpt)) {
            nave = parser.value(naveOpt).toInt();
            Log::info("  Overriding nave to {}", nave);
        }

        // Apply time window if specified
        int tminIdx = 0;
        int tmaxIdx = evoked.data.cols() - 1;
        if (parser.isSet(tminOpt)) {
            float tmin_s = parser.value(tminOpt).toFloat() / 1000.0f;
            for (int i = 0; i < evoked.times.size(); ++i) {
                if (evoked.times(i) >= tmin_s) {
                    tminIdx = i;
                    break;
                }
            }
            Log::info("  Start time restricted to {:.1f} ms (sample {})", evoked.times(tminIdx) * 1000.0f, tminIdx);
        }
        if (parser.isSet(tmaxOpt)) {
            float tmax_s = parser.value(tmaxOpt).toFloat() / 1000.0f;
            for (int i = evoked.times.size() - 1; i >= 0; --i) {
                if (evoked.times(i) <= tmax_s) {
                    tmaxIdx = i;
                    break;
                }
            }
            Log::info("  End time restricted to {:.1f} ms (sample {})", evoked.times(tmaxIdx) * 1000.0f, tmaxIdx);
        }

        FiffEvoked pickedEvoked = evoked.pick_channels(invOp.noise_cov.names);
        if (pickedEvoked.info.nchan != invOp.nchan) {
            Log::critical("Error: Found {} of {} inverse-operator channels in the evoked data.", pickedEvoked.info.nchan, invOp.nchan);
            return 1;
        }
        Log::info("  Picked {} inverse-operator channels", pickedEvoked.info.nchan);

        // Extract the time window
        nTimes = tmaxIdx - tminIdx + 1;
        data = pickedEvoked.data.block(0, tminIdx, pickedEvoked.data.rows(), nTimes);
        tmin = evoked.times(tminIdx);
        tstep = (evoked.times.size() > 1) ? (evoked.times(1) - evoked.times(0)) : 1.0f;

        // Build time vector for label output
        timesVec.resize(nTimes);
        for (int t = 0; t < nTimes; ++t)
            timesVec(t) = tmin + t * tstep;
    }

    //=========================================================================================================
    // Compute inverse solution
    //=========================================================================================================

    Log::info("\nComputing {} inverse solution (SNR={:.1f}, lambda2={:.4e}, nave={})...", method.toStdString(), snr, lambda2, nave);

    InvMinimumNorm minimumNorm(invOp, lambda2, method.toStdString());
    minimumNorm.doInverseSetup(nave, pickNormal);

    InvSourceEstimate stc = minimumNorm.calculateInverse(data, tmin, tstep, pickNormal);

    if (stc.empty()) {
        Log::critical("Error: Inverse computation returned empty result.");
        return 1;
    }

    Log::info("  Source estimate: {} sources, {} time points", static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));
    Log::info("  Time range: {:.1f} - {:.1f} ms", stc.tmin * 1000.0f, (stc.tmin + (stc.data.cols() - 1) * stc.tstep) * 1000.0f);

    //=========================================================================================================
    // Post-processing: abs/signed, collapse, scaling
    //=========================================================================================================

    // Apply abs/signed transformations
    if (doAbs) {
        Log::info("  Applying absolute value transform.");
        stc.data = stc.data.cwiseAbs();
    } else if (doSigned) {
        Log::info("  Preserving signed current direction.");
        // Signed: keep data as-is (already signed from fixed orientation)
    }

    // Collapse time axis to single frame
    if (collapseMode > 0) {
        const char* modeNames[] = {"", "max-abs", "L1 (mean-abs)", "L2 (RMS)"};
        Log::info("  Collapsing time axis using {} mode.", modeNames[collapseMode]);
        collapseData(stc.data, collapseMode);
    }

    // Scale output
    scaleData(stc.data, scaleTo, scaleBy, siCurrents);

    //=========================================================================================================
    // Write STC output
    //=========================================================================================================

    // Determine output base name
    QString stcBase;
    if (parser.isSet(outOpt)) {
        stcBase = parser.value(outOpt);
    } else if (useFwdAsData) {
        // Derive from forward file
        QFileInfo fi(parser.value(fwdOpt));
        QString base = fi.completeBaseName();
        if (base.endsWith("-fwd")) {
            base = base.left(base.lastIndexOf('-'));
        }
        stcBase = fi.path() + "/" + base + "-fwd-mne";
    } else {
        // Derive from meas file
        QFileInfo fi(measName);
        QString base = fi.completeBaseName();
        if (base.endsWith("-ave") || base.endsWith("-evoked")) {
            base = base.left(base.lastIndexOf('-'));
        }

        QString methodSuffix;
        if (method == "MNE")
            methodSuffix = "-mne";
        else if (method == "dSPM")
            methodSuffix = "-dspm";
        else if (method == "sLORETA")
            methodSuffix = "-sloreta";
        else
            methodSuffix = "-" + method.toLower();

        if (setNo > 0) {
            methodSuffix += QString("-set%1").arg(setNo + 1);
        }

        stcBase = fi.path() + "/" + base + methodSuffix;
    }

    // Split source estimate into hemispheres and write
    std::vector<VectorXi> vertno = invOp.src.get_vertno();

    if (vertno.size() >= 2) {
        int nLh = vertno[0].size();
        int nRh = vertno[1].size();

        // Left hemisphere
        if (nLh > 0) {
            QString lhFile = stcBase + "-lh.stc";
            Log::info("\nWriting left hemisphere STC to {}...", lhFile.toStdString());

            MatrixXd lhData = stc.data.topRows(nLh);
            InvSourceEstimate lhStc(lhData, vertno[0], stc.tmin, stc.tstep);

            if (!lhStc.write(lhFile.toStdU16String())) {
                Log::warning("Warning: Failed to write {}", lhFile.toStdString());
            } else {
                Log::info("  {} vertices, {} time points", nLh, static_cast<int>(lhData.cols()));
            }
        }

        // Right hemisphere
        if (nRh > 0) {
            QString rhFile = stcBase + "-rh.stc";
            Log::info("Writing right hemisphere STC to {}...", rhFile.toStdString());

            MatrixXd rhData = stc.data.bottomRows(nRh);
            InvSourceEstimate rhStc(rhData, vertno[1], stc.tmin, stc.tstep);

            if (!rhStc.write(rhFile.toStdU16String())) {
                Log::warning("Warning: Failed to write {}", rhFile.toStdString());
            } else {
                Log::info("  {} vertices, {} time points", nRh, static_cast<int>(rhData.cols()));
            }
        }

        //=====================================================================================
        // FsLabel-based ASCII output
        //=====================================================================================

        QStringList labelFiles = parser.values(labelOpt);
        if (doLabelTag && !labelFiles.isEmpty()) {
            Log::info("\nWriting label-based ASCII output...");

            for (const QString& labelFile : labelFiles) {
                FsLabel label;
                if (!FsLabel::read(labelFile.toStdU16String(), label)) {
                    Log::warning("  WARNING: Could not read label {}", labelFile.toStdString());
                    continue;
                }
                Log::info("  FsLabel: {} ({} vertices, hemi={})", label.name, static_cast<int>(label.vertices.size()), label.hemi);

                // Determine hemisphere and write
                // hemi: 0 = lh, 1 = rh (FsLabel convention)
                VectorXi hemiVertno;
                int hemiOffset = 0;
                QString hemiName;

                if (label.hemi == 0) {
                    hemiVertno = vertno[0];
                    hemiOffset = 0;
                    hemiName = "lh";
                } else if (label.hemi == 1) {
                    hemiVertno = vertno[1];
                    hemiOffset = nLh;
                    hemiName = "rh";
                } else {
                    // Try to auto-detect from label name
                    if (label.name.contains("lh")) {
                        hemiVertno = vertno[0];
                        hemiOffset = 0;
                        hemiName = "lh";
                    } else {
                        hemiVertno = vertno[1];
                        hemiOffset = nLh;
                        hemiName = "rh";
                    }
                }

                QString outName = stcBase + "-" + labelTag + "-" + hemiName + ".label";
                writeLabelOutput(label, stc.data, hemiVertno, hemiOffset,
                                 timesVec, outName, doLabelCoords, doLabelTimeByTime);
            }
        }
    } else {
        // Single source space (e.g., volume) — write as single STC
        QString outFile = stcBase + ".stc";
        Log::info("\nWriting STC to {}...", outFile.toStdString());

        if (auto written = stc.write(outFile.toStdU16String()); !written) {
            Log::critical("{}", written.error().what());
            return 1;
        }
        Log::info("  {} vertices, {} time points", static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));
    }

    //=========================================================================================================
    // Dipole snapshot output (--dip / --diptime)
    //=========================================================================================================

    if (parser.isSet(dipOpt)) {
        QString dipFile = parser.value(dipOpt);
        double dipTime_ms = parser.isSet(diptimeOpt) ? parser.value(diptimeOpt).toDouble() : 0.0;
        double dipTime_s = dipTime_ms / 1000.0;

        // Find nearest time index
        int dipIdx = 0;
        double minDiff = std::abs(timesVec(0) - dipTime_s);
        for (int t = 1; t < timesVec.size(); ++t) {
            double diff = std::abs(timesVec(t) - dipTime_s);
            if (diff < minDiff) {
                minDiff = diff;
                dipIdx = t;
            }
        }
        Log::info("\nWriting dipole snapshot at {:.1f} ms (nearest: {:.1f} ms, idx {})...", dipTime_ms, timesVec(dipIdx) * 1000.0, dipIdx);

        writeDipFile(dipFile, stc.data, invOp.src, vertno, dipIdx, timesVec(dipIdx) * 1000.0);
    }

    //=========================================================================================================
    // Time-point extraction (--pick)
    //=========================================================================================================

    QStringList pickValues = parser.values(pickOpt);
    if (!pickValues.isEmpty()) {
        Log::info("\nExtracting source estimates at {} time point(s)...", static_cast<int>(pickValues.size()));

        for (const QString& pickStr : pickValues) {
            double pickTime_ms = pickStr.toDouble();
            double pickTime_s = pickTime_ms / 1000.0;

            // Find nearest time index
            int pickIdx = 0;
            double minDiff = std::abs(timesVec(0) - pickTime_s);
            for (int t = 1; t < timesVec.size(); ++t) {
                double diff = std::abs(timesVec(t) - pickTime_s);
                if (diff < minDiff) {
                    minDiff = diff;
                    pickIdx = t;
                }
            }

            Log::info("  Pick at {:.1f} ms (nearest: {:.1f} ms, idx {})", pickTime_ms, timesVec(pickIdx) * 1000.0, pickIdx);

            // Write per-hemisphere w-like text files
            if (vertno.size() >= 2) {
                int nLhV = vertno[0].size();

                for (int h = 0; h < 2; ++h) {
                    QString hemiName = (h == 0) ? "lh" : "rh";
                    int offset = (h == 0) ? 0 : nLhV;
                    int nVert = vertno[h].size();

                    QString pickFile = stcBase + QString("-pick%1-%2.w").arg(QString::number(pickTime_ms, 'f', 0)).arg(hemiName);
                    QFile pf(pickFile);
                    if (!pf.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        Log::warning("  WARNING: Could not write {}", pickFile.toStdString());
                        continue;
                    }
                    QTextStream out(&pf);
                    out.setRealNumberNotation(QTextStream::ScientificNotation);
                    out.setRealNumberPrecision(6);
                    out << "# Source estimate at " << timesVec(pickIdx) * 1000.0 << " ms\n";
                    out << "# vertex value\n";
                    for (int v = 0; v < nVert; ++v) {
                        out << vertno[h](v) << "\t" << stc.data(offset + v, pickIdx) << "\n";
                    }
                    pf.close();
                    Log::info("    {}: {} vertices -> {}", hemiName.toStdString(), nVert, pickFile.toStdString());
                }
            }
        }
    }

    //=========================================================================================================
    // Predicted data output (--pred)
    //=========================================================================================================

    if (parser.isSet(predOpt)) {
        QString predFile = parser.value(predOpt);
        Log::info("\nComputing predicted sensor data...");

        // Read forward solution for prediction
        MNEForwardSolution predFwd;
        if (parser.isSet(predfwdOpt)) {
            predFwd = MNEForwardSolution(parser.value(predfwdOpt).toStdU16String(), {.surf_ori = true});
        } else if (useFwdAsData) {
            predFwd = MNEForwardSolution(parser.value(fwdOpt).toStdU16String(), {.surf_ori = true});
        } else {
            Log::info("  NOTE: --pred requires a forward solution.");
            Log::info("         Use --predfwd to specify one, or use --fwd mode.");
            Log::info("         Skipping predicted data output.");
            goto done;
        }

        if (predFwd.sol.data.rows() > 0) {
            writePredictedData(predFile, predFwd, stc.data, timesVec);
        } else {
            Log::info("  WARNING: Forward solution has no gain matrix.");
        }
    }

done:
    Log::info("\nDone.");

    return 0;
}
