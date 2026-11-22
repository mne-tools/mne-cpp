//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Export epochs from a raw FIFF file to MATLAB Level 5 MAT files.
 *
 *           Reads events from a text file, extracts epochs around matching events,
 *           and writes each epoch as a separate MAT file containing the data matrix,
 *           sampling frequency, and time vector.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>

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
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QRegularExpression>

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

// MAT-file Level 5 constants
static const quint32 miDOUBLE = 9;
static const quint32 miINT32 = 5;
static const quint32 miUINT32 = 6;
static const quint32 miINT8 = 1;
static const quint32 miMATRIX = 14;
static const quint32 mxDOUBLE_CLASS = 6;

//=============================================================================================================
// MAT-file writing helpers
//=============================================================================================================

static void writePad(QDataStream& ds, int numBytes)
{
    int pad = (8 - (numBytes % 8)) % 8;
    for (int i = 0; i < pad; ++i)
        ds << static_cast<quint8>(0);
}

static void writeMatTag(QDataStream& ds, quint32 dataType, quint32 numBytes)
{
    ds << dataType;
    ds << numBytes;
}

static void writeSmallTag(QDataStream& ds, quint32 dataType, quint32 numBytes, const QByteArray& data)
{
    if (numBytes <= 4) {
        quint32 packed = (numBytes << 16) | dataType;
        ds << packed;
        ds.writeRawData(data.constData(), numBytes);
        int pad = 4 - numBytes;
        for (int i = 0; i < pad; ++i)
            ds << static_cast<quint8>(0);
    } else {
        writeMatTag(ds, dataType, numBytes);
        ds.writeRawData(data.constData(), numBytes);
        writePad(ds, numBytes);
    }
}

static void writeMatrixVariable(QDataStream& ds, const QString& name, const MatrixXd& mat)
{
    int nRows = mat.rows();
    int nCols = mat.cols();
    QByteArray nameBytes = name.toLatin1();

    quint32 flagsSize = 8;
    quint32 dimsSize = 8;
    quint32 nameSize = nameBytes.size();
    quint32 namePadded = nameSize <= 4 ? 4 : nameSize + ((8 - (nameSize % 8)) % 8);
    quint32 nameTagSize = nameSize <= 4 ? 8 : 8 + namePadded;
    quint32 dataSize = nRows * nCols * 8;
    quint32 dataPadded = dataSize + ((8 - (dataSize % 8)) % 8);

    quint32 arrayFlagsSubSize = 8 + flagsSize;
    quint32 dimsSubSize = 8 + dimsSize;
    quint32 nameSubSize = nameTagSize;
    quint32 dataSubSize = 8 + dataPadded;

    quint32 totalPayload = arrayFlagsSubSize + dimsSubSize + nameSubSize + dataSubSize;

    writeMatTag(ds, miMATRIX, totalPayload);

    // Array Flags
    writeMatTag(ds, miUINT32, flagsSize);
    ds << static_cast<quint32>(mxDOUBLE_CLASS);
    ds << static_cast<quint32>(0);

    // Dimensions
    writeMatTag(ds, miINT32, dimsSize);
    ds << static_cast<qint32>(nRows);
    ds << static_cast<qint32>(nCols);

    // Name
    if (nameSize <= 4) {
        writeSmallTag(ds, miINT8, nameSize, nameBytes);
    } else {
        writeMatTag(ds, miINT8, nameSize);
        ds.writeRawData(nameBytes.constData(), nameSize);
        writePad(ds, nameSize);
    }

    // Data (column-major)
    writeMatTag(ds, miDOUBLE, dataSize);
    for (int c = 0; c < nCols; ++c) {
        for (int r = 0; r < nRows; ++r) {
            ds << mat(r, c);
        }
    }
    writePad(ds, dataSize);
}

static void writeMatHeader(QDataStream& ds, const QString& toolName)
{
    QByteArray headerText = QString("MATLAB 5.0 MAT-file, created by %1").arg(toolName).toLatin1();
    headerText.append(QByteArray(116 - headerText.size(), ' '));
    ds.writeRawData(headerText.constData(), 116);

    for (int i = 0; i < 8; ++i)
        ds << static_cast<quint8>(0);

    ds << static_cast<quint16>(0x0100);
    ds << static_cast<quint16>(0x4D49);
}

//=============================================================================================================

struct EventEntry
{
    int sample;
    int prev;
    int id;
};

