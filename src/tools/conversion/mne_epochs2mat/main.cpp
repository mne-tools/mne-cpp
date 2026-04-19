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
 * @brief    Export epochs from a raw FIFF file to MATLAB Level 5 MAT files.
 *
 *           Reads events from a text file, extracts epochs around matching events,
 *           and writes each epoch as a separate MAT file containing the data matrix,
 *           sampling frequency, and time vector.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QRegularExpression>
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

// MAT-file Level 5 constants
static const quint32 miDOUBLE       = 9;
static const quint32 miINT32        = 5;
static const quint32 miUINT32       = 6;
static const quint32 miINT8         = 1;
static const quint32 miMATRIX       = 14;
static const quint32 mxDOUBLE_CLASS = 6;

//=============================================================================================================
// MAT-file writing helpers
//=============================================================================================================

static void writePad(QDataStream &ds, int numBytes)
{
    int pad = (8 - (numBytes % 8)) % 8;
    for (int i = 0; i < pad; ++i)
        ds << static_cast<quint8>(0);
}

static void writeMatTag(QDataStream &ds, quint32 dataType, quint32 numBytes)
{
    ds << dataType;
    ds << numBytes;
}

static void writeSmallTag(QDataStream &ds, quint32 dataType, quint32 numBytes, const QByteArray &data)
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

static void writeMatrixVariable(QDataStream &ds, const QString &name, const MatrixXd &mat)
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

static void writeMatHeader(QDataStream &ds, const QString &toolName)
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

struct EventEntry {
    int sample;
    int prev;
    int id;
};

static QList<EventEntry> readEvents(const QString &filename)
{
    QList<EventEntry> events;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open event file: %s", qPrintable(filename));
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

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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

    if (rawFile.isEmpty() || eventFile.isEmpty() || outDir.isEmpty()
        || !parser.isSet(tminOpt) || !parser.isSet(tmaxOpt) || !parser.isSet(eventIdOpt)) {
        qCritical("--raw, --event, --tmin, --tmax, --event-id, and --out are required.");
        parser.showHelp(1);
    }

    if (tmin >= tmax) {
        qCritical("--tmin must be less than --tmax.");
        return 1;
    }

    // Create output directory
    QDir dir(outDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical("Cannot create output directory: %s", qPrintable(outDir));
            return 1;
        }
    }

    // Load raw data
    fprintf(stderr, "Loading raw: %s\n", qPrintable(rawFile));
    QFile fRaw(rawFile);
    FiffRawData raw(fRaw);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw data: %s", qPrintable(rawFile));
        return 1;
    }

    double sfreq = raw.info.sfreq;
    int nChan = raw.info.nchan;
    fprintf(stderr, "  %d channels, %.1f Hz\n", nChan, sfreq);

    // Read events
    QList<EventEntry> events = readEvents(eventFile);
    if (events.isEmpty()) {
        qCritical("No events read from: %s", qPrintable(eventFile));
        return 1;
    }
    fprintf(stderr, "Read %lld events\n", static_cast<long long>(events.size()));

    // Compute sample offsets for epoch
    int sminOffset = static_cast<int>(qRound(tmin * sfreq));
    int smaxOffset = static_cast<int>(qRound(tmax * sfreq));
    int epochLen = smaxOffset - sminOffset + 1;

    fprintf(stderr, "Epoch: %.3f to %.3f s (%d samples), event ID = %d\n",
            tmin, tmax, epochLen, eventId);

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
            fprintf(stderr, "  Skipping event at sample %d (out of bounds)\n", eventSample);
            continue;
        }

        MatrixXd data;
        MatrixXd times;
        if (!raw.read_raw_segment(data, times, fromSamp, toSamp)) {
            fprintf(stderr, "  Warning: cannot read epoch at sample %d\n", eventSample);
            continue;
        }

        ++epochCount;

        // Write MAT file
        QString matFile = dir.filePath(QString("epoch_%1.mat").arg(epochCount, 3, 10, QChar('0')));
        QFile outF(matFile);
        if (!outF.open(QIODevice::WriteOnly)) {
            qCritical("Cannot open output file: %s", qPrintable(matFile));
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

        fprintf(stderr, "  Wrote epoch %d -> %s (%d x %d)\n",
                epochCount, qPrintable(matFile), nChan, epochLen);
    }

    fprintf(stderr, "Done. Extracted %d epochs for event ID %d.\n", epochCount, eventId);
    return 0;
}
