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
 * @brief    CLI event annotation tool for FIFF measurement files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct EventEntry {
    int sample;
    int before;
    int after;
    QString comment;
};

//=============================================================================================================

static QList<EventEntry> readEventFile(const QString& fileName)
{
    QList<EventEntry> events;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return events;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 3) continue;

        EventEntry ev;
        ev.sample = parts[0].toInt();
        ev.before = parts[1].toInt();
        ev.after = parts[2].toInt();
        ev.comment = (parts.size() > 3) ? parts.mid(3).join(" ") : QString();
        events.append(ev);
    }
    file.close();
    return events;
}

//=============================================================================================================

static bool writeEventFile(const QString& fileName, const QList<EventEntry>& events)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot open output file: %s", qPrintable(fileName));
        return false;
    }
    QTextStream out(&file);
    out << "# MNE event file (mne_dacq_annotator)\n";
    out << "# sample  before  after  [comment]\n";
    for (const EventEntry& ev : events) {
        out << ev.sample << "\t" << ev.before << "\t" << ev.after;
        if (!ev.comment.isEmpty()) {
            out << "\t" << ev.comment;
        }
        out << "\n";
    }
    file.close();
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_dacq_annotator");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "CLI event annotation tool for FIFF measurement files.\n\n"
        "Create, list, and edit event annotations associated with MEG/EEG\n"
        "recordings. Events are stored in the standard MNE event file format\n"
        "(three-column: sample, before, after) with optional comment fields.\n\n"
        "The original MNE-C mne_dacq_annotator was a Motif/X11 GUI for\n"
        "real-time annotation during MEG acquisition. This CLI port provides\n"
        "the same annotation management as a scriptable command-line tool.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Input FIFF measurement file.", "file");
    QCommandLineOption eventsOpt("events", "Event file to read/edit (default: <meas>-eve.fif).", "file");
    QCommandLineOption addOpt("add", "Add event: sample:before:after[:comment].", "spec");
    QCommandLineOption removeOpt("remove", "Remove event at sample number.", "sample");
    QCommandLineOption listOpt("list", "List all events.");
    QCommandLineOption outOpt("out", "Output event file (default: overwrite --events).", "file");
    QCommandLineOption stim_chOpt("stim-ch", "Extract events from stimulus channel.", "name");
    QCommandLineOption threshOpt("thresh", "Threshold for stimulus channel extraction (default: 1.0).", "value", "1.0");

    parser.addOption(measOpt);
    parser.addOption(eventsOpt);
    parser.addOption(addOpt);
    parser.addOption(removeOpt);
    parser.addOption(listOpt);
    parser.addOption(outOpt);
    parser.addOption(stim_chOpt);
    parser.addOption(threshOpt);

    parser.process(app);

    QString measFile = parser.value(measOpt);
    QString eventsFile = parser.value(eventsOpt);
    QString outFile = parser.value(outOpt);
    bool doList = parser.isSet(listOpt);

    //=========================================================================
    // Read measurement info
    //=========================================================================
    float sfreq = 0.0f;

    if (!measFile.isEmpty()) {
        QFile fMeas(measFile);
        FiffRawData rawData(fMeas);
        if (rawData.info.isEmpty()) {
            qCritical("Cannot read measurement info from: %s", qPrintable(measFile));
            return 1;
        }
        sfreq = rawData.info.sfreq;
        printf("Measurement: %d channels, sfreq = %.1f Hz\n", rawData.info.nchan, sfreq);

        // Default events file
        if (eventsFile.isEmpty()) {
            QFileInfo fi(measFile);
            eventsFile = fi.path() + "/" + fi.completeBaseName() + "-eve.fif";
        }
    }

    //=========================================================================
    // Extract events from stimulus channel
    //=========================================================================
    if (parser.isSet(stim_chOpt)) {
        if (measFile.isEmpty()) {
            qCritical("--meas is required with --stim-ch.");
            return 1;
        }

        QString stimName = parser.value(stim_chOpt);
        double thresh = parser.value(threshOpt).toDouble();

        QFile fMeas(measFile);
        FiffRawData raw(fMeas);

        // Find stimulus channel
        int stimIdx = -1;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].ch_name == stimName) {
                stimIdx = i;
                break;
            }
        }
        if (stimIdx < 0) {
            qCritical("Stimulus channel '%s' not found.", qPrintable(stimName));
            return 1;
        }

        // Read all data for stimulus channel
        MatrixXd data;
        MatrixXd times;
        RowVectorXi sel(1);
        sel(0) = stimIdx;

        if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp, sel)) {
            qCritical("Cannot read stimulus channel data.");
            return 1;
        }

        // Find threshold crossings (rising edges)
        QList<EventEntry> events;
        for (int i = 1; i < data.cols(); ++i) {
            double prev = data(0, i - 1);
            double curr = data(0, i);
            if (prev < thresh && curr >= thresh) {
                int eventId = static_cast<int>(std::round(curr));
                EventEntry ev;
                ev.sample = raw.first_samp + i;
                ev.before = 0;
                ev.after = eventId;
                events.append(ev);
            }
        }

        printf("Found %lld events from stimulus channel '%s'\n",
               static_cast<long long>(events.size()), qPrintable(stimName));

        if (outFile.isEmpty()) outFile = eventsFile;
        if (outFile.isEmpty()) {
            qCritical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events)) return 1;
        printf("Written events to: %s\n", qPrintable(outFile));
        return 0;
    }

    //=========================================================================
    // Load existing events
    //=========================================================================
    QList<EventEntry> events;
    if (!eventsFile.isEmpty() && QFile::exists(eventsFile)) {
        events = readEventFile(eventsFile);
        printf("Loaded %lld existing events from: %s\n", static_cast<long long>(events.size()), qPrintable(eventsFile));
    }

    //=========================================================================
    // Add events
    //=========================================================================
    if (parser.isSet(addOpt)) {
        QString spec = parser.value(addOpt);
        QStringList parts = spec.split(':');
        if (parts.size() < 3) {
            qCritical("Invalid event spec '%s'. Use sample:before:after[:comment].", qPrintable(spec));
            return 1;
        }

        EventEntry ev;
        ev.sample = parts[0].toInt();
        ev.before = parts[1].toInt();
        ev.after = parts[2].toInt();
        ev.comment = (parts.size() > 3) ? parts.mid(3).join(":") : QString();

        events.append(ev);
        printf("Added event: sample=%d, before=%d, after=%d%s\n",
               ev.sample, ev.before, ev.after,
               ev.comment.isEmpty() ? "" : qPrintable(QString(", comment=%1").arg(ev.comment)));

        // Sort by sample
        std::sort(events.begin(), events.end(),
                  [](const EventEntry& a, const EventEntry& b) { return a.sample < b.sample; });
    }

    //=========================================================================
    // Remove events
    //=========================================================================
    if (parser.isSet(removeOpt)) {
        int removeSample = parser.value(removeOpt).toInt();
        int removed = 0;
        for (int i = events.size() - 1; i >= 0; --i) {
            if (events[i].sample == removeSample) {
                events.removeAt(i);
                removed++;
            }
        }
        printf("Removed %d event(s) at sample %d\n", removed, removeSample);
    }

    //=========================================================================
    // List events
    //=========================================================================
    if (doList) {
        printf("\n%-10s  %-8s  %-8s  %s\n", "Sample", "Before", "After", "Comment");
        printf("%-10s  %-8s  %-8s  %s\n", "------", "------", "-----", "-------");
        for (const EventEntry& ev : events) {
            printf("%-10d  %-8d  %-8d", ev.sample, ev.before, ev.after);
            if (sfreq > 0) {
                printf("  (t=%.3f s)", ev.sample / static_cast<double>(sfreq));
            }
            if (!ev.comment.isEmpty()) {
                printf("  %s", qPrintable(ev.comment));
            }
            printf("\n");
        }
        printf("\nTotal: %lld event(s)\n", static_cast<long long>(events.size()));
    }

    //=========================================================================
    // Write output
    //=========================================================================
    if (parser.isSet(addOpt) || parser.isSet(removeOpt)) {
        if (outFile.isEmpty()) outFile = eventsFile;
        if (outFile.isEmpty()) {
            qCritical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events)) return 1;
        printf("Written %lld events to: %s\n", static_cast<long long>(events.size()), qPrintable(outFile));
    }

    return 0;
}
