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
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    Implements mne_compute_raw_inverse application.
 *           Port of the original MNE-C mne_compute_raw_inverse by Matti Hamalainen.
 *
 *           Computes inverse solutions from raw or evoked FIFF data using
 *           an inverse operator and label-restricted source estimation.
 *           The output is written as STC files for each label.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_sourceestimate.h>
#include <mne/mne_sourcespace.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <fs/label.h>

#include <utils/generics/applicationlogger.h>

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
#include <QDebug>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace FSLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "1.0"
#define BIG_TIME 1e6f

//=============================================================================================================
// HELPER: detect whether a FIFF file is raw or evoked
//=============================================================================================================

static bool isRawFile(const QString &fileName)
{
    QFile file(fileName);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        return false;
    }

    FiffDirNode::SPtr dirTree = stream->dirtree();

    // Check for evoked data first — evoked files also use FIFFB_PROCESSED_DATA
    bool hasEvoked = dirTree->has_kind(FIFFB_EVOKED);
    if (hasEvoked) {
        stream->close();
        return false;
    }

    bool hasRaw = dirTree->has_kind(FIFFB_RAW_DATA) ||
                  dirTree->has_kind(FIFFB_CONTINUOUS_DATA);
    stream->close();
    return hasRaw;
}

//=============================================================================================================
// HELPER: sanitize a string for safe use as part of a filename
//=============================================================================================================

static QString sanitizeForFilename(const QString &s)
{
    QString result = s.simplified().toLower();
    result.replace(' ', '_');
    result.replace(QRegularExpression("[^a-z0-9_-]"), "");
    if (result.isEmpty()) result = "unknown";
    return result;
}

//=============================================================================================================
// HELPER: compose output file name from label file name
//=============================================================================================================

static QString composeOutName(const QString &labelName, const QString &tag, bool raw)
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

static int labelHemisphere(const QString &name)
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

static QStringList findLabelsInDir(const QString &dir)
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

static MNESourceEstimate processLabel(const QString &labelFile,
                                      const MNEInverseOperator &invOp,
                                      const MatrixXd &data,
                                      float tmin,
                                      float tstep,
                                      float lambda2,
                                      const QString &method,
                                      bool pickNormal)
{
    // Read the label
    Label label;
    if (!Label::read(labelFile, label)) {
        qWarning() << "Failed to read label file:" << labelFile;
        return MNESourceEstimate();
    }

    int hemi = labelHemisphere(labelFile);
    if (hemi < 0) {
        qWarning() << "Cannot determine hemisphere for label:" << labelFile;
        return MNESourceEstimate();
    }
    label.hemi = hemi;

    printf("  Label %s: %ld vertices (%s hemisphere)\n",
           labelFile.toUtf8().constData(),
           (long)label.vertices.size(),
           hemi == 0 ? "left" : "right");

    // Set up minimum norm with label restriction
    MinimumNorm minimumNorm(invOp, lambda2, method);
    minimumNorm.doInverseSetup(1, pickNormal);

    // Compute inverse for all data
    MNESourceEstimate stc = minimumNorm.calculateInverse(data, tmin, tstep, pickNormal);

    if (stc.isEmpty()) {
        qWarning() << "Inverse computation returned empty result for label:" << labelFile;
        return MNESourceEstimate();
    }

    // Restrict to label vertices
    VectorXi labelIndices = stc.getIndicesByLabel(QList<Label>() << label, false);

    if (labelIndices.size() == 0) {
        qWarning() << "No source vertices found in label:" << labelFile;
        return MNESourceEstimate();
    }

    // Extract label-restricted data
    MatrixXd labelData(labelIndices.size(), stc.data.cols());
    VectorXi labelVertices(labelIndices.size());
    for (int i = 0; i < labelIndices.size(); ++i) {
        labelData.row(i) = stc.data.row(labelIndices(i));
        labelVertices(i) = stc.vertices(labelIndices(i));
    }

    return MNESourceEstimate(labelData, labelVertices, stc.tmin, stc.tstep);
}

//=============================================================================================================
// Process label directory: compute average waveform per label
//=============================================================================================================

