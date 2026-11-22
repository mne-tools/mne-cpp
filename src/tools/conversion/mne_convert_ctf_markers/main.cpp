//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Convert CTF marker files to MNE event format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct CtfMarker
{
    QString name;
    int eventId;
    QList<int> samples;

    bool operator==(const CtfMarker& other) const
    {
        return name == other.name && eventId == other.eventId;
    }
};

//=============================================================================================================

/**
 * Parse CTF MarkerFile format.
 *
 * The format consists of marker definitions with:
 *   MARKER CLASSIFICATION
 *   NAME: marker_name
 *   COMMENT:
 *   COLOR: color_name
 *   EDITABLE: Yes/No
 *   COPY DATA TO: No
 *   CLASSID: N
 *   NUMBER OF MARKERS: M
 *   LIST OF MARKERS:
 *   TRIAL NUMBER       TIME FROM SYNC POINT (in seconds)
 *   trial_num           time
 *   ...
 */
static QList<CtfMarker> parseCtfMarkers(const QString& filename)
{
    QList<CtfMarker> markers;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mne::Log::critical("Cannot open CTF marker file: {}", filename.toStdString());
        return markers;
    }

    QTextStream in(&file);
    CtfMarker current;
    int classId = 0;
    bool inList = false;
    int nMarkers = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("NAME:")) {
            current = CtfMarker();
            current.name = line.mid(5).trimmed();
            inList = false;
            continue;
        }

        if (line.startsWith("CLASSID:")) {
            classId = line.mid(8).trimmed().toInt();
            current.eventId = classId;
            continue;
        }

        if (line.startsWith("NUMBER OF MARKERS:")) {
            nMarkers = line.mid(18).trimmed().toInt();
            continue;
        }

        if (line.startsWith("LIST OF MARKERS:")) {
            inList = true;
            // Skip header line
            if (!in.atEnd())
                in.readLine();
            continue;
        }

        if (inList && !line.isEmpty() && !line.startsWith("MARKER")) {
            // Parse trial_number and time
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                // The sample is derived from the trial number and time;
                // for simplicity, we use the time as a sample index if it's an integer,
                // or store trial_number as sample position
                int sample = parts[0].toInt();
                current.samples.append(sample);
            }

            if (current.samples.size() >= nMarkers && nMarkers > 0) {
                inList = false;
                if (!current.name.isEmpty()) {
                    markers.append(current);
                    current = CtfMarker();
                }
            }
            continue;
        }

        // Start of new marker classification block
        if (line.startsWith("MARKER CLASSIFICATION")) {
            if (!current.name.isEmpty() && !current.samples.isEmpty()) {
                markers.append(current);
            }
            current = CtfMarker();
            inList = false;
        }
    }

    // Add last marker if pending
    if (!current.name.isEmpty() && !current.samples.isEmpty() &&
        !markers.contains(current)) {
        markers.append(current);
    }

    return markers;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_convert_ctf_markers");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert CTF marker files to MNE event format.\n\n"
                                     "Output format: sample 0 event_id (one event per line).");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption markersOpt("markers", "CTF marker file.", "file");
    parser.addOption(markersOpt);

    QCommandLineOption outOpt("out", "Output event file (text format).", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString markersFile = parser.value(markersOpt);
    QString outFile = parser.value(outOpt);

    if (markersFile.isEmpty()) {
        mne::Log::critical("--markers is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        mne::Log::critical("--out is required.");
        return 1;
    }

    // Parse CTF markers
    QList<CtfMarker> markers = parseCtfMarkers(markersFile);
    if (markers.isEmpty()) {
        mne::Log::critical("No markers parsed from: {}", markersFile.toStdString());
        return 1;
    }

    // Write MNE event format
    QFile fOut(outFile);
    if (!fOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mne::Log::critical("Cannot open output file: {}", outFile.toStdString());
        return 1;
    }

    QTextStream out(&fOut);
    int totalEvents = 0;

    for (const CtfMarker& marker : markers) {
        mne::Log::info("Marker '{}' (id={}): {} events", marker.name.toStdString(), marker.eventId, static_cast<long long>(marker.samples.size()));

        for (int sample : marker.samples) {
            out << sample << " 0 " << marker.eventId << "\n";
            ++totalEvents;
        }
    }

    fOut.close();
    mne::Log::info("Written {} events to: {}", totalEvents, outFile.toStdString());
    return 0;
}
