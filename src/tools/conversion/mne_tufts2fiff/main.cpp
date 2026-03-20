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
 * @brief    Implements mne_tufts2fiff: convert Tufts University EEG data to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_file.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>

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
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
// Tufts raw file header structure
//=============================================================================================================

struct TuftsHeader {
    qint32 nChannels;
    qint32 nSamples;
    float sFreq;
    qint32 dataOffset;
};

//=============================================================================================================

static bool readTuftsHeader(QFile& file, TuftsHeader& header)
{
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    // Tufts raw format header
    qint16 nch;
    in >> nch;
    header.nChannels = nch;

    qint16 dummy;
    in >> dummy; // epoch size or flags

    qint32 nSamples;
    in >> nSamples;
    header.nSamples = nSamples;

    // Sampling frequency (typically stored as period in ms)
    qint16 periodMs;
    in >> periodMs;
    if (periodMs > 0)
        header.sFreq = 1000.0f / periodMs;
    else
        header.sFreq = 250.0f; // default

    header.dataOffset = 512; // typical header size

    printf("Tufts header: %d channels, %d samples, %.1f Hz\n",
           header.nChannels, header.nSamples, header.sFreq);
    return true;
}

//=============================================================================================================

static bool readElpFile(const QString& filename, MatrixX3f& positions, QStringList& names)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open elp file:" << filename;
        return false;
    }
    QTextStream in(&file);

    QVector<Vector3f> posVec;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('%'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 4) {
            names.append(parts[0]);
            // Positions are typically in spherical or Cartesian coords (mm)
            float x = parts[1].toFloat() * 0.001f;
            float y = parts[2].toFloat() * 0.001f;
            float z = parts[3].toFloat() * 0.001f;
            posVec.append(Vector3f(x, y, z));
        }
    }
    positions.resize(posVec.size(), 3);
    for (int i = 0; i < posVec.size(); i++) {
        positions.row(i) = posVec[i].transpose();
    }
    printf("Read %d electrode positions from %s\n",
           static_cast<int>(posVec.size()), filename.toUtf8().constData());
    return true;
}

//=============================================================================================================

