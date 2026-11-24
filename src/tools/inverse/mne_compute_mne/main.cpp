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

#include <mne/fiff/fiff.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_stream.h>

#include <mne/mne/mne.h>
#include <mne/mne/mne_inverse_operator.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_source_spaces.h>

#include <mne/inv/minimum_norm/inv_minimum_norm.h>

#include <mne/fs/fs_label.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <vector>

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
 * @brief Format a number like QTextStream in scientific notation with precision 6.
 */
static std::string sci(double value)
{
    return std::format("{:.6e}", value);
}

/**
 * @brief Parse a number like QString::toDouble(): 0 if the text is not a number.
 */
static double toDouble(const std::string& text)
{
    return to_double(text).value_or(0.0);
}

/**
 * @brief Parse a number like QString::toFloat(): 0 if the text is not a number.
 */
static float toFloat(const std::string& text)
{
    return static_cast<float>(toDouble(text));
}

/**
 * @brief Parse a number like QString::toInt(): 0 if the text is not an integer.
 */
static int toInt(const std::string& text)
{
    return to_int(text).value_or(0);
}

/**
 * @brief The file name without its last extension, like QFileInfo::completeBaseName().
 */
static std::string completeBaseName(const std::string& file)
{
    const std::string name = utf8_from_path(path_from_utf8(file).filename());
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

/**
 * @brief The directory of a file, "." if it has none, like QFileInfo::path().
 */
static std::string dirPath(const std::string& file)
{
    const std::filesystem::path parent = path_from_utf8(file).parent_path();
    return parent.empty() ? std::string(".") : utf8_from_path(parent);
}

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
static bool readBaselines(const std::string& fileName,
                          const StringList& ch_names,
                          VectorXd& baselines)
{
    std::ifstream file(path_from_utf8(fileName));
    if (!file) {
        Log::warning("  WARNING: Could not open baselines file {}", fileName);
        return false;
    }

    baselines = VectorXd::Zero(static_cast<Eigen::Index>(ch_names.size()));
    int nFound = 0;
    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string_view line = trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() >= 2) {
            const int idx = mne::channel_index(ch_names, std::string(parts[0]));
            if (idx >= 0) {
                baselines(idx) = to_double(parts[1]).value_or(0.0);
                nFound++;
            }
        }
    }
    file.close();
    Log::info("  Read {} baseline values from {}", nFound, fileName);
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
                             const std::string& outputFile,
                             bool labelCoords,
                             bool timeBytime)
{
    // Find label vertices in the source space
    std::vector<int> srcIdx;       // indices into stcData rows
    std::vector<int> labelVertIdx; // indices into label.vertices

    for (int lv = 0; lv < label.vertices.size(); ++lv) {
        int vno = label.vertices(lv);
        for (int sv = 0; sv < vertno.size(); ++sv) {
            if (vertno(sv) == vno) {
                srcIdx.push_back(hemiOffset + sv);
                labelVertIdx.push_back(lv);
                break;
            }
        }
    }

    if (srcIdx.empty()) {
        Log::info("  WARNING: No vertices found in label {} for this hemisphere.", label.name);
        return false;
    }

    std::ofstream out(path_from_utf8(outputFile));
    if (!out) {
        Log::warning("  WARNING: Could not open label output file {}", outputFile);
        return false;
    }

    const int nTimes = static_cast<int>(stcData.cols());
    const int nSrc = static_cast<int>(srcIdx.size());

    if (timeBytime) {
        // Time-by-time layout: rows = time, columns = vertices
        // Header with vertex numbers
        if (labelCoords) {
            out << "# Vertex:";
            for (int i = 0; i < nSrc; ++i)
                out << "\t" << label.vertices(labelVertIdx[i]);
            out << "\n";
            out << "# X(mm):";
            for (int i = 0; i < nSrc; ++i)
                out << "\t" << sci(label.pos(labelVertIdx[i], 0) * 1000.0);
            out << "\n";
            out << "# Y(mm):";
            for (int i = 0; i < nSrc; ++i)
                out << "\t" << sci(label.pos(labelVertIdx[i], 1) * 1000.0);
            out << "\n";
            out << "# Z(mm):";
            for (int i = 0; i < nSrc; ++i)
                out << "\t" << sci(label.pos(labelVertIdx[i], 2) * 1000.0);
            out << "\n";
        }
        // Data rows: time value1 value2 ...
        for (int t = 0; t < nTimes; ++t) {
            out << sci(times(t) * 1000.0);
            for (int i = 0; i < nSrc; ++i)
                out << "\t" << sci(stcData(srcIdx[i], t));
            out << "\n";
        }
    } else {
        // Point-by-point layout (default): rows = vertices, columns = time
        // First row: times
        out << "# Time(ms):";
        for (int t = 0; t < nTimes; ++t)
            out << "\t" << sci(times(t) * 1000.0);
        out << "\n";

        // Data rows: vertex [x y z] value1 value2 ...
        for (int i = 0; i < nSrc; ++i) {
            out << label.vertices(labelVertIdx[i]);
            if (labelCoords) {
                out << "\t" << sci(label.pos(labelVertIdx[i], 0) * 1000.0)
                    << "\t" << sci(label.pos(labelVertIdx[i], 1) * 1000.0)
                    << "\t" << sci(label.pos(labelVertIdx[i], 2) * 1000.0);
            }
            for (int t = 0; t < nTimes; ++t)
                out << "\t" << sci(stcData(srcIdx[i], t));
            out << "\n";
        }
    }

    out.close();
    Log::info("  FsLabel output: {} vertices, {} times -> {}", nSrc, nTimes, outputFile);
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
static bool writeDipFile(const std::string& fileName,
                         const MatrixXd& stcData,
                         const MNESourceSpaces& src,
                         const std::vector<VectorXi>& vertno,
                         int timeIdx,
                         double time_ms)
{
    std::ofstream out(path_from_utf8(fileName));
    if (!out) {
        Log::warning("  WARNING: Could not open dipole output file {}", fileName);
        return false;
    }

    out << "# Dipole snapshot at " << sci(time_ms) << " ms\n";
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
                << sci(px) << "\t" << sci(py) << "\t" << sci(pz) << "\t"
                << sci(nx) << "\t" << sci(ny) << "\t" << sci(nz) << "\t"
                << sci(amp) << "\n";
        }
    }

    out.close();
    Log::info("  Dipole snapshot: {} sources at {:.1f} ms -> {}", row, time_ms, fileName);
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
static bool writePredictedData(const std::string& fileName,
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

    std::ofstream out(path_from_utf8(fileName));
    if (!out) {
        Log::warning("  WARNING: Could not open predicted data output file {}", fileName);
        return false;
    }

    out << "# Predicted sensor data (forward * sources)\n";
    out << "# " << predicted.rows() << " channels x " << predicted.cols() << " time points\n";
    out << "# Time(ms):";
    for (int t = 0; t < static_cast<int>(predicted.cols()); ++t)
        out << "\t" << sci(timesVec(t) * 1000.0);
    out << "\n";
    for (int c = 0; c < static_cast<int>(predicted.rows()); ++c) {
        for (int t = 0; t < static_cast<int>(predicted.cols()); ++t) {
            if (t > 0)
                out << "\t";
            out << sci(predicted(c, t));
        }
        out << "\n";
    }
    out.close();

    Log::info("  Predicted data written to {}", fileName);
    return true;
}

