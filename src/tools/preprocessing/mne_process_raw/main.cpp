//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    mne_process_raw main entry point.
 *
 *           Batch processing of raw MEG/EEG data.
 *           Ported from mne_browse_raw/mne_process_raw (MNE-C) by Matti Hamalainen.
 *
 *           Features:
 *           - Filter and decimate raw data
 *           - Detect and save events from trigger channels
 *           - Compute averages from description files
 *           - Compute noise covariance matrices from description files
 *           - Create SSP (Signal-Space Projection) operators
 *           - Grand averaging across multiple files
 *
 *           Compatible with MNE-C command-line options.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "batchprocessor.h"

#include <mne/mne_process_description.h>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEPROCESSRAWAPP;
using namespace MNELIB;

//=============================================================================================================
// VERSION
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Prints a richly formatted, grouped help text to stdout.
 */
static void printHelp()
{
    QTextStream out(stdout);

    out << "\n"
        << "Usage: mne_process_raw [options]\n"
        << "\n"
        << "  Process raw MEG/EEG data — filtering, averaging, covariance\n"
        << "  computation, event detection, SSP operator creation, and decimation.\n"
        << "\n"
        << "  Ported from MNE-C mne_process_raw by Matti Hamalainen.\n"
        << "\n"
        << "General:\n"
        << "  --help                    Print this help and exit.\n"
        << "  --version                 Print version information and exit.\n"
        << "  --cd <dir>                Change working directory before processing.\n"
        << "\n"
        << "Input:\n"
        << "  --raw <file>              Raw data file to process.  May be given\n"
        << "                            multiple times for grand averaging.\n"
        << "  --grad <number>           Desired software gradient compensation order.\n"
        << "  --allowmaxshield          Allow loading of unprocessed MaxShield data.\n"
        << "\n"
        << "Events:\n"
        << "  --events <file>           Load event list from file.\n"
        << "  --eventsout <file>        Save detected events to file.\n"
        << "  --allevents               Detect all trigger transitions, not just\n"
        << "                            rising edges.\n"
        << "  --digtrig <name>          Trigger channel name (default: 'STI 014').\n"
        << "                            Use underscores for spaces (e.g. STI_014).\n"
        << "  --digtrigmask <value>     Trigger channel bit mask (decimal or 0xHex).\n"
        << "\n"
        << "Projection (SSP):\n"
        << "  --proj <file>             Load SSP vectors from file (repeatable).\n"
        << "  --projon                  Activate SSP when processing.\n"
        << "  --projoff                 Deactivate SSP when processing.\n"
        << "  --makeproj                Compute new SSP operators from the data.\n"
        << "  --projevent <code>        Event code for projector computation.\n"
        << "  --projtmin <sec>          Start time for projector window.\n"
        << "  --projtmax <sec>          End time for projector window.\n"
        << "  --projngrad <n>           Gradiometer SSP components  (default: 2).\n"
        << "  --projnmag <n>            Magnetometer SSP components (default: 2).\n"
        << "  --projneeg <n>            EEG SSP components          (default: 0).\n"
        << "  --projgradrej <fT/cm>     Grad rejection for SSP      (default: 2000).\n"
        << "  --projmagrej <fT>         Mag  rejection for SSP      (default: 3000).\n"
        << "  --projeegrej <uV>         EEG  rejection for SSP      (default: 0).\n"
        << "  --saveprojtag <tag>       Tag appended to SSP output filenames.\n"
        << "\n"
        << "Filtering:\n"
        << "  --filtersize <n>          FFT length for filtering  (default: 8192).\n"
        << "  --highpass <Hz>           High-pass corner frequency.\n"
        << "  --lowpass <Hz>            Low-pass  corner frequency.\n"
        << "  --highpassw <Hz>          High-pass transition bandwidth.\n"
        << "  --lowpassw <Hz>           Low-pass  transition bandwidth.\n"
        << "  --filteroff               Do not filter the data.\n"
        << "\n"
        << "Output / Decimation:\n"
        << "  --save <file>             Save processed raw data to file.\n"
        << "  --decim <factor>          Decimation factor (default: 1).\n"
        << "  --split <MB>              Split output at this file size (MB).\n"
        << "  --anon                    Omit subject information from output.\n"
        << "  --savehere                Write output to current dir instead of\n"
        << "                            the raw file's directory.\n"
        << "\n"
        << "Averaging:\n"
        << "  --ave <file>              Average description file (repeatable).\n"
        << "  --saveavetag <tag>        Tag appended to average output filenames.\n"
        << "  --gave <file>             Grand average output file.\n"
        << "\n"
        << "Covariance:\n"
        << "  --cov <file>              Covariance description file (repeatable).\n"
        << "  --savecovtag <tag>        Tag appended to covariance output filenames.\n"
        << "  --gcov <file>             Grand covariance output file.\n"
        << "\n"
        << "Examples:\n"
        << "\n"
        << "  Detect events and save them:\n"
        << "    mne_process_raw --raw data.fif --eventsout events.fif\n"
        << "\n"
        << "  Band-pass filter and save:\n"
        << "    mne_process_raw --raw data.fif --highpass 1 --lowpass 40 \\\n"
        << "                    --save filtered.fif\n"
        << "\n"
        << "  Compute averages from a description file:\n"
        << "    mne_process_raw --raw data.fif --ave auditory.ave\n"
        << "\n"
        << "  Compute noise covariance:\n"
        << "    mne_process_raw --raw data.fif --cov noise.cov\n"
        << "\n"
        << "  Grand average across subjects:\n"
        << "    mne_process_raw --raw subj1.fif --raw subj2.fif \\\n"
        << "                    --ave paradigm.ave --gave grand_avg.fif\n"
        << "\n"
        << "  Decimate and save with projection:\n"
        << "    mne_process_raw --raw data.fif --proj ssp.fif --projon \\\n"
        << "                    --decim 4 --save decimated.fif\n"
        << "\n"
        << "  Create SSP operators from empty-room data:\n"
        << "    mne_process_raw --raw empty_room.fif --makeproj \\\n"
        << "                    --projngrad 3 --projnmag 3\n"
        << "\n";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The main entry point of mne_process_raw.
 *
 * @param[in] argc  Number of command-line arguments.
 * @param[in] argv  Array of command-line argument strings.
 * @return Application exit code.
 */
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_process_raw");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================
    // Set up command-line parser
    //=========================================================================
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Process raw MEG/EEG data.\n\n"
        "Ported from MNE-C mne_process_raw by Matti Hamalainen.\n"
        "Supports filtering, averaging, covariance computation, event detection,\n"
        "SSP operator creation, and decimation of raw data files.");
    parser.addVersionOption();

    // Custom help option (replaces the default one with grouped output)
    QCommandLineOption helpOpt(QStringList() << "h" << "help", "Print this help and exit.");
    parser.addOption(helpOpt);

    // --- Working directory ---
    QCommandLineOption cdOpt("cd", "Change working directory.", "dir");
    parser.addOption(cdOpt);

    // --- Raw data files ---
    QCommandLineOption rawOpt("raw", "Input raw data file (may be specified multiple times).", "file");
    parser.addOption(rawOpt);

    // --- Gradient compensation ---
    QCommandLineOption gradOpt("grad", "Desired software gradient compensation order.", "number");
    parser.addOption(gradOpt);

    // --- MaxShield ---
    QCommandLineOption maxshieldOpt("allowmaxshield", "Allow unprocessed MaxShield data.");
    parser.addOption(maxshieldOpt);

    // --- Event I/O ---
    QCommandLineOption eventsOpt("events", "Load events from this file.", "file");
    parser.addOption(eventsOpt);

    QCommandLineOption eventsOutOpt("eventsout", "Save detected events to this file.", "file");
    parser.addOption(eventsOutOpt);

    QCommandLineOption allEventsOpt("allevents", "Save all trigger line transitions (not just rising edges).");
    parser.addOption(allEventsOpt);

    // --- Projection ---
    QCommandLineOption projOpt("proj", "Load SSP projection from this file (may be specified multiple times).", "file");
    parser.addOption(projOpt);

    QCommandLineOption projOnOpt("projon", "Apply SSP to the data.");
    parser.addOption(projOnOpt);

    QCommandLineOption projOffOpt("projoff", "Do not apply SSP to the data.");
    parser.addOption(projOffOpt);

    QCommandLineOption makeProjOpt("makeproj", "Create new SSP operators from the data.");
    parser.addOption(makeProjOpt);

    QCommandLineOption projEventOpt("projevent", "Event code for projector creation.", "number");
    parser.addOption(projEventOpt);

    QCommandLineOption projTminOpt("projtmin", "Start time for projector calculation (sec).", "time");
    parser.addOption(projTminOpt);

    QCommandLineOption projTmaxOpt("projtmax", "End time for projector calculation (sec).", "time");
    parser.addOption(projTmaxOpt);

    QCommandLineOption projNGradOpt("projngrad", "Number of gradiometer components for SSP (default=2).", "value", "2");
    parser.addOption(projNGradOpt);

    QCommandLineOption projNMagOpt("projnmag", "Number of magnetometer components for SSP (default=2).", "value", "2");
    parser.addOption(projNMagOpt);

    QCommandLineOption projNEegOpt("projneeg", "Number of EEG components for SSP (default=0).", "value", "0");
    parser.addOption(projNEegOpt);

    QCommandLineOption projGradRejOpt("projgradrej", "Grad rejection for SSP (fT/cm, default=2000).", "value", "2000");
    parser.addOption(projGradRejOpt);

    QCommandLineOption projMagRejOpt("projmagrej", "Mag rejection for SSP (fT, default=3000).", "value", "3000");
    parser.addOption(projMagRejOpt);

    QCommandLineOption projEegRejOpt("projeegrej", "EEG rejection for SSP (uV, default=0).", "value", "0");
    parser.addOption(projEegRejOpt);

    QCommandLineOption saveProjTagOpt("saveprojtag", "Tag for projection output files.", "tag");
    parser.addOption(saveProjTagOpt);

    // --- Filter ---
    QCommandLineOption filterSizeOpt("filtersize", "Filter length (default=8192).", "size", "8192");
    parser.addOption(filterSizeOpt);

    QCommandLineOption highpassOpt("highpass", "Highpass corner frequency (Hz).", "freq");
    parser.addOption(highpassOpt);

    QCommandLineOption lowpassOpt("lowpass", "Lowpass corner frequency (Hz).", "freq");
    parser.addOption(lowpassOpt);

    QCommandLineOption highpasswOpt("highpassw", "Highpass transition width (Hz).", "freq");
    parser.addOption(highpasswOpt);

    QCommandLineOption lowpasswOpt("lowpassw", "Lowpass transition width (Hz).", "freq");
    parser.addOption(lowpasswOpt);

    QCommandLineOption filterOffOpt("filteroff", "Do not filter the data.");
    parser.addOption(filterOffOpt);

    // --- Save/Decimate ---
    QCommandLineOption saveOpt("save", "Save filtered/decimated raw data to this file.", "file");
    parser.addOption(saveOpt);

    QCommandLineOption anonOpt("anon", "Omit subject info from output.");
    parser.addOption(anonOpt);

    QCommandLineOption decimOpt("decim", "Decimation factor for saving (default=1).", "factor", "1");
    parser.addOption(decimOpt);

    QCommandLineOption splitOpt("split", "Split output files at this size (MB).", "size");
    parser.addOption(splitOpt);

    // --- Averaging ---
    QCommandLineOption aveOpt("ave", "Average description file (may be specified multiple times).", "file");
    parser.addOption(aveOpt);

    QCommandLineOption saveAveTagOpt("saveavetag", "Tag for average output files.", "tag");
    parser.addOption(saveAveTagOpt);

    QCommandLineOption gaveOpt("gave", "Grand average output file.", "file");
    parser.addOption(gaveOpt);

    // --- Covariance ---
    QCommandLineOption covOpt("cov", "Covariance description file (may be specified multiple times).", "file");
    parser.addOption(covOpt);

    QCommandLineOption saveCovTagOpt("savecovtag", "Tag for covariance output files.", "tag");
    parser.addOption(saveCovTagOpt);

    QCommandLineOption gcovOpt("gcov", "Grand average covariance output file.", "file");
    parser.addOption(gcovOpt);

    // --- Save location ---
    QCommandLineOption saveHereOpt("savehere", "Save output files in current directory instead of raw data directory.");
    parser.addOption(saveHereOpt);

    // --- Trigger ---
    QCommandLineOption digTrigOpt("digtrig", "Digital trigger channel name (default='STI 014').", "name", "STI 014");
    parser.addOption(digTrigOpt);

    QCommandLineOption digTrigMaskOpt("digtrigmask", "Mask for digital trigger channel (decimal or 0x hex).", "value");
    parser.addOption(digTrigMaskOpt);

    //=========================================================================
    // Parse command-line arguments
    //=========================================================================
    parser.parse(app.arguments());

    if (parser.isSet(helpOpt)) {
        printHelp();
        return 0;
    }
    if (parser.isSet("version")) {
        QTextStream(stdout) << QCoreApplication::applicationName()
                            << " " << QCoreApplication::applicationVersion() << "\n";
        return 0;
    }

    // Report unknown options
    if (!parser.unknownOptionNames().isEmpty()) {
        for (const QString &opt : parser.unknownOptionNames())
            qCritical() << "Unknown option:" << opt;
        printHelp();
        return 1;
    }

    //=========================================================================
    // Change working directory if requested
    //=========================================================================
    if (parser.isSet(cdOpt)) {
        QDir::setCurrent(parser.value(cdOpt));
        qInfo() << "Working directory changed to:" << QDir::currentPath();
    }

    //=========================================================================
    // Populate ProcessingSettings
    //=========================================================================
    ProcessingSettings settings;

    // Raw files
    settings.rawFiles = parser.values(rawOpt);

    // Event files
    settings.eventFiles    = parser.values(eventsOpt);
    settings.eventsOutFiles = parser.values(eventsOutOpt);

    // Trigger settings
    settings.digTrigger = parser.value(digTrigOpt);
    // Replace underscores with spaces for MNE-C compatibility
    settings.digTrigger.replace('_', ' ');

    if (parser.isSet(digTrigMaskOpt)) {
        QString maskStr = parser.value(digTrigMaskOpt);
        bool ok;
        if (maskStr.startsWith("0x", Qt::CaseInsensitive))
            settings.digTriggerMask = maskStr.toUInt(&ok, 16);
        else
            settings.digTriggerMask = maskStr.toUInt(&ok, 10);
    }

    // Projection settings
    settings.projFiles = parser.values(projOpt);
    if (parser.isSet(projOnOpt))
        settings.projOn = 1;
    else if (parser.isSet(projOffOpt))
        settings.projOn = 0;

    settings.makeProj    = parser.isSet(makeProjOpt);
    settings.projEvent   = parser.value(projEventOpt).toInt();
    settings.projTmin    = parser.value(projTminOpt).toFloat();
    settings.projTmax    = parser.value(projTmaxOpt).toFloat();
    settings.projNGrad   = parser.value(projNGradOpt).toInt();
    settings.projNMag    = parser.value(projNMagOpt).toInt();
    settings.projNEeg    = parser.value(projNEegOpt).toInt();
    settings.projGradReject = parser.value(projGradRejOpt).toFloat() * 1e-13f;
    settings.projMagReject  = parser.value(projMagRejOpt).toFloat() * 1e-15f;
    settings.projEegReject  = parser.value(projEegRejOpt).toFloat() * 1e-6f;
    settings.saveProjTag = parser.value(saveProjTagOpt);
    if (!settings.saveProjTag.isEmpty())
        settings.makeProj = true;

    // Filter settings
    settings.filter.filterSize = parser.value(filterSizeOpt).toInt();
    if (parser.isSet(highpassOpt))
        settings.filter.highpass = parser.value(highpassOpt).toFloat();
    if (parser.isSet(lowpassOpt))
        settings.filter.lowpass = parser.value(lowpassOpt).toFloat();
    if (parser.isSet(highpasswOpt))
        settings.filter.highpassWidth = parser.value(highpasswOpt).toFloat();
    if (parser.isSet(lowpasswOpt))
        settings.filter.lowpassWidth = parser.value(lowpasswOpt).toFloat();
    settings.filter.filterOn = !parser.isSet(filterOffOpt);

    // Save settings
    settings.saveFiles = parser.values(saveOpt);
    settings.decimation = parser.value(decimOpt).toInt();
    if (parser.isSet(splitOpt))
        settings.splitSize = static_cast<qint64>(parser.value(splitOpt).toFloat() * 1024 * 1024);

    // Averaging settings
    settings.aveFiles   = parser.values(aveOpt);
    settings.saveAveTag = parser.value(saveAveTagOpt);
    settings.grandAveFile = parser.value(gaveOpt);

    // Covariance settings
    settings.covFiles   = parser.values(covOpt);
    settings.saveCovTag = parser.value(saveCovTagOpt);
    settings.grandCovFile = parser.value(gcovOpt);

    // Other settings
    settings.saveHere = parser.isSet(saveHereOpt);

    //=========================================================================
    // Validate required options
    //=========================================================================
    if (settings.rawFiles.isEmpty()) {
        qCritical() << "Error: At least one --raw file must be specified.";
        printHelp();
        return 1;
    }

    //=========================================================================
    // Run batch processing
    //=========================================================================
    return BatchProcessor::run(settings);
}
