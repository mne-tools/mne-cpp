//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2002
 * @brief    Implements mne_compute_raw_inverse application.
 *           Port of the original MNE-C mne_compute_raw_inverse by Matti Hamalainen.
 *
 *           Computes inverse solutions from raw or evoked FIFF data using
 *           an inverse operator and label-restricted source estimation.
 *           The output is written as STC files for each label.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/mne/mne.h>
#include <mne/mne/mne_inverse_operator.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_source_spaces.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>
#include <mne/fs/fs_label.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <print>
#include <string>
#include <system_error>
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
constexpr auto BIG_TIME = 1e6f;

//=============================================================================================================
// HELPER: detect whether a FIFF file is raw or evoked
//=============================================================================================================

static bool isRawFile(const std::string& fileName)
{
    FiffStream stream(path_from_utf8(fileName));
    if (!stream.open()) {
        return false;
    }

    std::shared_ptr<FiffDirNode> dirTree = stream.dirtree();

    // Check for evoked data first — evoked files also use FIFFB_PROCESSED_DATA
    bool hasEvoked = dirTree->has_kind(FIFFB_EVOKED);
    if (hasEvoked) {
        stream.close();
        return false;
    }

    bool hasRaw = dirTree->has_kind(FIFFB_RAW_DATA) ||
        dirTree->has_kind(FIFFB_CONTINUOUS_DATA);
    stream.close();
    return hasRaw;
}

//=============================================================================================================
// HELPER: sanitize a string for safe use as part of a filename
//=============================================================================================================