static bool readCalibration(const QString& filename, VectorXf& cals)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open calibration file:" << filename;
        return false;
    }
    QTextStream in(&file);
    QVector<float> calVec;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        bool ok;
        float val = line.toFloat(&ok);
        if (ok && val != 0.0f)
            calVec.append(val);
    }
    cals.resize(calVec.size());
    for (int i = 0; i < calVec.size(); i++)
        cals(i) = calVec[i];
    printf("Read %d calibration values\n", static_cast<int>(calVec.size()));
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_tufts2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert Tufts University EEG data to FIFF format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOpt("raw", "Tufts raw data file.", "file");
    parser.addOption(rawOpt);
    QCommandLineOption calOpt("cal", "Calibration file.", "file");
    parser.addOption(calOpt);
    QCommandLineOption elpOpt("elp", "Electrode position file.", "file");
    parser.addOption(elpOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);
    QCommandLineOption sfreqOpt("sfreq", "Override sampling frequency.", "Hz");
    parser.addOption(sfreqOpt);

    parser.process(app);

    QString rawFile = parser.value(rawOpt);
    QString calFile = parser.value(calOpt);
    QString elpFile = parser.value(elpOpt);
    QString outFile = parser.value(outOpt);
    float sfreqOverride = parser.isSet(sfreqOpt) ? parser.value(sfreqOpt).toFloat() : 0.0f;

    if (rawFile.isEmpty()) {
        fprintf(stderr, "Raw data file (--raw) is required.\n");
        return 1;
    }
    if (outFile.isEmpty()) {
        fprintf(stderr, "Output file (--out) is required.\n");
        return 1;
    }

    //
    // Read Tufts raw data
    //
    QFile raw(rawFile);
    if (!raw.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Cannot open raw file: %s\n", rawFile.toUtf8().constData());
        return 1;
    }

    TuftsHeader header;
    if (!readTuftsHeader(raw, header)) {
        fprintf(stderr, "Failed to read Tufts header.\n");
        return 1;
    }

    if (sfreqOverride > 0.0f)
        header.sFreq = sfreqOverride;

    // Read calibration if provided
    VectorXf cals;
    if (!calFile.isEmpty()) {
        readCalibration(calFile, cals);
    }

    // Read electrode positions if provided
    MatrixX3f elpPositions;
    QStringList elpNames;
    if (!elpFile.isEmpty()) {
        readElpFile(elpFile, elpPositions, elpNames);
    }

    //
    // Create channel info
    //
    int nch = header.nChannels;
    QList<FiffChInfo> chs;
    for (int k = 0; k < nch; k++) {
        FiffChInfo ch;
        ch.scanNo = k + 1;
        ch.logNo = k + 1;
        ch.kind = FIFFV_EEG_CH;
        ch.range = 1.0f;
        ch.cal = (cals.size() > k) ? cals(k) : 1.0f;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.unit = FIFF_UNIT_V;
        ch.unit_mul = FIFF_UNITM_NONE;

        if (k < elpNames.size()) {
            ch.ch_name = elpNames[k];
        } else {
            ch.ch_name = QString("EEG%1").arg(k + 1, 3, 10, QChar('0'));
        }

        if (k < elpPositions.rows()) {
            ch.chpos.r0[0] = elpPositions(k, 0);
            ch.chpos.r0[1] = elpPositions(k, 1);
            ch.chpos.r0[2] = elpPositions(k, 2);
        }

        chs.append(ch);
    }

    //
    // Read raw data
    //
    raw.seek(header.dataOffset);
    int nSamples = header.nSamples;
    MatrixXf data(nch, nSamples);

    QDataStream dataIn(&raw);
    dataIn.setByteOrder(QDataStream::LittleEndian);
    dataIn.setFloatingPointPrecision(QDataStream::SinglePrecision);

    for (int s = 0; s < nSamples; s++) {
        for (int c = 0; c < nch; c++) {
            qint16 val;
            dataIn >> val;
            data(c, s) = static_cast<float>(val);
        }
    }
    raw.close();

    // Apply calibrations
    for (int c = 0; c < nch; c++) {
        if (cals.size() > c && cals(c) != 0.0f) {
            data.row(c) *= cals(c);
        }
    }

    //
    // Write FIFF output
    //
    QFile outF(outFile);
    FiffStream::SPtr stream = FiffStream::start_file(outF);
    if (!stream) {
        fprintf(stderr, "Cannot create output FIFF file.\n");
        return 1;
    }

    // Measurement info
    stream->start_block(FIFFB_MEAS);
    stream->start_block(FIFFB_MEAS_INFO);

    fiff_int_t nchan = nch;
    stream->write_int(FIFF_NCHAN, &nchan);
    float sfreq = header.sFreq;
    stream->write_float(FIFF_SFREQ, &sfreq);

    for (int k = 0; k < nch; k++) {
        stream->write_ch_info(chs[k]);
    }

    // Digitization points (if electrode positions provided)
    if (elpPositions.rows() > 0) {
        stream->start_block(FIFFB_ISOTRAK);
        for (int k = 0; k < elpPositions.rows(); k++) {
            FiffDigPoint dig;
            dig.kind = (k < 3) ? FIFFV_POINT_CARDINAL : FIFFV_POINT_EEG;
            dig.ident = (k < 3) ? (k + 1) : (k - 2);
            dig.r[0] = elpPositions(k, 0);
            dig.r[1] = elpPositions(k, 1);
            dig.r[2] = elpPositions(k, 2);
            dig.coord_frame = FIFFV_COORD_HEAD;
            stream->write_dig_point(dig);
        }
        stream->end_block(FIFFB_ISOTRAK);
    }

    stream->end_block(FIFFB_MEAS_INFO);

    // Raw data
    stream->start_block(FIFFB_RAW_DATA);

    // Write in 10-second buffers
    int bufSize = static_cast<int>(10.0f * header.sFreq);
    for (int start = 0; start < nSamples; start += bufSize) {
        int end = qMin(start + bufSize, nSamples);
        MatrixXf buf = data.block(0, start, nch, end - start);

        fiff_int_t firstSamp = start;
        stream->write_int(FIFF_FIRST_SAMPLE, &firstSamp);
        stream->write_float_matrix(FIFF_DATA_BUFFER, buf);
    }

    stream->end_block(FIFFB_RAW_DATA);
    stream->end_block(FIFFB_MEAS);
    stream->end_file();

    printf("Wrote FIFF: %d channels, %d samples at %.1f Hz to %s\n",
           nch, nSamples, header.sFreq, outFile.toUtf8().constData());

    return 0;
}
