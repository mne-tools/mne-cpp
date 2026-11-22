//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    CLI event annotation tool for FIFF measurement files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

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

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct EventEntry
{
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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return events;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 3)
            continue;

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
        Log::critical("Cannot open output file: {}", fileName.toStdString());
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

int main(int argc, char* argv[])
{
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
        FiffRawData rawData(measFile.toStdU16String());
        if (rawData.info.empty()) {
            Log::critical("Cannot read measurement info from: {}", measFile.toStdString());
            return 1;
        }
        sfreq = rawData.info.sfreq;
        Log::info("Measurement: {} channels, sfreq = {:.1f} Hz", rawData.info.nchan, sfreq);

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
            Log::critical("--meas is required with --stim-ch.");
            return 1;
        }

        QString stimName = parser.value(stim_chOpt);
        double thresh = parser.value(threshOpt).toDouble();

        FiffRawData raw(measFile.toStdU16String());

        // Find stimulus channel
        int stimIdx = -1;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].ch_name == stimName) {
                stimIdx = i;
                break;
            }
        }
        if (stimIdx < 0) {
            Log::critical("Stimulus channel '{}' not found.", stimName.toStdString());
            return 1;
        }

        // Read all data for stimulus channel
        MatrixXd data;
        MatrixXd times;
        RowVectorXi sel(1);
        sel(0) = stimIdx;

        if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp, sel)) {
            Log::critical("Cannot read stimulus channel data.");
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

        Log::info("Found {} events from stimulus channel '{}'", static_cast<long long>(events.size()), stimName.toStdString());

        if (outFile.isEmpty())
            outFile = eventsFile;
        if (outFile.isEmpty()) {
            Log::critical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events))
            return 1;
        Log::info("Written events to: {}", outFile.toStdString());
        return 0;
    }

    //=========================================================================
    // Load existing events
    //=========================================================================
    QList<EventEntry> events;
    if (!eventsFile.isEmpty() && QFile::exists(eventsFile)) {
        events = readEventFile(eventsFile);
        Log::info("Loaded {} existing events from: {}", static_cast<long long>(events.size()), eventsFile.toStdString());
    }

    //=========================================================================
    // Add events
    //=========================================================================
    if (parser.isSet(addOpt)) {
        QString spec = parser.value(addOpt);
        QStringList parts = spec.split(':');
        if (parts.size() < 3) {
            Log::critical("Invalid event spec '{}'. Use sample:before:after[:comment].", spec.toStdString());
            return 1;
        }

        EventEntry ev;
        ev.sample = parts[0].toInt();
        ev.before = parts[1].toInt();
        ev.after = parts[2].toInt();
        ev.comment = (parts.size() > 3) ? parts.mid(3).join(":") : QString();

        events.append(ev);
        Log::info("Added event: sample={}, before={}, after={}{}", ev.sample, ev.before, ev.after, ev.comment.isEmpty() ? "" : qPrintable(QString(", comment=%1").arg(ev.comment)));

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
        Log::info("Removed {} event(s) at sample {}", removed, removeSample);
    }

    //=========================================================================
    // List events
    //=========================================================================
    if (doList) {
        Log::info("\n{:<10}  {:<8}  {:<8}  {}", "Sample", "Before", "After", "Comment");
        Log::info("{:<10}  {:<8}  {:<8}  {}", "------", "------", "-----", "-------");
        for (const EventEntry& ev : events) {
            QString line = QString::asprintf("%-10d  %-8d  %-8d",
                                             ev.sample, ev.before, ev.after);
            if (sfreq > 0) {
                line += QString::asprintf("  (t=%.3f s)",
                                          ev.sample / static_cast<double>(sfreq));
            }
            if (!ev.comment.isEmpty()) {
                line += "  " + ev.comment;
            }
            Log::info("{}", line.toStdString());
        }
        Log::info("\nTotal: {} event(s)", static_cast<long long>(events.size()));
    }

    //=========================================================================
    // Write output
    //=========================================================================
    if (parser.isSet(addOpt) || parser.isSet(removeOpt)) {
        if (outFile.isEmpty())
            outFile = eventsFile;
        if (outFile.isEmpty()) {
            Log::critical("Specify --out for output event file.");
            return 1;
        }

        if (!writeEventFile(outFile, events))
            return 1;
        Log::info("Written {} events to: {}", static_cast<long long>(events.size()), outFile.toStdString());
    }

    return 0;
}
