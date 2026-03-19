//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Export raw FIFF data to MATLAB Level 5 MAT file format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <QByteArray>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

// MAT-file Level 5 constants
static const quint32 miDOUBLE   = 9;
static const quint32 miINT32    = 5;
static const quint32 miUINT32   = 6;
static const quint32 miINT8     = 1;
static const quint32 miMATRIX   = 14;
static const quint32 mxDOUBLE_CLASS = 6;

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Export raw FIFF data to MATLAB .mat file.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --raw <file>       Input raw FIFF file\n");
    fprintf(stderr, "  --out <file>       Output .mat file\n");
    fprintf(stderr, "  --from <samp>      First sample (default: start of file)\n");
    fprintf(stderr, "  --to <samp>        Last sample (default: end of file)\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================
// MAT-file writing helpers
//=============================================================================================================

static void writePad(QDataStream &ds, int numBytes)
{
    // Pad to 8-byte boundary
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
    // Small Data Element format (data <= 4 bytes)
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

// Write a double matrix variable to MAT file
static void writeMatrixVariable(QDataStream &ds, const QString &name, const MatrixXd &mat)
{
    int nRows = mat.rows();
    int nCols = mat.cols();
    QByteArray nameBytes = name.toLatin1();

    // Calculate sizes
    quint32 flagsSize = 8;                      // 2 x uint32 flags
    quint32 dimsSize = 8;                       // 2 x int32 dims
    quint32 nameSize = nameBytes.size();
    quint32 namePadded = nameSize <= 4 ? 4 : nameSize + ((8 - (nameSize % 8)) % 8);
    quint32 nameTagSize = nameSize <= 4 ? 8 : 8 + namePadded;
    quint32 dataSize = nRows * nCols * 8;       // doubles
    quint32 dataPadded = dataSize + ((8 - (dataSize % 8)) % 8);

    // Array flags subelement
    quint32 arrayFlagsSubSize = 8 + flagsSize;
    // Dims subelement
    quint32 dimsSubSize = 8 + dimsSize;
    // Name subelement
    quint32 nameSubSize = nameTagSize;
    // Data subelement
    quint32 dataSubSize = 8 + dataPadded;

    quint32 totalPayload = arrayFlagsSubSize + dimsSubSize + nameSubSize + dataSubSize;

    // miMATRIX tag
    writeMatTag(ds, miMATRIX, totalPayload);

    // Array Flags subelement
    writeMatTag(ds, miUINT32, flagsSize);
    ds << static_cast<quint32>(mxDOUBLE_CLASS);  // flags (double class)
    ds << static_cast<quint32>(0);                // reserved

    // Dimensions subelement
    writeMatTag(ds, miINT32, dimsSize);
    ds << static_cast<qint32>(nRows);
    ds << static_cast<qint32>(nCols);

    // Name subelement
    if (nameSize <= 4) {
        writeSmallTag(ds, miINT8, nameSize, nameBytes);
    } else {
        writeMatTag(ds, miINT8, nameSize);
        ds.writeRawData(nameBytes.constData(), nameSize);
        writePad(ds, nameSize);
    }

    // Data subelement (column-major)
    writeMatTag(ds, miDOUBLE, dataSize);
    for (int c = 0; c < nCols; ++c) {
        for (int r = 0; r < nRows; ++r) {
            ds << mat(r, c);
        }
    }
    writePad(ds, dataSize);
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString rawFile;
    QString outFile;
    int fromSamp = -1;
    int toSamp = -1;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--raw") == 0) {
            if (++k >= argc) { qCritical("--raw: argument required."); return 1; }
            rawFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--from") == 0) {
            if (++k >= argc) { qCritical("--from: argument required."); return 1; }
            fromSamp = atoi(argv[k]);
        }
        else if (strcmp(argv[k], "--to") == 0) {
            if (++k >= argc) { qCritical("--to: argument required."); return 1; }
            toSamp = atoi(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (rawFile.isEmpty()) { qCritical("--raw is required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }

    // Load raw data
    QFile fRaw(rawFile);
    FiffRawData raw(fRaw);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw: %s", qPrintable(rawFile));
        return 1;
    }

    int nChan = raw.info.nchan;
    double sfreq = raw.info.sfreq;
    int first = (fromSamp >= 0) ? fromSamp : raw.first_samp;
    int last = (toSamp >= 0) ? toSamp : raw.last_samp;

    printf("Raw: %d channels, %.1f Hz, samples %d..%d\n", nChan, sfreq, first, last);

    // Read data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, first, last)) {
        qCritical("Cannot read data segment.");
        return 1;
    }

    printf("Read %lld channels x %lld samples\n", data.rows(), data.cols());

    // Build channel names
    QStringList chNames;
    for (int c = 0; c < nChan; ++c)
        chNames << raw.info.chs[c].ch_name;

    // Write MAT file (Level 5 format)
    QFile outF(outFile);
    if (!outF.open(QIODevice::WriteOnly)) {
        qCritical("Cannot open output: %s", qPrintable(outFile));
        return 1;
    }

    QDataStream ds(&outF);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    // --- 128-byte header ---
    // Text field (116 bytes)
    QByteArray headerText = QByteArray("MATLAB 5.0 MAT-file, created by mne_raw2mat");
    headerText.append(QByteArray(116 - headerText.size(), ' '));
    ds.writeRawData(headerText.constData(), 116);

    // Subsystem data offset (8 bytes, zeros)
    for (int i = 0; i < 8; ++i)
        ds << static_cast<quint8>(0);

    // Version (2 bytes) + Endian indicator (2 bytes)
    ds << static_cast<quint16>(0x0100);  // version
    ds << static_cast<quint16>(0x4D49);  // 'MI' = little-endian

    // --- Write 'data' variable (channels x samples) ---
    writeMatrixVariable(ds, "data", data);

    // --- Write 'sfreq' variable (1x1) ---
    MatrixXd sfreqMat(1, 1);
    sfreqMat(0, 0) = sfreq;
    writeMatrixVariable(ds, "sfreq", sfreqMat);

    // --- Write 'times' variable (1 x samples) ---
    writeMatrixVariable(ds, "times", times);

    outF.close();
    printf("Written MAT file: %s\n", qPrintable(outFile));
    printf("Variables: data (%d x %lld), sfreq (1x1), times (1 x %lld)\n",
           nChan, data.cols(), times.cols());

    return 0;
}