//=============================================================================================================
// HELPER: compose output STC file name
//=============================================================================================================

[[maybe_unused]] static std::string composeStcName(const std::string& measFile, const std::string& method,
                                                   int setNo, const std::string& hemi)
{
    std::string base = completeBaseName(measFile);
    const std::string dir = dirPath(measFile);

    // Remove -ave or -evoked suffix if present
    if (base.ends_with("-ave") || base.ends_with("-evoked")) {
        base = base.substr(0, base.rfind('-'));
    }

    std::string suffix;
    if (method == "MNE") {
        suffix = "-mne";
    } else if (method == "dSPM") {
        suffix = "-dspm";
    } else if (method == "sLORETA") {
        suffix = "-sloreta";
    } else {
        suffix = "-" + to_lower(method);
    }

    if (setNo > 0) {
        suffix += std::format("-set{}", setNo + 1);
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
    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    CommandLineParser parser(
        "Compute MNE/dSPM/sLORETA source estimates from evoked data.\n"
        "Port of the original MNE-C mne_compute_mne by Matti Hamalainen.\n\n"
        "Reads evoked data and an inverse operator, applies the minimum\n"
        "norm estimate, and writes STC output files.\n\n"
        "Default method is MNE (bare minimum norm estimate).\n"
        "Use --spm for dSPM or --sLORETA for sLORETA noise normalization.");
    parser.add_help_option();
    parser.add_version_option();

    // --- Core options ---

    // --inv: Inverse operator file
    parser.add_option({{"inv"}, "The inverse operator file.", "file", {}});

    // --meas: Input measurement (evoked data) file
    parser.add_option({{"meas"}, "The evoked data (measurement) file.", "file", {}});

    // --fwd: Forward solution file (use forward as synthetic data instead of --meas)
    parser.add_option({{"fwd"}, "Forward solution file. Uses forward solution columns as synthetic data\n"
                                "instead of evoked measurements.",
                       "file",
                       {}});

    // --fwdamp: Source amplitude for forward-as-data mode (nAm, default 50)
    parser.add_option({{"fwdamp"}, "Source amplitude in nAm for --fwd mode (default: 50).", "nAm", "50"});

    // --set: Data set number (1-based, default 1)
    parser.add_option({{"set"}, "Data set number to use (1-based, default: 1).", "number", "1"});

    // --snr: SNR value
    parser.add_option({{"snr"}, "Assumed SNR value (default: 3.0).", "value", "3.0"});

    // --nave: Override number of averages
    parser.add_option({{"nave"}, "Override number of averages (default: from data).", "number", {}});

    // --- Method options ---

    // --method: Estimation method
    parser.add_option({{"method"}, "Estimation method: MNE, dSPM, or sLORETA (default: MNE).", "method", "MNE"});

    // --spm (flag for dSPM, matching SVN MNE-C)
    parser.add_option({{"spm"}, "Compute dSPM (statistical parametric map).", {}, {}});

    // --sLORETA (flag for sLORETA, matching SVN MNE-C)
    parser.add_option({{"sLORETA"}, "Use sLORETA noise normalization.", {}, {}});

    // --- Sign and orientation options ---

    // --abs: Output absolute value of current estimates
    parser.add_option({{"abs"}, "Output absolute value of current estimates.", {}, {}});

    // --signed: Preserve current direction sign (into/out of cortex)
    parser.add_option({{"signed"}, "Preserve signed current direction (into/out of cortex).", {}, {}});

    // --picknormalcomp: Pick normal component only
    parser.add_option({{"picknormalcomp"}, "Pick the source component normal to the cortex.", {}, {}});

    // --- Time window ---

    // --tmin: Start time for analysis (ms)
    parser.add_option({{"tmin"}, "Start time for analysis in ms.", "time/ms", {}});

    // --tmax: End time for analysis (ms)
    parser.add_option({{"tmax"}, "End time for analysis in ms.", "time/ms", {}});

    // --- Baseline correction ---

    // --bmin: Baseline start time (ms)
    parser.add_option({{"bmin"}, "Baseline start time in ms.", "time/ms", {}});

    // --bmax: Baseline end time (ms)
    parser.add_option({{"bmax"}, "Baseline end time in ms.", "time/ms", {}});

    // --baselines: Per-channel baselines from file (overrides --bmin/--bmax)
    parser.add_option({{"baselines"}, "Per-channel baselines file (one line per channel: name value).", "file", {}});

    // --- Output options ---

    // --out / --stcout: Output STC base name (optional override)
    parser.add_option({{"out", "stcout"}, "Base name for STC output files (default: derived from --meas).", "basename", {}});

    // --- Collapse options (collapse time axis to single frame) ---

    // --collapse: Collapse with maximum absolute value
    parser.add_option({{"collapse"}, "Collapse time axis: keep value with max absolute value (per source).", {}, {}});

    // --collapse1: Collapse with L1 norm (mean absolute value)
    parser.add_option({{"collapse1"}, "Collapse time axis with L1 norm (mean absolute value per source).", {}, {}});

    // --collapse2: Collapse with L2 norm (RMS per source)
    parser.add_option({{"collapse2"}, "Collapse time axis with L2 norm (RMS per source).", {}, {}});

    // --- Scaling options ---

    // --scaleto: Scale output so max = value (default 50.0 when not in SI mode)
    parser.add_option({{"scaleto"}, "Scale output so global maximum = value (default: 50.0).", "value", "50.0"});

    // --scaleby: Multiply all output by a factor
    parser.add_option({{"scaleby"}, "Multiply all output values by this factor.", "factor", {}});

    // --SIcurrents: Output raw SI-unit currents (no scaling)
    parser.add_option({{"SIcurrents"}, "Output raw SI-unit currents (no scaling applied).", {}, {}});

    // --- FsLabel options ---

    // --label: Restrict to label region(s) (can be specified multiple times)
    parser.add_option({{"label"}, "FsLabel file to restrict the output (can be repeated).", "file", {}});

    // --labeltag: Write label-based ASCII output (appends .label to output)
    parser.add_option({{"labeltag"}, "FsLabel tag for ASCII output file. Written as <out>-<tag>-<hemi>.label.", "tag", {}});

    // --labelcoords: Include vertex coordinates in label ASCII output
    parser.add_option({{"labelcoords"}, "Include source coordinates (x,y,z) in label ASCII output.", {}, {}});

    // --labeltimebytime: Time-by-time layout for label ASCII output
    parser.add_option({{"labeltimebytime"}, "Use time-by-time layout for label ASCII output.", {}, {}});

    // --- InvDipole output options ---

    // --dip: Output dipole snapshot file
    parser.add_option({{"dip"}, "Write a dipole snapshot file at the time specified by --diptime.", "file", {}});

    // --diptime: Time for dipole snapshot (ms)
    parser.add_option({{"diptime"}, "Time in ms for the dipole snapshot (requires --dip).", "time/ms", {}});

    // --- Predicted data output ---

    // --pred: Write predicted sensor data
    parser.add_option({{"pred"}, "Write predicted sensor data (forward * sources) to file.\n"
                                 "Requires the forward solution (read from inverse operator).",
                       "file",
                       {}});

    // --predfwd: Forward solution file for --pred (if different from inverse)
    parser.add_option({{"predfwd"}, "Forward solution file for --pred output (default: from inverse operator).", "file", {}});

    // --- Time-point extraction ---

    // --pick: Extract source estimate at specific time(s) in ms
    parser.add_option({{"pick"}, "Extract source estimate at this time in ms (can be repeated).", "time/ms", {}});

    // --- Matching options ---

    // --nomatch: Do not check measurement ID between inverse operator and evoked
    parser.add_option({{"nomatch"}, "Do not check that inverse operator and data have matching measurement IDs.", {}, {}});

    parser.process(argc, argv, "mne_compute_mne", PROGRAM_VERSION);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.is_set("inv")) {
        Log::critical("Error: --inv option is required.");
        std::print("{}", parser.help_text("mne_compute_mne"));
        return 1;
    }
    bool useFwdAsData = parser.is_set("fwd");
    if (!parser.is_set("meas") && !useFwdAsData) {
        Log::critical("Error: --meas option is required (unless --fwd is used).");
        std::print("{}", parser.help_text("mne_compute_mne"));
        return 1;
    }

    const std::string invName = parser.value("inv");
    const std::string measName = parser.is_set("meas") ? parser.value("meas") : std::string();
    int setNo = toInt(parser.value("set")) - 1; // Convert to 0-based
    double snr = toDouble(parser.value("snr"));
    double lambda2 = 1.0 / (snr * snr);

    std::string method = parser.value("method");
    if (parser.is_set("sLORETA")) {
        method = "sLORETA";
    }
    if (parser.is_set("spm")) {
        method = "dSPM";
    }

    // Validate method
    if (method != "MNE" && method != "dSPM" && method != "sLORETA") {
        Log::critical("Error: Unknown method {} (must be MNE, dSPM, or sLORETA).", method);
        return 1;
    }

    bool pickNormal = parser.is_set("picknormalcomp");
    bool doAbs = parser.is_set("abs");
    bool doSigned = parser.is_set("signed");
    bool nomatch = parser.is_set("nomatch");
    bool siCurrents = parser.is_set("SIcurrents");

    // Parse collapse mode
    int collapseMode = 0;
    if (parser.is_set("collapse"))
        collapseMode = COLLAPSE_MAX;
    if (parser.is_set("collapse1"))
        collapseMode = COLLAPSE_L1;
    if (parser.is_set("collapse2"))
        collapseMode = COLLAPSE_L2;

    // Parse scaling
    double scaleTo = toDouble(parser.value("scaleto"));
    double scaleBy = 0.0;
    if (parser.is_set("scaleby")) {
        scaleBy = toDouble(parser.value("scaleby"));
        scaleTo = 0.0; // scaleby overrides scaleto
    }

    // FsLabel output options
    bool doLabelTag = parser.is_set("labeltag");
    bool doLabelCoords = parser.is_set("labelcoords");
    bool doLabelTimeByTime = parser.is_set("labeltimebytime");
    const std::string labelTag = doLabelTag ? parser.value("labeltag") : std::string();

    //=========================================================================================================
    // Read inverse operator
    //=========================================================================================================

    Log::info("{}", "");
    Log::info("========================================");
    Log::info("Reading inverse operator from {}...", invName);

    auto invOpRead = MNEInverseOperator::read(path_from_utf8(invName));
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
        const std::string fwdName = parser.value("fwd");
        double fwdAmp = toDouble(parser.value("fwdamp")) * 1e-9; // nAm -> Am

        Log::info("\nUsing forward solution as synthetic data from {}...", fwdName);
        Log::info("  Source amplitude: {:.1f} nAm", fwdAmp * 1e9);

        auto fwdRead = MNEForwardSolution::read(path_from_utf8(fwdName));
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
        int lastSource = static_cast<int>(pickedData.cols()) - 1;
        if (parser.is_set("tmin")) {
            firstSource = std::max(0, std::min(toInt(parser.value("tmin")), lastSource));
        }
        if (parser.is_set("tmax")) {
            lastSource = std::max(firstSource, std::min(toInt(parser.value("tmax")), lastSource));
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
        Log::info("\nReading evoked data from {} (set {})...", measName, setNo + 1);

        FiffEvoked::Baseline baseline;
        if (parser.is_set("bmin") || parser.is_set("bmax")) {
            float bmin = parser.is_set("bmin") ? toFloat(parser.value("bmin")) / 1000.0f : -1.0f;
            float bmax = parser.is_set("bmax") ? toFloat(parser.value("bmax")) / 1000.0f : -1.0f;
            baseline = std::pair(bmin, bmax);
            Log::info("  Baseline: {:.1f} - {:.1f} ms", bmin * 1000.0f, bmax * 1000.0f);
        }
        auto read = FiffEvoked::read(path_from_utf8(measName), setNo, baseline);
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
        if (parser.is_set("baselines")) {
            StringList chNames;
            for (int c = 0; c < static_cast<int>(evoked.info.ch_names.size()); ++c)
                chNames.push_back(evoked.info.ch_names[c]);

            VectorXd baselines;
            if (readBaselines(parser.value("baselines"), chNames, baselines)) {
                for (int c = 0; c < evoked.data.rows(); ++c) {
                    evoked.data.row(c).array() -= baselines(c);
                }
                Log::info("  Applied per-channel baselines from file.");
            }
        }

        // Override nave if specified
        nave = evoked.nave;
        if (parser.is_set("nave")) {
            nave = toInt(parser.value("nave"));
            Log::info("  Overriding nave to {}", nave);
        }

        // Apply time window if specified
        int tminIdx = 0;
        int tmaxIdx = static_cast<int>(evoked.data.cols()) - 1;
        if (parser.is_set("tmin")) {
            float tmin_s = toFloat(parser.value("tmin")) / 1000.0f;
            for (int i = 0; i < evoked.times.size(); ++i) {
                if (evoked.times(i) >= tmin_s) {
                    tminIdx = i;
                    break;
                }
            }
            Log::info("  Start time restricted to {:.1f} ms (sample {})", evoked.times(tminIdx) * 1000.0f, tminIdx);
        }
        if (parser.is_set("tmax")) {
            float tmax_s = toFloat(parser.value("tmax")) / 1000.0f;
            for (int i = static_cast<int>(evoked.times.size()) - 1; i >= 0; --i) {
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

    Log::info("\nComputing {} inverse solution (SNR={:.1f}, lambda2={:.4e}, nave={})...", method, snr, lambda2, nave);

    InvMinimumNorm minimumNorm(invOp, lambda2, method);
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
    std::string stcBase;
    if (parser.is_set("out")) {
        stcBase = parser.value("out");
    } else if (useFwdAsData) {
        // Derive from forward file
        const std::string fwdName = parser.value("fwd");
        std::string base = completeBaseName(fwdName);
        if (base.ends_with("-fwd")) {
            base = base.substr(0, base.rfind('-'));
        }
        stcBase = dirPath(fwdName) + "/" + base + "-fwd-mne";
    } else {
        // Derive from meas file
        std::string base = completeBaseName(measName);
        if (base.ends_with("-ave") || base.ends_with("-evoked")) {
            base = base.substr(0, base.rfind('-'));
        }

        std::string methodSuffix;
        if (method == "MNE")
            methodSuffix = "-mne";
        else if (method == "dSPM")
            methodSuffix = "-dspm";
        else if (method == "sLORETA")
            methodSuffix = "-sloreta";
        else
            methodSuffix = "-" + to_lower(method);

        if (setNo > 0) {
            methodSuffix += std::format("-set{}", setNo + 1);
        }

        stcBase = dirPath(measName) + "/" + base + methodSuffix;
    }

    // Split source estimate into hemispheres and write
    std::vector<VectorXi> vertno = invOp.src.get_vertno();

    if (vertno.size() >= 2) {
        const int nLh = static_cast<int>(vertno[0].size());
        const int nRh = static_cast<int>(vertno[1].size());

        // Left hemisphere
        if (nLh > 0) {
            const std::string lhFile = stcBase + "-lh.stc";
            Log::info("\nWriting left hemisphere STC to {}...", lhFile);

            MatrixXd lhData = stc.data.topRows(nLh);
            InvSourceEstimate lhStc(lhData, vertno[0], stc.tmin, stc.tstep);

            if (!lhStc.write(path_from_utf8(lhFile))) {
                Log::warning("Warning: Failed to write {}", lhFile);
            } else {
                Log::info("  {} vertices, {} time points", nLh, static_cast<int>(lhData.cols()));
            }
        }

        // Right hemisphere
        if (nRh > 0) {
            const std::string rhFile = stcBase + "-rh.stc";
            Log::info("Writing right hemisphere STC to {}...", rhFile);

            MatrixXd rhData = stc.data.bottomRows(nRh);
            InvSourceEstimate rhStc(rhData, vertno[1], stc.tmin, stc.tstep);

            if (!rhStc.write(path_from_utf8(rhFile))) {
                Log::warning("Warning: Failed to write {}", rhFile);
            } else {
                Log::info("  {} vertices, {} time points", nRh, static_cast<int>(rhData.cols()));
            }
        }

        //=====================================================================================
        // FsLabel-based ASCII output
        //=====================================================================================

        const StringList labelFiles = parser.values("label");
        if (doLabelTag && !labelFiles.empty()) {
            Log::info("\nWriting label-based ASCII output...");

            for (const std::string& labelFile : labelFiles) {
                FsLabel label;
                if (!FsLabel::read(path_from_utf8(labelFile), label)) {
                    Log::warning("  WARNING: Could not read label {}", labelFile);
                    continue;
                }
                Log::info("  FsLabel: {} ({} vertices, hemi={})", label.name, static_cast<int>(label.vertices.size()), label.hemi);

                // Determine hemisphere and write
                // hemi: 0 = lh, 1 = rh (FsLabel convention)
                VectorXi hemiVertno;
                int hemiOffset = 0;
                std::string hemiName;

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
                    if (label.name.find("lh") != std::string::npos) {
                        hemiVertno = vertno[0];
                        hemiOffset = 0;
                        hemiName = "lh";
                    } else {
                        hemiVertno = vertno[1];
                        hemiOffset = nLh;
                        hemiName = "rh";
                    }
                }

                const std::string outName = std::format("{}-{}-{}.label", stcBase, labelTag, hemiName);
                writeLabelOutput(label, stc.data, hemiVertno, hemiOffset,
                                 timesVec, outName, doLabelCoords, doLabelTimeByTime);
            }
        }
    } else {
        // Single source space (e.g., volume) — write as single STC
        const std::string outFile = stcBase + ".stc";
        Log::info("\nWriting STC to {}...", outFile);

        if (auto written = stc.write(path_from_utf8(outFile)); !written) {
            Log::critical("{}", written.error().what());
            return 1;
        }
        Log::info("  {} vertices, {} time points", static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));
    }

    //=========================================================================================================
    // Dipole snapshot output (--dip / --diptime)
    //=========================================================================================================

    if (parser.is_set("dip")) {
        const std::string dipFile = parser.value("dip");
        double dipTime_ms = parser.is_set("diptime") ? toDouble(parser.value("diptime")) : 0.0;
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

    const StringList pickValues = parser.values("pick");
    if (!pickValues.empty()) {
        Log::info("\nExtracting source estimates at {} time point(s)...", static_cast<int>(pickValues.size()));

        for (const std::string& pickStr : pickValues) {
            double pickTime_ms = toDouble(pickStr);
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
                const int nLhV = static_cast<int>(vertno[0].size());

                for (int h = 0; h < 2; ++h) {
                    const std::string hemiName = (h == 0) ? "lh" : "rh";
                    int offset = (h == 0) ? 0 : nLhV;
                    const int nVert = static_cast<int>(vertno[h].size());

                    const std::string pickFile = stcBase + std::format("-pick{:.0f}-{}.w", pickTime_ms, hemiName);
                    std::ofstream out(path_from_utf8(pickFile));
                    if (!out) {
                        Log::warning("  WARNING: Could not write {}", pickFile);
                        continue;
                    }
                    out << "# Source estimate at " << sci(timesVec(pickIdx) * 1000.0) << " ms\n";
                    out << "# vertex value\n";
                    for (int v = 0; v < nVert; ++v) {
                        out << vertno[h](v) << "\t" << sci(stc.data(offset + v, pickIdx)) << "\n";
                    }
                    out.close();
                    Log::info("    {}: {} vertices -> {}", hemiName, nVert, pickFile);
                }
            }
        }
    }

    //=========================================================================================================
    // Predicted data output (--pred)
    //=========================================================================================================

    if (parser.is_set("pred")) {
        const std::string predFile = parser.value("pred");
        Log::info("\nComputing predicted sensor data...");

        // Read forward solution for prediction
        MNEForwardSolution predFwd;
        if (parser.is_set("predfwd")) {
            predFwd = MNEForwardSolution(path_from_utf8(parser.value("predfwd")), {.surf_ori = true});
        } else if (useFwdAsData) {
            predFwd = MNEForwardSolution(path_from_utf8(parser.value("fwd")), {.surf_ori = true});
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