static std::string sanitizeForFilename(const std::string& s)
{
    // Whitespace runs become one '_', then everything but [a-z0-9_-] is dropped
    std::string result;
    bool pendingSpace = false;
    for (const char ch : trimmed(s)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            pendingSpace = true;
            continue;
        }
        if (pendingSpace) {
            result += '_';
            pendingSpace = false;
        }
        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if ((lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9') || lower == '_' || lower == '-')
            result += lower;
    }
    if (result.empty())
        result = "unknown";
    return result;
}

//=============================================================================================================
// HELPER: file name parts, like QFileInfo
//=============================================================================================================

// The file name without its last extension, like QFileInfo::completeBaseName()
static std::string stemOf(const std::string& file)
{
    const std::string name = utf8_from_path(path_from_utf8(file).filename());
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

// The directory of a file, "." if it has none, like QFileInfo::path()
static std::string directoryOf(const std::string& file)
{
    const std::filesystem::path parent = path_from_utf8(file).parent_path();
    return parent.empty() ? std::string(".") : utf8_from_path(parent);
}

// The file name with its extension, like QFileInfo::fileName()
static std::string fileNameOf(const std::string& file)
{
    return utf8_from_path(path_from_utf8(file).filename());
}

// A number like QString::toFloat(): 0 if the text is not a number
static float floatValue(const std::string& text)
{
    return static_cast<float>(to_double(text).value_or(0.0));
}

//=============================================================================================================
// HELPER: compose output file name from label file name
//=============================================================================================================

[[maybe_unused]] static std::string composeOutName(const std::string& labelName, const std::string& tag, bool raw)
{
    const std::string base = stemOf(labelName);
    const std::string dir = directoryOf(labelName);
    const std::string suffix = raw ? "_raw.fif" : "-stc.fif";

    if (!tag.empty()) {
        return dir + "/" + base + "-" + tag + suffix;
    }
    return dir + "/" + base + suffix;
}

//=============================================================================================================
// HELPER: detect label hemisphere from file name
//=============================================================================================================

static int labelHemisphere(const std::string& name)
{
    const std::string base = fileNameOf(name);

    if (base.ends_with("-lh.label") || (base.starts_with("lh.") && base.ends_with(".label"))) {
        return 0; // left hemisphere
    } else if (base.ends_with("-rh.label") || (base.starts_with("rh.") && base.ends_with(".label"))) {
        return 1; // right hemisphere
    }
    return -1;
}

//=============================================================================================================
// HELPER: find label files in a directory
//=============================================================================================================

static StringList findLabelsInDir(const std::string& dir)
{
    // Like QDirIterator(dir, {"*.label"}, QDir::Files): visible files whose name ends in .label in any case
    StringList labels;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(path_from_utf8(dir), ec), end; !ec && it != end; it.increment(ec)) {
        const std::string name = utf8_from_path(it->path().filename());
        if (name.starts_with('.') || name.size() < 6 || !iequals(std::string_view(name).substr(name.size() - 6), ".label"))
            continue;
        std::error_code typeEc;
        if (!it->is_regular_file(typeEc))
            continue;
        labels.push_back(std::format("{}{}{}", dir, dir.ends_with('/') ? "" : "/", name));
    }
    std::ranges::sort(labels);
    return labels;
}

//=============================================================================================================
// Process a single label: compute source estimate for the label vertices
//=============================================================================================================

static InvSourceEstimate processLabel(const std::string& labelFile,
                                      const MNEInverseOperator& invOp,
                                      const MatrixXd& data,
                                      float tmin,
                                      float tstep,
                                      float lambda2,
                                      const std::string& method,
                                      bool pickNormal)
{
    // Read the label
    FsLabel label;
    if (!FsLabel::read(path_from_utf8(labelFile), label)) {
        Log::warning("Failed to read label file: {}", labelFile);
        return InvSourceEstimate();
    }

    int hemi = labelHemisphere(labelFile);
    if (hemi < 0) {
        Log::warning("Cannot determine hemisphere for label: {}", labelFile);
        return InvSourceEstimate();
    }
    label.hemi = hemi;

    Log::info("  FsLabel {}: {} vertices ({} hemisphere)", labelFile, static_cast<long>(label.vertices.size()), hemi == 0 ? "left" : "right");

    // Set up minimum norm with label restriction
    InvMinimumNorm minimumNorm(invOp, lambda2, method);
    minimumNorm.doInverseSetup(1, pickNormal);

    // Compute inverse for all data
    InvSourceEstimate stc = minimumNorm.calculateInverse(data, tmin, tstep, pickNormal);

    if (stc.empty()) {
        Log::warning("Inverse computation returned empty result for label: {}", labelFile);
        return InvSourceEstimate();
    }

    // Restrict to label vertices
    VectorXi labelIndices = stc.indices_by_label(std::span(&label, 1), false);

    if (labelIndices.size() == 0) {
        Log::warning("No source vertices found in label: {}", labelFile);
        return InvSourceEstimate();
    }

    // Extract label-restricted data
    MatrixXd labelData(labelIndices.size(), stc.data.cols());
    VectorXi labelVertices(labelIndices.size());
    for (int i = 0; i < labelIndices.size(); ++i) {
        labelData.row(i) = stc.data.row(labelIndices(i));
        labelVertices(i) = stc.vertices(labelIndices(i));
    }

    return InvSourceEstimate(labelData, labelVertices, stc.tmin, stc.tstep);
}

//=============================================================================================================
// Process label directory: compute average waveform per label
//=============================================================================================================

static InvSourceEstimate processLabelDir(const std::string& labelDir,
                                         const MNEInverseOperator& invOp,
                                         const MatrixXd& data,
                                         float tmin,
                                         float tstep,
                                         float lambda2,
                                         const std::string& method,
                                         StringList& labelNames)
{
    const StringList labelFiles = findLabelsInDir(labelDir);
    if (labelFiles.empty()) {
        Log::warning("No label files found in directory: {}", labelDir);
        return InvSourceEstimate();
    }

    Log::info("Found {} label files in {}", static_cast<long long>(labelFiles.size()), labelDir);

    // Compute full inverse solution first (with pickNormal=true for labeldir)
    InvMinimumNorm minimumNorm(invOp, lambda2, method);
    minimumNorm.doInverseSetup(1, true);

    InvSourceEstimate fullStc = minimumNorm.calculateInverse(data, tmin, tstep, true);
    if (fullStc.empty()) {
        Log::warning("Full inverse computation returned empty result.");
        return InvSourceEstimate();
    }

    // For each label, compute the average source waveform
    MatrixXd avgData(std::ssize(labelFiles), fullStc.data.cols());
    VectorXi avgVertices(std::ssize(labelFiles));
    avgData.setZero();

    int validLabels = 0;
    for (int i = 0; i < std::ssize(labelFiles); ++i) {
        FsLabel label;
        if (!FsLabel::read(path_from_utf8(labelFiles[i]), label)) {
            Log::warning("  Skipping unreadable label: {}", labelFiles[i]);
            continue;
        }

        int hemi = labelHemisphere(labelFiles[i]);
        if (hemi < 0) {
            Log::warning("  Skipping label with unknown hemisphere: {}", labelFiles[i]);
            continue;
        }
        label.hemi = hemi;

        // Find label vertices in the source estimate
        VectorXi labelIndices = fullStc.indices_by_label(std::span(&label, 1), false);

        if (labelIndices.size() == 0) {
            Log::warning("  No source vertices found in label: {}", labelFiles[i]);
            continue;
        }

        // Compute average of source waveforms across label vertices
        RowVectorXd avg = RowVectorXd::Zero(fullStc.data.cols());
        for (int j = 0; j < labelIndices.size(); ++j) {
            avg += fullStc.data.row(labelIndices(j));
        }
        avg /= static_cast<double>(labelIndices.size());

        avgData.row(validLabels) = avg;

        // Use vertex closest to center of mass
        avgVertices(validLabels) = fullStc.vertices(labelIndices(0));

        labelNames.push_back(fileNameOf(labelFiles[i]));

        Log::info("  FsLabel {}: {} vertices, averaged", labelNames.back(), static_cast<long>(labelIndices.size()));

        validLabels++;
    }

    if (validLabels == 0) {
        Log::warning("No valid labels found.");
        return InvSourceEstimate();
    }

    // Trim to actual number of valid labels
    avgData.conservativeResize(validLabels, avgData.cols());
    avgVertices.conservativeResize(validLabels);

    return InvSourceEstimate(avgData, avgVertices, fullStc.tmin, fullStc.tstep);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_compute_raw_inverse application.
 * This is a port of the original MNE-C mne_compute_raw_inverse by Matti Hamalainen.
 *
 * It reads raw or evoked FIFF data and an inverse operator, then computes
 * minimum norm (MNE/dSPM/sLORETA) source estimates restricted to specified
 * label regions. Results are written as STC files.
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
        "Compute raw/evoked inverse solution (MNE/dSPM/sLORETA) restricted to labels.\n"
        "Port of the original MNE-C mne_compute_raw_inverse by Matti Hamalainen.\n\n"
        "Produces source waveforms from labels. Output the results as STC files.");
    parser.add_help_option();
    parser.add_version_option();

    // --in: Input raw or evoked FIFF file
    parser.add_option({{"in"}, "The raw or evoked data input file.", "file", {}});

    // --inv: Inverse operator file
    parser.add_option({{"inv"}, "The inverse operator file.", "file", {}});

    // --snr: SNR estimate
    parser.add_option({{"snr"}, "SNR to use (default: 1.0).", "value", "1.0"});

    // --nave: Number of averages
    parser.add_option({{"nave"}, "Number of averages (default: 1 for raw, from data for evoked).", "number", {}});

    // --set: Evoked data set number (-1 or omitted = process all sets)
    parser.add_option({{"set"}, "Evoked data set number to use (default: process all sets).", "number", {}});

    // --bmin: Baseline start time in ms
    parser.add_option({{"bmin"}, "Baseline starting time in ms.", "time/ms", {}});

    // --bmax: Baseline end time in ms
    parser.add_option({{"bmax"}, "Baseline ending time in ms.", "time/ms", {}});

    // --label: FsLabel file(s) to process (can be specified multiple times)
    parser.add_option({{"label"}, "FsLabel file to process (can have multiple).", "file", {}});

    // --labeldir: Process all labels in this directory
    parser.add_option({{"labeldir"}, "Create file with average waveform for each label in this directory.", "dir", {}});

    // --out: Output file name
    parser.add_option({{"out"}, "Output file name (needed for --labeldir, optional otherwise).", "file", {}});

    // --picknormalcomp: Pick normal component
    parser.add_option({{"picknormalcomp"}, "Pick the current component normal to the cortex.", {}, {}});

    // --spm / --dSPM: Use dSPM
    parser.add_option({{"spm"}, "Use dSPM method.", {}, {}});

    // --sloreta: Use sLORETA
    parser.add_option({{"sloreta"}, "Use sLORETA method.", {}, {}});

    // --mricoord: Use MRI coordinates
    parser.add_option({{"mricoord"}, "List source locations in MRI coordinates instead of head coordinates.", {}, {}});

    // --orignames: Use original label file names
    parser.add_option({{"orignames"}, "Use original label file names in channel names with --labeldir.", {}, {}});

    // --align_z: Align waveform signs
    parser.add_option({{"align_z"}, "Try to align waveform signs using normal information.", {}, {}});

    // --labellist: Output label name list
    parser.add_option({{"labellist"}, "Output the names of labels used from --labeldir to this file.", "file", {}});

    parser.process(argc, argv, "mne_compute_raw_inverse", PROGRAM_VERSION);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.is_set("in")) {
        Log::info("Error: --in option is required.\n");
        std::print("{}", parser.help_text("mne_compute_raw_inverse"));
        return 1;
    }
    if (!parser.is_set("inv")) {
        Log::info("Error: --inv option is required.\n");
        std::print("{}", parser.help_text("mne_compute_raw_inverse"));
        return 1;
    }

    const std::string inName = parser.value("in");
    const std::string invName = parser.value("inv");

    // Labels
    const StringList labelFiles = parser.values("label");
    const std::string labelDir = parser.value("labeldir");
    const std::string outName = parser.value("out");

    if (!labelDir.empty() && outName.empty()) {
        Log::info("Error: --out option must be specified with --labeldir.\n");
        std::print("{}", parser.help_text("mne_compute_raw_inverse"));
        return 1;
    }

    // SNR
    float snr = floatValue(parser.value("snr"));
    if (snr <= 0.0f) {
        Log::critical("Error: SNR must be positive.");
        return 1;
    }
    float lambda2 = 1.0f / (snr * snr);

    // Method
    std::string method = "MNE";
    if (parser.is_set("spm")) {
        method = "dSPM";
    } else if (parser.is_set("sloreta")) {
        method = "sLORETA";
    }

    // Set number for evoked data (-1 means process all sets)
    bool processAllSets = !parser.is_set("set");
    int setNo = processAllSets ? -1 : to_int(parser.value("set")).value_or(0);

    // Baseline
    float bmin = BIG_TIME, bmax = BIG_TIME;
    bool doBaseline = false;
    if (parser.is_set("bmin") && parser.is_set("bmax")) {
        bmin = floatValue(parser.value("bmin")) / 1000.0f;
        bmax = floatValue(parser.value("bmax")) / 1000.0f;
        doBaseline = true;
    }

    // Other options
    bool pickNormal = parser.is_set("picknormalcomp");
    int nave = parser.is_set("nave") ? to_int(parser.value("nave")).value_or(0) : -1;

    // With --labeldir, always pick normal component
    if (!labelDir.empty()) {
        pickNormal = true;
    }

    //=========================================================================================================
    // Report configuration
    //=========================================================================================================

    Log::info("{}", "");
    Log::info("mne_compute_raw_inverse v{}", PROGRAM_VERSION);
    Log::info("========================================");
    Log::info("Input file             : {}", inName);
    if (doBaseline) {
        Log::info("Baseline               : {:10.2f} ... {:10.2f} ms", 1000.0f * bmin, 1000.0f * bmax);
    }
    Log::info("Inverse operator file  : {}", invName);
    Log::info("SNR                    : {:f}", snr);
    Log::info("Method                 : {}", method);
    if (pickNormal) {
        Log::info("Picking normal component to cortex");
    }

    if (!labelFiles.empty()) {
        Log::info("FsLabel files to process :");
        for (const std::string& label : labelFiles) {
            Log::info("  {}", label);
        }
    } else if (!labelDir.empty()) {
        Log::info("FsLabel directory        : {}", labelDir);
    } else {
        Log::info("Full source space inverse (no label restriction)");
    }

    //=========================================================================================================
    // Read the inverse operator
    //=========================================================================================================

    Log::info("\nReading the inverse operator...");
    auto inverseOperatorRead = MNEInverseOperator::read(path_from_utf8(invName));
    if (!inverseOperatorRead) {
        Log::critical("{}", inverseOperatorRead.error().what());
        return 1;
    }
    MNEInverseOperator& inverseOperator = *inverseOperatorRead;
    Log::info("  Inverse operator read successfully.");
    Log::info("  {} channels, {} sources", inverseOperator.nchan, inverseOperator.nsource);

    // Pre-extract label list name for use in processing
    const std::string labelListName = parser.value("labellist");

    //=========================================================================================================
    // Processing lambda: compute inverse and write results for one data set
    //=========================================================================================================

    auto processInverseResults = [&inverseOperator, &lambda2, &method, &pickNormal,
                                  &labelDir, &labelFiles, &labelListName, &inName](const MatrixXd& inputData, float tmin, float tstep,
                                                                                   int curNave, const std::string& curOutName) -> int {
        if (!labelDir.empty()) {
            //-----------------------------------------------------------------------------------------------------
            // FsLabel directory mode: compute average waveform for each label
            //-----------------------------------------------------------------------------------------------------
            Log::info("\nProcessing label directory: {}", labelDir);

            StringList labelNames;
            InvSourceEstimate stc = processLabelDir(labelDir, inverseOperator,
                                                    inputData, tmin, tstep,
                                                    lambda2, method, labelNames);

            if (stc.empty()) {
                Log::critical("Error: FsLabel directory processing failed.");
                return 1;
            }

            // Write the STC result
            // Ensure output has proper extension
            std::string stcOut = curOutName;
            if (!stcOut.ends_with(".stc") && !stcOut.ends_with(".fif")) {
                stcOut += "-lh.stc";
            }

            if (!stc.write(path_from_utf8(stcOut))) {
                Log::critical("Error: Failed to write STC file: {}", stcOut);
                return 1;
            }
            Log::info("\nWrote {} ({} labels, {} time points)", stcOut, static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));

            // Write label list if requested
            if (!labelListName.empty() && !labelNames.empty()) {
                std::ofstream labelListFile(path_from_utf8(labelListName));
                if (labelListFile) {
                    for (const std::string& name : labelNames) {
                        labelListFile << name << "\n";
                    }
                    labelListFile.close();
                    Log::info("FsLabel names output to {}", labelListName);
                }
            }

        } else if (!labelFiles.empty()) {
            //-----------------------------------------------------------------------------------------------------
            // Individual label mode: process each label file separately
            //-----------------------------------------------------------------------------------------------------
            for (const std::string& labelFile : labelFiles) {
                Log::info("\nProcessing label: {}", labelFile);

                InvSourceEstimate stc = processLabel(labelFile, inverseOperator,
                                                     inputData, tmin, tstep,
                                                     lambda2, method, pickNormal);

                if (stc.empty()) {
                    Log::warning("Warning: Skipping label {} (no result).", labelFile);
                    continue;
                }

                // Compose output file name
                std::string tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                std::string stcOut;
                if (!curOutName.empty() && labelFiles.size() == 1) {
                    stcOut = curOutName;
                } else {
                    stcOut = directoryOf(labelFile) + "/" + stemOf(labelFile) + "-" + tag;
                    // Determine hemisphere suffix
                    int hemi = labelHemisphere(labelFile);
                    if (hemi == 0) {
                        stcOut += "-lh.stc";
                    } else {
                        stcOut += "-rh.stc";
                    }
                }

                if (!stc.write(path_from_utf8(stcOut))) {
                    Log::critical("Error: Failed to write STC file: {}", stcOut);
                    continue;
                }
                Log::info("  Wrote {} ({} sources, {} time points)", stcOut, static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));
            }

        } else {
            //-----------------------------------------------------------------------------------------------------
            // Full source space mode: compute inverse for all sources
            //-----------------------------------------------------------------------------------------------------
            Log::info("\nComputing full source space inverse...");

            InvMinimumNorm minimumNorm(inverseOperator, lambda2, method);
            minimumNorm.doInverseSetup(curNave, pickNormal);

            InvSourceEstimate stc = minimumNorm.calculateInverse(inputData, tmin, tstep, pickNormal);

            if (stc.empty()) {
                Log::critical("Error: Full source space inverse computation failed.");
                return 1;
            }

            // Compose output base name
            std::string stcOut;
            if (!curOutName.empty()) {
                stcOut = curOutName;
                // Strip hemisphere suffix if user provided one
                if (stcOut.ends_with("-lh.stc") || stcOut.ends_with("-rh.stc"))
                    stcOut.resize(stcOut.size() - 7);
            } else {
                std::string tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                stcOut = directoryOf(inName) + "/" + stemOf(inName) + "-" + tag;
            }

            // Split the combined source estimate into lh and rh hemispheres.
            // The inverse operator's source space tells us how many sources
            // belong to each hemisphere.
            int nSrcLh = inverseOperator.src[0].nuse;
            int nSrcRh = inverseOperator.src[1].nuse;
            int nSrcTotal = static_cast<int>(stc.data.rows());

            if (nSrcLh + nSrcRh != nSrcTotal) {
                Log::warning("Warning: Source count mismatch (lh={} + rh={} != {}). Writing combined STC to lh file.", nSrcLh, nSrcRh, nSrcTotal);

                const std::string stcOutLh = stcOut + "-lh.stc";
                if (!stc.write(path_from_utf8(stcOutLh))) {
                    Log::critical("Error: Failed to write STC file: {}", stcOutLh);
                    return 1;
                }
                Log::info("  Wrote {} ({} sources, {} time points)", stcOutLh, nSrcTotal, static_cast<int>(stc.data.cols()));
            } else {
                // Write left hemisphere STC
                if (nSrcLh > 0) {
                    InvSourceEstimate stcLh(stc.data.topRows(nSrcLh),
                                            stc.vertices.head(nSrcLh),
                                            stc.tmin, stc.tstep);

                    const std::string stcOutLh = stcOut + "-lh.stc";
                    if (!stcLh.write(path_from_utf8(stcOutLh))) {
                        Log::critical("Error: Failed to write LH STC file: {}", stcOutLh);
                        return 1;
                    }
                    Log::info("  Wrote {} ({} sources, {} time points)", stcOutLh, nSrcLh, static_cast<int>(stc.data.cols()));
                }

                // Write right hemisphere STC
                if (nSrcRh > 0) {
                    InvSourceEstimate stcRh(stc.data.bottomRows(nSrcRh),
                                            stc.vertices.tail(nSrcRh),
                                            stc.tmin, stc.tstep);

                    const std::string stcOutRh = stcOut + "-rh.stc";
                    if (!stcRh.write(path_from_utf8(stcOutRh))) {
                        Log::critical("Error: Failed to write RH STC file: {}", stcOutRh);
                        return 1;
                    }
                    Log::info("  Wrote {} ({} sources, {} time points)", stcOutRh, nSrcRh, static_cast<int>(stc.data.cols()));
                }
            }
        }

        return 0;
    };

    //=========================================================================================================
    // Detect and read input data (raw or evoked), then process
    //=========================================================================================================

    bool isRaw = isRawFile(inName);

    if (isRaw) {
        Log::info("\nReading raw data file: {}", inName);

        if (nave <= 0)
            nave = 1;
        Log::info("  nave = {}", nave);

        FiffRawData raw(path_from_utf8(inName));

        // Pick channels that match the inverse operator
        StringList invChNames;
        for (int i = 0; i < static_cast<int>(inverseOperator.noise_cov.names.size()); ++i) {
            invChNames.push_back(inverseOperator.noise_cov.names[i]);
        }

        RowVectorXi picks = raw.info.pick_channels(invChNames);
        if (picks.size() == 0) {
            Log::critical("Error: No matching channels found between raw data and inverse operator.");
            return 1;
        }

        Log::info("  Picked {} channels from raw data", static_cast<int>(picks.size()));

        // Activate projectors
        for (int k = 0; k < std::ssize(raw.info.projs); ++k) {
            raw.info.projs[k].active = true;
        }

        // Read the full raw segment
        MatrixXd inputData;
        MatrixXd times;
        if (!raw.read_raw_segment(inputData, times, raw.first_samp, raw.last_samp, picks)) {
            Log::critical("Error: Failed to read raw data segment.");
            return 1;
        }

        float tstep = 1.0f / raw.info.sfreq;
        float tmin = static_cast<float>(raw.first_samp) * tstep;

        Log::info("  Read {} samples ({} channels)", static_cast<int>(inputData.cols()), static_cast<int>(inputData.rows()));

        int result = processInverseResults(inputData, tmin, tstep, nave, outName);
        if (result != 0)
            return result;

    } else {
        //-----------------------------------------------------------------------------------------------------
        // Read all evoked data sets
        //-----------------------------------------------------------------------------------------------------
        Log::info("\nReading evoked data file: {}", inName);

        FiffEvoked::Baseline baseline;
        if (doBaseline) {
            baseline = std::pair(bmin, bmax);
        }
        auto read = FiffEvokedSet::read(path_from_utf8(inName), {}, baseline);
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        FiffEvokedSet& evokedSet = *read;

        if (evokedSet.evoked.empty()) {
            Log::critical("Error: No evoked data sets found in {}", inName);
            return 1;
        }

        // Determine which sets to process
        std::vector<int> setsToProcess;
        if (processAllSets) {
            Log::info("  Found {} evoked data set(s):", static_cast<int>(evokedSet.evoked.size()));
            for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
                Log::info("    [{}] {}", i, evokedSet.evoked[i].comment);
                setsToProcess.push_back(i);
            }
        } else {
            if (setNo < 0 || setNo >= std::ssize(evokedSet.evoked)) {
                Log::critical("Error: Set {} out of range (file has {} sets, 0-{})", setNo, static_cast<int>(evokedSet.evoked.size()), static_cast<int>(evokedSet.evoked.size()) - 1);
                return 1;
            }
            setsToProcess.push_back(setNo);
        }

        // Process each selected evoked set
        for (int setIdx : setsToProcess) {
            const FiffEvoked& evoked = evokedSet.evoked[setIdx];
            const std::string& setComment = evoked.comment;
            const std::string commentTag = sanitizeForFilename(setComment);

            Log::info("\n=== Evoked set {}: \"{}\" ===", setIdx, setComment);

            int curNave = (nave > 0) ? nave : evoked.nave;
            Log::info("  nave = {}", curNave);

            // Pick channels matching inverse operator
            FiffEvoked pickedEvoked = evoked.pick_channels(inverseOperator.noise_cov.names);
            Log::info("  Picked {} channels from evoked data", pickedEvoked.info.nchan);

            if (doBaseline) {
                Log::info("  Baseline correction: {:10.2f} ... {:10.2f} ms", 1000.0f * bmin, 1000.0f * bmax);
            } else {
                Log::info("  No baseline setting in effect.");
            }

            MatrixXd inputData = pickedEvoked.data;
            float tmin = pickedEvoked.times(0);
            float tstep = 1.0f / pickedEvoked.info.sfreq;

            Log::info("  Read {} time points ({} channels)", static_cast<int>(inputData.cols()), static_cast<int>(inputData.rows()));

            // Compose per-set output name with set description
            std::string setOutName;
            if (!outName.empty()) {
                setOutName = std::format("{}-{}", outName, commentTag);
            } else {
                std::string tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                setOutName = std::format("{}/{}-{}-{}", directoryOf(inName), stemOf(inName), tag, commentTag);
            }

            int result = processInverseResults(inputData, tmin, tstep, curNave, setOutName);
            if (result != 0)
                return result;
        }
    }

    Log::info("\nFinished.");
    return 0;
}