static QList<EventEntry> readEvents(const QString& filename)
{
    QList<EventEntry> events;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open event file: {}", filename.toStdString());
        return events;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 3) {
            EventEntry ev;
            ev.sample = parts[0].toInt();
            ev.prev = parts[1].toInt();
            ev.id = parts[2].toInt();
            events.append(ev);
        }
    }
    file.close();
    return events;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_epochs2mat");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Export epochs to MATLAB .mat format.\n\n"
                                     "Reads events from a text file, extracts epochs from raw data\n"
                                     "around matching events, and writes each epoch as a MAT5 file.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOpt("raw", "Raw FIFF file.", "file");
    parser.addOption(rawOpt);

    QCommandLineOption eventOpt("event", "Event file (text: sample prev event_id).", "file");
    parser.addOption(eventOpt);

    QCommandLineOption tminOpt("tmin", "Epoch start time in seconds (e.g. -0.2).", "sec");
    parser.addOption(tminOpt);

    QCommandLineOption tmaxOpt("tmax", "Epoch end time in seconds (e.g. 0.5).", "sec");
    parser.addOption(tmaxOpt);

    QCommandLineOption eventIdOpt("event-id", "Event ID to extract.", "id");
    parser.addOption(eventIdOpt);

    QCommandLineOption outOpt("out", "Output directory for .mat files.", "dir");
    parser.addOption(outOpt);

    parser.process(app);

    QString rawFile = parser.value(rawOpt);
    QString eventFile = parser.value(eventOpt);
    QString outDir = parser.value(outOpt);
    float tmin = parser.value(tminOpt).toFloat();
    float tmax = parser.value(tmaxOpt).toFloat();
    int eventId = parser.value(eventIdOpt).toInt();

    if (rawFile.isEmpty() || eventFile.isEmpty() || outDir.isEmpty() || !parser.isSet(tminOpt) || !parser.isSet(tmaxOpt) || !parser.isSet(eventIdOpt)) {
        Log::critical("--raw, --event, --tmin, --tmax, --event-id, and --out are required.");
        parser.showHelp(1);
    }

    if (tmin >= tmax) {
        Log::critical("--tmin must be less than --tmax.");
        return 1;
    }

    const QFileInfo rawFileInfo(rawFile);
    if (!rawFileInfo.isFile() || !rawFileInfo.isReadable()) {
        Log::critical("Cannot read raw data: {}", rawFile.toStdString());
        return 1;
    }

    // Create output directory
    QDir dir(outDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            Log::critical("Cannot create output directory: {}", outDir.toStdString());
            return 1;
        }
    }

    // Load raw data
    Log::info("Loading raw: {}", rawFile.toStdString());
    FiffRawData raw(rawFile.toStdU16String());
    if (raw.info.empty()) {
        Log::critical("Cannot read raw data: {}", rawFile.toStdString());
        return 1;
    }

    double sfreq = raw.info.sfreq;
    int nChan = raw.info.nchan;
    Log::info("  {} channels, {:.1f} Hz", nChan, sfreq);

    // Read events
    QList<EventEntry> events = readEvents(eventFile);
    if (events.isEmpty()) {
        Log::critical("No events read from: {}", eventFile.toStdString());
        return 1;
    }
    Log::info("Read {} events", static_cast<long long>(events.size()));

    // Compute sample offsets for epoch
    int sminOffset = static_cast<int>(qRound(tmin * sfreq));
    int smaxOffset = static_cast<int>(qRound(tmax * sfreq));
    int epochLen = smaxOffset - sminOffset + 1;

    Log::info("Epoch: {:.3f} to {:.3f} s ({} samples), event ID = {}", tmin, tmax, epochLen, eventId);

    // Build time vector
    MatrixXd timesMat(1, epochLen);
    for (int i = 0; i < epochLen; ++i)
        timesMat(0, i) = (sminOffset + i) / sfreq;

    // Extract and write epochs
    int epochCount = 0;
    for (int e = 0; e < events.size(); ++e) {
        if (events[e].id != eventId)
            continue;

        int eventSample = events[e].sample;
        int fromSamp = eventSample + sminOffset;
        int toSamp = eventSample + smaxOffset;

        // Check bounds
        if (fromSamp < raw.first_samp || toSamp > raw.last_samp) {
            Log::info("  Skipping event at sample {} (out of bounds)", eventSample);
            continue;
        }

        MatrixXd data;
        MatrixXd times;
        if (!raw.read_raw_segment(data, times, fromSamp, toSamp)) {
            Log::info("  Warning: cannot read epoch at sample {}", eventSample);
            continue;
        }

        ++epochCount;

        // Write MAT file
        QString matFile = dir.filePath(QString("epoch_%1.mat").arg(epochCount, 3, 10, QChar('0')));
        QFile outF(matFile);
        if (!outF.open(QIODevice::WriteOnly)) {
            Log::critical("Cannot open output file: {}", matFile.toStdString());
            return 1;
        }

        QDataStream ds(&outF);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

        writeMatHeader(ds, "mne_epochs2mat");
        writeMatrixVariable(ds, "data", data);

        MatrixXd sfreqMat(1, 1);
        sfreqMat(0, 0) = sfreq;
        writeMatrixVariable(ds, "sfreq", sfreqMat);

        writeMatrixVariable(ds, "times", timesMat);

        outF.close();

        Log::info("  Wrote epoch {} -> {} ({} x {})", epochCount, matFile.toStdString(), nChan, epochLen);
    }

    Log::info("Done. Extracted {} epochs for event ID {}.", epochCount, eventId);
    return 0;
}