static MNESourceEstimate processLabelDir(const QString &labelDir,
                                         const MNEInverseOperator &invOp,
                                         const MatrixXd &data,
                                         float tmin,
                                         float tstep,
                                         float lambda2,
                                         const QString &method,
                                         QStringList &labelNames)
{
    QStringList labelFiles = findLabelsInDir(labelDir);
    if (labelFiles.isEmpty()) {
        qWarning() << "No label files found in directory:" << labelDir;
        return MNESourceEstimate();
    }

    printf("Found %lld label files in %s\n", (long long)labelFiles.size(), labelDir.toUtf8().constData());

    // Compute full inverse solution first (with pickNormal=true for labeldir)
    MinimumNorm minimumNorm(invOp, lambda2, method);
    minimumNorm.doInverseSetup(1, true);

    MNESourceEstimate fullStc = minimumNorm.calculateInverse(data, tmin, tstep, true);
    if (fullStc.isEmpty()) {
        qWarning() << "Full inverse computation returned empty result.";
        return MNESourceEstimate();
    }

    // For each label, compute the average source waveform
    MatrixXd avgData(labelFiles.size(), fullStc.data.cols());
    VectorXi avgVertices(labelFiles.size());
    avgData.setZero();

    int validLabels = 0;
    for (int i = 0; i < labelFiles.size(); ++i) {
        Label label;
        if (!Label::read(labelFiles[i], label)) {
            qWarning() << "  Skipping unreadable label:" << labelFiles[i];
            continue;
        }

        int hemi = labelHemisphere(labelFiles[i]);
        if (hemi < 0) {
            qWarning() << "  Skipping label with unknown hemisphere:" << labelFiles[i];
            continue;
        }
        label.hemi = hemi;

        // Find label vertices in the source estimate
        VectorXi labelIndices = fullStc.getIndicesByLabel(QList<Label>() << label, false);

        if (labelIndices.size() == 0) {
            qWarning() << "  No source vertices found in label:" << labelFiles[i];
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

        printf("  Label %s: %ld vertices, averaged\n",
               fi.fileName().toUtf8().constData(), (long)labelIndices.size());

        validLabels++;
    }

    if (validLabels == 0) {
        qWarning() << "No valid labels found.";
        return MNESourceEstimate();
    }

    // Trim to actual number of valid labels
    avgData.conservativeResize(validLabels, avgData.cols());
    avgVertices.conservativeResize(validLabels);

    return MNESourceEstimate(avgData, avgVertices, fullStc.tmin, fullStc.tstep);
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
int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
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
        "Produces source waveforms from labels. Output the results as STC files."
    );
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

    // --label: Label file(s) to process (can be specified multiple times)
    QCommandLineOption labelOpt(QStringList() << "label",
        "Label file to process (can have multiple).", "file");
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
        fprintf(stderr, "Error: --in option is required.\n\n");
        parser.showHelp(1);
    }
    if (!parser.isSet(invOpt)) {
        fprintf(stderr, "Error: --inv option is required.\n\n");
        parser.showHelp(1);
    }

    QString inName = parser.value(inOpt);
    QString invName = parser.value(invOpt);

    // Labels
    QStringList labelFiles = parser.values(labelOpt);
    QString labelDir = parser.value(labeldirOpt);
    QString outName = parser.value(outOpt);

    if (!labelDir.isEmpty() && outName.isEmpty()) {
        fprintf(stderr, "Error: --out option must be specified with --labeldir.\n\n");
        parser.showHelp(1);
    }

    // SNR
    float snr = parser.value(snrOpt).toFloat();
    if (snr <= 0.0f) {
        fprintf(stderr, "Error: SNR must be positive.\n");
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

    printf("\n");
    printf("mne_compute_raw_inverse v%s\n", PROGRAM_VERSION);
    printf("========================================\n");
    printf("Input file             : %s\n", inName.toUtf8().constData());
    if (doBaseline) {
        printf("Baseline               : %10.2f ... %10.2f ms\n", 1000.0f * bmin, 1000.0f * bmax);
    }
    printf("Inverse operator file  : %s\n", invName.toUtf8().constData());
    printf("SNR                    : %f\n", snr);
    printf("Method                 : %s\n", method.toUtf8().constData());
    if (pickNormal) {
        printf("Picking normal component to cortex\n");
    }

    if (!labelFiles.isEmpty()) {
        printf("Label files to process :\n");
        for (const QString &label : labelFiles) {
            printf("  %s\n", label.toUtf8().constData());
        }
    } else if (!labelDir.isEmpty()) {
        printf("Label directory        : %s\n", labelDir.toUtf8().constData());
    } else {
        printf("Full source space inverse (no label restriction)\n");
    }

    //=========================================================================================================
    // Read the inverse operator
    //=========================================================================================================

    printf("\nReading the inverse operator...\n");
    QFile invFile(invName);
    MNEInverseOperator inverseOperator(invFile);

    if (inverseOperator.eigen_leads->data.size() == 0) {
        fprintf(stderr, "Error: Failed to read inverse operator from %s\n", invName.toUtf8().constData());
        return 1;
    }
    printf("  Inverse operator read successfully.\n");
    printf("  %d channels, %d sources\n", inverseOperator.nchan, inverseOperator.nsource);

    // Pre-extract label list name for use in processing
    QString labelListName = parser.value(labelListOpt);

    //=========================================================================================================
    // Processing lambda: compute inverse and write results for one data set
    //=========================================================================================================

    auto processInverseResults = [&inverseOperator, &lambda2, &method, &pickNormal,
                                   &labelDir, &labelFiles, &labelListName, &inName]
                                  (const MatrixXd &inputData, float tmin, float tstep,
                                   int curNave, const QString &curOutName) -> int
    {
        if (!labelDir.isEmpty()) {
        //-----------------------------------------------------------------------------------------------------
        // Label directory mode: compute average waveform for each label
        //-----------------------------------------------------------------------------------------------------
        printf("\nProcessing label directory: %s\n", labelDir.toUtf8().constData());

        QStringList labelNames;
        MNESourceEstimate stc = processLabelDir(labelDir, inverseOperator,
                                                inputData, tmin, tstep,
                                                lambda2, method, labelNames);

        if (stc.isEmpty()) {
            fprintf(stderr, "Error: Label directory processing failed.\n");
            return 1;
        }

        // Write the STC result
        // Ensure output has proper extension
        QString stcOut = curOutName;
        if (!stcOut.endsWith(".stc") && !stcOut.endsWith(".fif")) {
            stcOut += "-lh.stc";
        }

        QFile stcFile(stcOut);
        if (!stc.write(stcFile)) {
            fprintf(stderr, "Error: Failed to write STC file: %s\n", stcOut.toUtf8().constData());
            return 1;
        }
        printf("\nWrote %s (%d labels, %d time points)\n",
               stcOut.toUtf8().constData(), (int)stc.data.rows(), (int)stc.data.cols());

        // Write label list if requested
        if (!labelListName.isEmpty() && !labelNames.isEmpty()) {
            QFile labelListFile(labelListName);
            if (labelListFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&labelListFile);
                for (const QString &name : labelNames) {
                    out << name << "\n";
                }
                labelListFile.close();
                printf("Label names output to %s\n", labelListName.toUtf8().constData());
            }
        }

    } else if (!labelFiles.isEmpty()) {
        //-----------------------------------------------------------------------------------------------------
        // Individual label mode: process each label file separately
        //-----------------------------------------------------------------------------------------------------
        for (const QString &labelFile : labelFiles) {
            printf("\nProcessing label: %s\n", labelFile.toUtf8().constData());

            MNESourceEstimate stc = processLabel(labelFile, inverseOperator,
                                                 inputData, tmin, tstep,
                                                 lambda2, method, pickNormal);

            if (stc.isEmpty()) {
                fprintf(stderr, "Warning: Skipping label %s (no result).\n", labelFile.toUtf8().constData());
                continue;
            }

            // Compose output file name
            QString tag = (method == "dSPM") ? "spm" : "mne";
            if (method == "sLORETA") tag = "sloreta";
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

            QFile stcFile(stcOut);
            if (!stc.write(stcFile)) {
                fprintf(stderr, "Error: Failed to write STC file: %s\n", stcOut.toUtf8().constData());
                continue;
            }
            printf("  Wrote %s (%d sources, %d time points)\n",
                   stcOut.toUtf8().constData(), (int)stc.data.rows(), (int)stc.data.cols());
        }

    } else {
        //-----------------------------------------------------------------------------------------------------
        // Full source space mode: compute inverse for all sources
        //-----------------------------------------------------------------------------------------------------
        printf("\nComputing full source space inverse...\n");

        MinimumNorm minimumNorm(inverseOperator, lambda2, method);
        minimumNorm.doInverseSetup(curNave, pickNormal);

        MNESourceEstimate stc = minimumNorm.calculateInverse(inputData, tmin, tstep, pickNormal);

        if (stc.isEmpty()) {
            fprintf(stderr, "Error: Full source space inverse computation failed.\n");
            return 1;
        }

        // Compose output base name
        QString stcOut;
        if (!curOutName.isEmpty()) {
            stcOut = curOutName;
            // Strip hemisphere suffix if user provided one
            if (stcOut.endsWith("-lh.stc")) stcOut.chop(7);
            else if (stcOut.endsWith("-rh.stc")) stcOut.chop(7);
        } else {
            QFileInfo fi(inName);
            QString tag = (method == "dSPM") ? "spm" : "mne";
            if (method == "sLORETA") tag = "sloreta";
            stcOut = fi.path() + "/" + fi.completeBaseName() + "-" + tag;
        }

        // Split the combined source estimate into lh and rh hemispheres.
        // The inverse operator's source space tells us how many sources
        // belong to each hemisphere.
        int nSrcLh = inverseOperator.src[0].nuse;
        int nSrcRh = inverseOperator.src[1].nuse;
        int nSrcTotal = (int)stc.data.rows();

        if (nSrcLh + nSrcRh != nSrcTotal) {
            fprintf(stderr, "Warning: Source count mismatch (lh=%d + rh=%d != %d). "
                    "Writing combined STC to lh file.\n", nSrcLh, nSrcRh, nSrcTotal);

            QString stcOutLh = stcOut + "-lh.stc";
            QFile stcFileLh(stcOutLh);
            if (!stc.write(stcFileLh)) {
                fprintf(stderr, "Error: Failed to write STC file: %s\n", stcOutLh.toUtf8().constData());
                return 1;
            }
            printf("  Wrote %s (%d sources, %d time points)\n",
                   stcOutLh.toUtf8().constData(), nSrcTotal, (int)stc.data.cols());
        } else {
            // Write left hemisphere STC
            if (nSrcLh > 0) {
                MNESourceEstimate stcLh(stc.data.topRows(nSrcLh),
                                        stc.vertices.head(nSrcLh),
                                        stc.tmin, stc.tstep);

                QString stcOutLh = stcOut + "-lh.stc";
                QFile stcFileLh(stcOutLh);
                if (!stcLh.write(stcFileLh)) {
                    fprintf(stderr, "Error: Failed to write LH STC file: %s\n", stcOutLh.toUtf8().constData());
                    return 1;
                }
                printf("  Wrote %s (%d sources, %d time points)\n",
                       stcOutLh.toUtf8().constData(), nSrcLh, (int)stc.data.cols());
            }

            // Write right hemisphere STC
            if (nSrcRh > 0) {
                MNESourceEstimate stcRh(stc.data.bottomRows(nSrcRh),
                                        stc.vertices.tail(nSrcRh),
                                        stc.tmin, stc.tstep);

                QString stcOutRh = stcOut + "-rh.stc";
                QFile stcFileRh(stcOutRh);
                if (!stcRh.write(stcFileRh)) {
                    fprintf(stderr, "Error: Failed to write RH STC file: %s\n", stcOutRh.toUtf8().constData());
                    return 1;
                }
                printf("  Wrote %s (%d sources, %d time points)\n",
                       stcOutRh.toUtf8().constData(), nSrcRh, (int)stc.data.cols());
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
        printf("\nReading raw data file: %s\n", inName.toUtf8().constData());

        if (nave <= 0) nave = 1;
        printf("  nave = %d\n", nave);

        QFile rawFile(inName);
        FiffRawData raw(rawFile);

        // Pick channels that match the inverse operator
        QStringList invChNames;
        for (int i = 0; i < inverseOperator.noise_cov->names.size(); ++i) {
            invChNames << inverseOperator.noise_cov->names[i];
        }

        RowVectorXi picks = raw.info.pick_channels(invChNames);
        if (picks.size() == 0) {
            fprintf(stderr, "Error: No matching channels found between raw data and inverse operator.\n");
            return 1;
        }

        printf("  Picked %d channels from raw data\n", (int)picks.size());

        // Activate projectors
        for (int k = 0; k < raw.info.projs.size(); ++k) {
            raw.info.projs[k].active = true;
        }

        // Read the full raw segment
        MatrixXd inputData;
        MatrixXd times;
        if (!raw.read_raw_segment(inputData, times, raw.first_samp, raw.last_samp, picks)) {
            fprintf(stderr, "Error: Failed to read raw data segment.\n");
            return 1;
        }

        float tstep = 1.0f / raw.info.sfreq;
        float tmin = static_cast<float>(raw.first_samp) * tstep;

        printf("  Read %d samples (%d channels)\n",
               (int)inputData.cols(), (int)inputData.rows());

        int result = processInverseResults(inputData, tmin, tstep, nave, outName);
        if (result != 0) return result;

    } else {
        //-----------------------------------------------------------------------------------------------------
        // Read all evoked data sets
        //-----------------------------------------------------------------------------------------------------
        printf("\nReading evoked data file: %s\n", inName.toUtf8().constData());

        QFile evokedFile(inName);
        QPair<float, float> baseline;
        if (doBaseline) {
            baseline = QPair<float, float>(bmin, bmax);
        } else {
            baseline = QPair<float, float>(-1.0f, -1.0f);
        }

        FiffEvokedSet evokedSet;
        if (!FiffEvokedSet::read(evokedFile, evokedSet, baseline)) {
            fprintf(stderr, "Error: Failed to read evoked data from %s\n", inName.toUtf8().constData());
            return 1;
        }

        if (evokedSet.evoked.isEmpty()) {
            fprintf(stderr, "Error: No evoked data sets found in %s\n", inName.toUtf8().constData());
            return 1;
        }

        // Determine which sets to process
        QList<int> setsToProcess;
        if (processAllSets) {
            printf("  Found %d evoked data set(s):\n", (int)evokedSet.evoked.size());
            for (int i = 0; i < evokedSet.evoked.size(); ++i) {
                printf("    [%d] %s\n", i, evokedSet.evoked[i].comment.toUtf8().constData());
                setsToProcess.append(i);
            }
        } else {
            if (setNo < 0 || setNo >= evokedSet.evoked.size()) {
                fprintf(stderr, "Error: Set %d out of range (file has %d sets, 0-%d)\n",
                        setNo, (int)evokedSet.evoked.size(), (int)evokedSet.evoked.size() - 1);
                return 1;
            }
            setsToProcess.append(setNo);
        }

        // Process each selected evoked set
        for (int setIdx : setsToProcess) {
            const FiffEvoked &evoked = evokedSet.evoked[setIdx];
            QString setComment = evoked.comment;
            QString commentTag = sanitizeForFilename(setComment);

            printf("\n=== Evoked set %d: \"%s\" ===\n", setIdx, setComment.toUtf8().constData());

            int curNave = (nave > 0) ? nave : evoked.nave;
            printf("  nave = %d\n", curNave);

            // Pick channels matching inverse operator
            FiffEvoked pickedEvoked = evoked.pick_channels(inverseOperator.noise_cov->names);
            printf("  Picked %d channels from evoked data\n", pickedEvoked.info.nchan);

            if (doBaseline) {
                printf("  Baseline correction: %10.2f ... %10.2f ms\n", 1000.0f * bmin, 1000.0f * bmax);
            } else {
                printf("  No baseline setting in effect.\n");
            }

            MatrixXd inputData = pickedEvoked.data;
            float tmin = pickedEvoked.times(0);
            float tstep = 1.0f / pickedEvoked.info.sfreq;

            printf("  Read %d time points (%d channels)\n",
                   (int)inputData.cols(), (int)inputData.rows());

            // Compose per-set output name with set description
            QString setOutName;
            if (!outName.isEmpty()) {
                setOutName = outName + "-" + commentTag;
            } else {
                QFileInfo fi(inName);
                QString tag = (method == "dSPM") ? "spm" : "mne";
                if (method == "sLORETA") tag = "sloreta";
                setOutName = fi.path() + "/" + fi.completeBaseName() + "-" + tag + "-" + commentTag;
            }

            int result = processInverseResults(inputData, tmin, tstep, curNave, setOutName);
            if (result != 0) return result;
        }
    }

    printf("\nFinished.\n");
    return 0;
}
