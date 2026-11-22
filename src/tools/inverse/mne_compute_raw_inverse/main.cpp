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
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

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

static bool isRawFile(const QString& fileName)
{
    FiffStream stream(fileName.toStdU16String());
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

static QString sanitizeForFilename(const QString& s)
{
    QString result = s.simplified().toLower();
    result.replace(' ', '_');
    result.replace(QRegularExpression("[^a-z0-9_-]"), "");
    if (result.isEmpty())
        result = "unknown";
    return result;
}

//=============================================================================================================
// HELPER: compose output file name from label file name
//=============================================================================================================

[[maybe_unused]] static QString composeOutName(const QString& labelName, const QString& tag, bool raw)
{
    QFileInfo fi(labelName);
    QString base = fi.completeBaseName();
    QString dir = fi.path();
    QString suffix = raw ? "_raw.fif" : "-stc.fif";

    if (!tag.isEmpty()) {
        return dir + "/" + base + "-" + tag + suffix;
    }
    return dir + "/" + base + suffix;
}

//=============================================================================================================
// HELPER: detect label hemisphere from file name
//=============================================================================================================

static int labelHemisphere(const QString& name)
{
    QFileInfo fi(name);
    QString base = fi.fileName();

    if (base.endsWith("-lh.label") || (base.startsWith("lh.") && base.endsWith(".label"))) {
        return 0; // left hemisphere
    } else if (base.endsWith("-rh.label") || (base.startsWith("rh.") && base.endsWith(".label"))) {
        return 1; // right hemisphere
    }
    return -1;
}

//=============================================================================================================
// HELPER: find label files in a directory
//=============================================================================================================

static QStringList findLabelsInDir(const QString& dir)
{
    QStringList labels;
    QDirIterator it(dir, QStringList() << "*.label", QDir::Files);
    while (it.hasNext()) {
        labels.append(it.next());
    }
    labels.sort();
    return labels;
}

//=============================================================================================================
// Process a single label: compute source estimate for the label vertices
//=============================================================================================================

static InvSourceEstimate processLabel(const QString& labelFile,
                                      const MNEInverseOperator& invOp,
                                      const MatrixXd& data,
                                      float tmin,
                                      float tstep,
                                      float lambda2,
                                      const QString& method,
                                      bool pickNormal)
{
    // Read the label
    FsLabel label;
    if (!FsLabel::read(labelFile.toStdU16String(), label)) {
        Log::warning("Failed to read label file: {}", labelFile.toStdString());
        return InvSourceEstimate();
    }

    int hemi = labelHemisphere(labelFile);
    if (hemi < 0) {
        Log::warning("Cannot determine hemisphere for label: {}", labelFile.toStdString());
        return InvSourceEstimate();
    }
    label.hemi = hemi;

    Log::info("  FsLabel {}: {} vertices ({} hemisphere)", labelFile.toStdString(), static_cast<long>(label.vertices.size()), hemi == 0 ? "left" : "right");

    // Set up minimum norm with label restriction
    InvMinimumNorm minimumNorm(invOp, lambda2, method.toStdString());
    minimumNorm.doInverseSetup(1, pickNormal);

    // Compute inverse for all data
    InvSourceEstimate stc = minimumNorm.calculateInverse(data, tmin, tstep, pickNormal);

    if (stc.empty()) {
        Log::warning("Inverse computation returned empty result for label: {}", labelFile.toStdString());
        return InvSourceEstimate();
    }

    // Restrict to label vertices
    VectorXi labelIndices = stc.indices_by_label(std::span(&label, 1), false);

    if (labelIndices.size() == 0) {
        Log::warning("No source vertices found in label: {}", labelFile.toStdString());
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

static InvSourceEstimate processLabelDir(const QString& labelDir,
                                         const MNEInverseOperator& invOp,
                                         const MatrixXd& data,
                                         float tmin,
                                         float tstep,
                                         float lambda2,
                                         const QString& method,
                                         QStringList& labelNames)
{
    QStringList labelFiles = findLabelsInDir(labelDir);
    if (labelFiles.isEmpty()) {
        Log::warning("No label files found in directory: {}", labelDir.toStdString());
        return InvSourceEstimate();
    }

    Log::info("Found {} label files in {}", static_cast<long long>(labelFiles.size()), labelDir.toStdString());

    // Compute full inverse solution first (with pickNormal=true for labeldir)
    InvMinimumNorm minimumNorm(invOp, lambda2, method.toStdString());
    minimumNorm.doInverseSetup(1, true);

    InvSourceEstimate fullStc = minimumNorm.calculateInverse(data, tmin, tstep, true);
    if (fullStc.empty()) {
        Log::warning("Full inverse computation returned empty result.");
        return InvSourceEstimate();
    }

    // For each label, compute the average source waveform
    MatrixXd avgData(labelFiles.size(), fullStc.data.cols());
    VectorXi avgVertices(labelFiles.size());
    avgData.setZero();

    int validLabels = 0;
    for (int i = 0; i < labelFiles.size(); ++i) {
        FsLabel label;
        if (!FsLabel::read(labelFiles[i].toStdU16String(), label)) {
            Log::warning("  Skipping unreadable label: {}", labelFiles[i].toStdString());
            continue;
        }

        int hemi = labelHemisphere(labelFiles[i]);
        if (hemi < 0) {
            Log::warning("  Skipping label with unknown hemisphere: {}", labelFiles[i].toStdString());
            continue;
        }
        label.hemi = hemi;

        // Find label vertices in the source estimate
        VectorXi labelIndices = fullStc.indices_by_label(std::span(&label, 1), false);

        if (labelIndices.size() == 0) {
            Log::warning("  No source vertices found in label: {}", labelFiles[i].toStdString());
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

        QFileInfo fi(labelFiles[i]);
        labelNames.append(fi.fileName());

        Log::info("  FsLabel {}: {} vertices, averaged", fi.fileName().toStdString(), static_cast<long>(labelIndices.size()));

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
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_compute_raw_inverse");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Compute raw/evoked inverse solution (MNE/dSPM/sLORETA) restricted to labels.\n"
        "Port of the original MNE-C mne_compute_raw_inverse by Matti Hamalainen.\n\n"
        "Produces source waveforms from labels. Output the results as STC files.");
    parser.addHelpOption();
    parser.addVersionOption();

    // --in: Input raw or evoked FIFF file
    QCommandLineOption inOpt(QStringList() << "in",
                             "The raw or evoked data input file.", "file");
    parser.addOption(inOpt);

    // --inv: Inverse operator file
    QCommandLineOption invOpt(QStringList() << "inv",
                              "The inverse operator file.", "file");
    parser.addOption(invOpt);

    // --snr: SNR estimate
    QCommandLineOption snrOpt(QStringList() << "snr",
                              "SNR to use (default: 1.0).", "value", "1.0");
    parser.addOption(snrOpt);

    // --nave: Number of averages
    QCommandLineOption naveOpt(QStringList() << "nave",
                               "Number of averages (default: 1 for raw, from data for evoked).", "number");
    parser.addOption(naveOpt);

    // --set: Evoked data set number (-1 or omitted = process all sets)
    QCommandLineOption setOpt(QStringList() << "set",
                              "Evoked data set number to use (default: process all sets).", "number");
    parser.addOption(setOpt);

    // --bmin: Baseline start time in ms
    QCommandLineOption bminOpt(QStringList() << "bmin",
                               "Baseline starting time in ms.", "time/ms");
    parser.addOption(bminOpt);

    // --bmax: Baseline end time in ms
    QCommandLineOption bmaxOpt(QStringList() << "bmax",
                               "Baseline ending time in ms.", "time/ms");
    parser.addOption(bmaxOpt);

    // --label: FsLabel file(s) to process (can be specified multiple times)
    QCommandLineOption labelOpt(QStringList() << "label",
                                "FsLabel file to process (can have multiple).", "file");
    parser.addOption(labelOpt);

    // --labeldir: Process all labels in this directory
    QCommandLineOption labeldirOpt(QStringList() << "labeldir",
                                   "Create file with average waveform for each label in this directory.", "dir");
    parser.addOption(labeldirOpt);

    // --out: Output file name
    QCommandLineOption outOpt(QStringList() << "out",
                              "Output file name (needed for --labeldir, optional otherwise).", "file");
    parser.addOption(outOpt);

    // --picknormalcomp: Pick normal component
    QCommandLineOption pickNormalOpt(QStringList() << "picknormalcomp",
                                     "Pick the current component normal to the cortex.");
    parser.addOption(pickNormalOpt);

    // --spm / --dSPM: Use dSPM
    QCommandLineOption spmOpt(QStringList() << "spm",
                              "Use dSPM method.");
    parser.addOption(spmOpt);

    // --sloreta: Use sLORETA
    QCommandLineOption sloretaOpt(QStringList() << "sloreta",
                                  "Use sLORETA method.");
    parser.addOption(sloretaOpt);

    // --mricoord: Use MRI coordinates
    QCommandLineOption mriCoordOpt(QStringList() << "mricoord",
                                   "List source locations in MRI coordinates instead of head coordinates.");
    parser.addOption(mriCoordOpt);

    // --orignames: Use original label file names
    QCommandLineOption origNamesOpt(QStringList() << "orignames",
                                    "Use original label file names in channel names with --labeldir.");
    parser.addOption(origNamesOpt);

    // --align_z: Align waveform signs
    QCommandLineOption alignZOpt(QStringList() << "align_z",
                                 "Try to align waveform signs using normal information.");
    parser.addOption(alignZOpt);

    // --labellist: Output label name list
    QCommandLineOption labelListOpt(QStringList() << "labellist",
                                    "Output the names of labels used from --labeldir to this file.", "file");
    parser.addOption(labelListOpt);

    parser.process(app);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    if (!parser.isSet(inOpt)) {
        Log::info("Error: --in option is required.\n");
        parser.showHelp(1);
    }
    if (!parser.isSet(invOpt)) {
        Log::info("Error: --inv option is required.\n");
        parser.showHelp(1);
    }

    QString inName = parser.value(inOpt);
    QString invName = parser.value(invOpt);

    // Labels
    QStringList labelFiles = parser.values(labelOpt);
    QString labelDir = parser.value(labeldirOpt);
    QString outName = parser.value(outOpt);

    if (!labelDir.isEmpty() && outName.isEmpty()) {
        Log::info("Error: --out option must be specified with --labeldir.\n");
        parser.showHelp(1);
    }

    // SNR
    float snr = parser.value(snrOpt).toFloat();
    if (snr <= 0.0f) {
        Log::critical("Error: SNR must be positive.");
        return 1;
    }
    float lambda2 = 1.0f / (snr * snr);

    // Method
    QString method = "MNE";
    if (parser.isSet(spmOpt)) {
        method = "dSPM";
    } else if (parser.isSet(sloretaOpt)) {
        method = "sLORETA";
    }

    // Set number for evoked data (-1 means process all sets)
    bool processAllSets = !parser.isSet(setOpt);
    int setNo = processAllSets ? -1 : parser.value(setOpt).toInt();

    // Baseline
    float bmin = BIG_TIME, bmax = BIG_TIME;
    bool doBaseline = false;
    if (parser.isSet(bminOpt) && parser.isSet(bmaxOpt)) {
        bmin = parser.value(bminOpt).toFloat() / 1000.0f;
        bmax = parser.value(bmaxOpt).toFloat() / 1000.0f;
        doBaseline = true;
    }

    // Other options
    bool pickNormal = parser.isSet(pickNormalOpt);
    int nave = parser.isSet(naveOpt) ? parser.value(naveOpt).toInt() : -1;

    // With --labeldir, always pick normal component
    if (!labelDir.isEmpty()) {
        pickNormal = true;
    }

    //=========================================================================================================
    // Report configuration
    //=========================================================================================================

    Log::info("{}", "");
    Log::info("mne_compute_raw_inverse v{}", PROGRAM_VERSION);
    Log::info("========================================");
    Log::info("Input file             : {}", inName.toStdString());
    if (doBaseline) {
        Log::info("Baseline               : {:10.2f} ... {:10.2f} ms", 1000.0f * bmin, 1000.0f * bmax);
    }
    Log::info("Inverse operator file  : {}", invName.toStdString());
    Log::info("SNR                    : {:f}", snr);
    Log::info("Method                 : {}", method.toStdString());
    if (pickNormal) {
        Log::info("Picking normal component to cortex");
    }

    if (!labelFiles.isEmpty()) {
        Log::info("FsLabel files to process :");
        for (const QString& label : labelFiles) {
            Log::info("  {}", label.toStdString());
        }
    } else if (!labelDir.isEmpty()) {
        Log::info("FsLabel directory        : {}", labelDir.toStdString());
    } else {
        Log::info("Full source space inverse (no label restriction)");
    }

    //=========================================================================================================
    // Read the inverse operator
    //=========================================================================================================

    Log::info("\nReading the inverse operator...");
    auto inverseOperatorRead = MNEInverseOperator::read(invName.toStdU16String());
    if (!inverseOperatorRead) {
        Log::critical("{}", inverseOperatorRead.error().what());
        return 1;
    }
    MNEInverseOperator& inverseOperator = *inverseOperatorRead;
    Log::info("  Inverse operator read successfully.");
    Log::info("  {} channels, {} sources", inverseOperator.nchan, inverseOperator.nsource);

    // Pre-extract label list name for use in processing
    QString labelListName = parser.value(labelListOpt);

    //=========================================================================================================
    // Processing lambda: compute inverse and write results for one data set
    //=========================================================================================================

    auto processInverseResults = [&inverseOperator, &lambda2, &method, &pickNormal,
                                  &labelDir, &labelFiles, &labelListName, &inName](const MatrixXd& inputData, float tmin, float tstep,
                                                                                   int curNave, const QString& curOutName) -> int {
        if (!labelDir.isEmpty()) {
            //-----------------------------------------------------------------------------------------------------
            // FsLabel directory mode: compute average waveform for each label
            //-----------------------------------------------------------------------------------------------------
            Log::info("\nProcessing label directory: {}", labelDir.toStdString());

            QStringList labelNames;
            InvSourceEstimate stc = processLabelDir(labelDir, inverseOperator,
                                                    inputData, tmin, tstep,
                                                    lambda2, method, labelNames);

            if (stc.empty()) {
                Log::critical("Error: FsLabel directory processing failed.");
                return 1;
            }

            // Write the STC result
            // Ensure output has proper extension
            QString stcOut = curOutName;
            if (!stcOut.endsWith(".stc") && !stcOut.endsWith(".fif")) {
                stcOut += "-lh.stc";
            }

            if (!stc.write(stcOut.toStdU16String())) {
                Log::critical("Error: Failed to write STC file: {}", stcOut.toStdString());
                return 1;
            }
            Log::info("\nWrote {} ({} labels, {} time points)", stcOut.toStdString(), static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));

            // Write label list if requested
            if (!labelListName.isEmpty() && !labelNames.isEmpty()) {
                QFile labelListFile(labelListName);
                if (labelListFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&labelListFile);
                    for (const QString& name : labelNames) {
                        out << name << "\n";
                    }
                    labelListFile.close();
                    Log::info("FsLabel names output to {}", labelListName.toStdString());
                }
            }

        } else if (!labelFiles.isEmpty()) {
            //-----------------------------------------------------------------------------------------------------
            // Individual label mode: process each label file separately
            //-----------------------------------------------------------------------------------------------------
            for (const QString& labelFile : labelFiles) {
                Log::info("\nProcessing label: {}", labelFile.toStdString());

                InvSourceEstimate stc = processLabel(labelFile, inverseOperator,
                                                     inputData, tmin, tstep,
                                                     lambda2, method, pickNormal);

                if (stc.empty()) {
                    Log::warning("Warning: Skipping label {} (no result).", labelFile.toStdString());
                    continue;
                }

                // Compose output file name
                QString tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                QString stcOut;
                if (!curOutName.isEmpty() && labelFiles.size() == 1) {
                    stcOut = curOutName;
                } else {
                    QFileInfo fi(labelFile);
                    stcOut = fi.path() + "/" + fi.completeBaseName() + "-" + tag;
                    // Determine hemisphere suffix
                    int hemi = labelHemisphere(labelFile);
                    if (hemi == 0) {
                        stcOut += "-lh.stc";
                    } else {
                        stcOut += "-rh.stc";
                    }
                }

                if (!stc.write(stcOut.toStdU16String())) {
                    Log::critical("Error: Failed to write STC file: {}", stcOut.toStdString());
                    continue;
                }
                Log::info("  Wrote {} ({} sources, {} time points)", stcOut.toStdString(), static_cast<int>(stc.data.rows()), static_cast<int>(stc.data.cols()));
            }

        } else {
            //-----------------------------------------------------------------------------------------------------
            // Full source space mode: compute inverse for all sources
            //-----------------------------------------------------------------------------------------------------
            Log::info("\nComputing full source space inverse...");

            InvMinimumNorm minimumNorm(inverseOperator, lambda2, method.toStdString());
            minimumNorm.doInverseSetup(curNave, pickNormal);

            InvSourceEstimate stc = minimumNorm.calculateInverse(inputData, tmin, tstep, pickNormal);

            if (stc.empty()) {
                Log::critical("Error: Full source space inverse computation failed.");
                return 1;
            }

            // Compose output base name
            QString stcOut;
            if (!curOutName.isEmpty()) {
                stcOut = curOutName;
                // Strip hemisphere suffix if user provided one
                if (stcOut.endsWith("-lh.stc"))
                    stcOut.chop(7);
                else if (stcOut.endsWith("-rh.stc"))
                    stcOut.chop(7);
            } else {
                QFileInfo fi(inName);
                QString tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                stcOut = fi.path() + "/" + fi.completeBaseName() + "-" + tag;
            }

            // Split the combined source estimate into lh and rh hemispheres.
            // The inverse operator's source space tells us how many sources
            // belong to each hemisphere.
            int nSrcLh = inverseOperator.src[0].nuse;
            int nSrcRh = inverseOperator.src[1].nuse;
            int nSrcTotal = static_cast<int>(stc.data.rows());

            if (nSrcLh + nSrcRh != nSrcTotal) {
                Log::warning("Warning: Source count mismatch (lh={} + rh={} != {}). Writing combined STC to lh file.", nSrcLh, nSrcRh, nSrcTotal);

                QString stcOutLh = stcOut + "-lh.stc";
                if (!stc.write(stcOutLh.toStdU16String())) {
                    Log::critical("Error: Failed to write STC file: {}", stcOutLh.toStdString());
                    return 1;
                }
                Log::info("  Wrote {} ({} sources, {} time points)", stcOutLh.toStdString(), nSrcTotal, static_cast<int>(stc.data.cols()));
            } else {
                // Write left hemisphere STC
                if (nSrcLh > 0) {
                    InvSourceEstimate stcLh(stc.data.topRows(nSrcLh),
                                            stc.vertices.head(nSrcLh),
                                            stc.tmin, stc.tstep);

                    QString stcOutLh = stcOut + "-lh.stc";
                    if (!stcLh.write(stcOutLh.toStdU16String())) {
                        Log::critical("Error: Failed to write LH STC file: {}", stcOutLh.toStdString());
                        return 1;
                    }
                    Log::info("  Wrote {} ({} sources, {} time points)", stcOutLh.toStdString(), nSrcLh, static_cast<int>(stc.data.cols()));
                }

                // Write right hemisphere STC
                if (nSrcRh > 0) {
                    InvSourceEstimate stcRh(stc.data.bottomRows(nSrcRh),
                                            stc.vertices.tail(nSrcRh),
                                            stc.tmin, stc.tstep);

                    QString stcOutRh = stcOut + "-rh.stc";
                    if (!stcRh.write(stcOutRh.toStdU16String())) {
                        Log::critical("Error: Failed to write RH STC file: {}", stcOutRh.toStdString());
                        return 1;
                    }
                    Log::info("  Wrote {} ({} sources, {} time points)", stcOutRh.toStdString(), nSrcRh, static_cast<int>(stc.data.cols()));
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
        Log::info("\nReading raw data file: {}", inName.toStdString());

        if (nave <= 0)
            nave = 1;
        Log::info("  nave = {}", nave);

        FiffRawData raw(inName.toStdU16String());

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
        Log::info("\nReading evoked data file: {}", inName.toStdString());

        FiffEvoked::Baseline baseline;
        if (doBaseline) {
            baseline = std::pair(bmin, bmax);
        }
        auto read = FiffEvokedSet::read(inName.toStdU16String(), {}, baseline);
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        FiffEvokedSet& evokedSet = *read;

        if (evokedSet.evoked.empty()) {
            Log::critical("Error: No evoked data sets found in {}", inName.toStdString());
            return 1;
        }

        // Determine which sets to process
        QList<int> setsToProcess;
        if (processAllSets) {
            Log::info("  Found {} evoked data set(s):", static_cast<int>(evokedSet.evoked.size()));
            for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
                Log::info("    [{}] {}", i, evokedSet.evoked[i].comment);
                setsToProcess.append(i);
            }
        } else {
            if (setNo < 0 || setNo >= std::ssize(evokedSet.evoked)) {
                Log::critical("Error: Set {} out of range (file has {} sets, 0-{})", setNo, static_cast<int>(evokedSet.evoked.size()), static_cast<int>(evokedSet.evoked.size()) - 1);
                return 1;
            }
            setsToProcess.append(setNo);
        }

        // Process each selected evoked set
        for (int setIdx : setsToProcess) {
            const FiffEvoked& evoked = evokedSet.evoked[setIdx];
            QString setComment = QString::fromStdString(evoked.comment);
            QString commentTag = sanitizeForFilename(setComment);

            Log::info("\n=== Evoked set {}: \"{}\" ===", setIdx, setComment.toStdString());

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
            QString setOutName;
            if (!outName.isEmpty()) {
                setOutName = outName + "-" + commentTag;
            } else {
                QFileInfo fi(inName);
                QString tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA")
                    tag = "sloreta";
                setOutName = fi.path() + "/" + fi.completeBaseName() + "-" + tag + "-" + commentTag;
            }

            int result = processInverseResults(inputData, tmin, tstep, curNave, setOutName);
            if (result != 0)
                return result;
        }
    }

    Log::info("\nFinished.");
    return 0;
}
