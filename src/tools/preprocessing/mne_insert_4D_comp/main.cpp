//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * @brief    Implements mne_insert_4D_comp: merge 4D Magnes compensation channels with Procrustes alignment.
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
#include <QRegularExpression>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static bool readSensorPositions(const QString& filename, MatrixX3f& pos, QStringList& names)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open sensor position file:" << filename;
        return false;
    }
    QTextStream in(&file);
    QVector<Vector3f> posVec;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 4) {
            names.append(parts[0]);
            // Convert from mm to meters
            posVec.append(Vector3f(parts[1].toFloat() * 0.001f,
                                   parts[2].toFloat() * 0.001f,
                                   parts[3].toFloat() * 0.001f));
        }
    }
    pos.resize(posVec.size(), 3);
    for (int i = 0; i < posVec.size(); i++)
        pos.row(i) = posVec[i].transpose();
    return true;
}

//=============================================================================================================

static Matrix4f procrustes(const MatrixX3f& source, const MatrixX3f& target)
{
    // Procrustes alignment: find rotation + translation to map source onto target
    // Both matrices should have the same number of rows (corresponding points)
    int n = static_cast<int>(source.rows());

    // Centroids
    Vector3f centS = source.colwise().mean();
    Vector3f centT = target.colwise().mean();

    // Center the point sets
    MatrixX3f srcCentered = source.rowwise() - centS.transpose();
    MatrixX3f tgtCentered = target.rowwise() - centT.transpose();

    // Compute the cross-covariance matrix
    Matrix3f H = srcCentered.transpose() * tgtCentered;

    // SVD
    JacobiSVD<Matrix3f> svd(H, ComputeFullU | ComputeFullV);
    Matrix3f U = svd.matrixU();
    Matrix3f V = svd.matrixV();

    // Handle reflection
    float d = (V * U.transpose()).determinant();
    Matrix3f S = Matrix3f::Identity();
    if (d < 0)
        S(2, 2) = -1.0f;

    // Rotation
    Matrix3f R = V * S * U.transpose();

    // Translation
    Vector3f t = centT - R * centS;

    // Build 4x4 transform
    Matrix4f T = Matrix4f::Identity();
    T.block<3, 3>(0, 0) = R;
    T.block<3, 1>(0, 3) = t;
    return T;
}

//=============================================================================================================

static bool readRefData(const QString& filename, MatrixXf& refData, int& nChannels, int& nSamples)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open reference data file:" << filename;
        return false;
    }
    QTextStream in(&file);

    // First pass: count dimensions
    QVector<QVector<float>> rows;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        QVector<float> row;
        for (const QString& p : parts) {
            bool ok;
            float val = p.toFloat(&ok);
            if (ok) row.append(val);
        }
        if (!row.isEmpty())
            rows.append(row);
    }

    if (rows.isEmpty()) {
        qCritical() << "No data in reference file";
        return false;
    }

    nSamples = rows.size();
    nChannels = rows[0].size();

    refData.resize(nChannels, nSamples);
    for (int s = 0; s < nSamples; s++) {
        for (int c = 0; c < qMin(nChannels, rows[s].size()); c++) {
            refData(c, s) = rows[s][c];
        }
    }

    printf("Read reference data: %d channels x %d samples\n", nChannels, nSamples);
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_insert_4D_comp");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Merge 4D Magnes WH3600 compensation data with a FIFF file.\n\nSupports Procrustes alignment of sensor positions.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input FIFF file with MEG data.", "file");
    parser.addOption(inOpt);
    QCommandLineOption refOpt("ref", "Reference channel data file (text).", "file");
    parser.addOption(refOpt);
    QCommandLineOption refposOpt("refpos", "Reference sensor positions file.", "file");
    parser.addOption(refposOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);
    QCommandLineOption megposOpt("megpos", "MEG sensor positions file (for alignment).", "file");
    parser.addOption(megposOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString refFile = parser.value(refOpt);
    QString refPosFile = parser.value(refposOpt);
    QString megPosFile = parser.value(megposOpt);
    QString outFile = parser.value(outOpt);

    if (inFile.isEmpty() || refFile.isEmpty() || outFile.isEmpty()) {
        fprintf(stderr, "Required: --in, --ref, --out\n");
        return 1;
    }

    //
    // Read input FIFF
    //
    QFile fIn(inFile);
    FiffRawData raw(fIn);
    if (!raw.info.isEmpty()) {
        printf("Read input: %d channels at %.1f Hz\n",
               raw.info.nchan, raw.info.sfreq);
    } else {
        fprintf(stderr, "Failed to read input FIFF: %s\n", inFile.toUtf8().constData());
        return 1;
    }

    //
    // Read reference data
    //
    MatrixXf refData;
    int nRefCh = 0, nRefSamp = 0;
    if (!readRefData(refFile, refData, nRefCh, nRefSamp))
        return 1;

    //
    // Read sensor positions for alignment (optional)
    //
    Matrix4f alignTransform = Matrix4f::Identity();
    if (!refPosFile.isEmpty() && !megPosFile.isEmpty()) {
        MatrixX3f refPos, megPos;
        QStringList refNames, megNames;
        readSensorPositions(refPosFile, refPos, refNames);
        readSensorPositions(megPosFile, megPos, megNames);

        // Use matching positions for Procrustes alignment
        int nMatch = qMin(static_cast<int>(refPos.rows()),
                          static_cast<int>(megPos.rows()));
        if (nMatch >= 3) {
            alignTransform = procrustes(refPos.topRows(nMatch), megPos.topRows(nMatch));
            printf("Computed Procrustes alignment from %d matching points\n", nMatch);
        }
    }

    //
    // Create reference channel info
    //
    QList<FiffChInfo> refChs;
    for (int k = 0; k < nRefCh; k++) {
        FiffChInfo ch;
        ch.scanNo = raw.info.nchan + k + 1;
        ch.logNo = k + 1;
        ch.kind = FIFFV_REF_MEG_CH;
        ch.range = 1.0f;
        ch.cal = 1.0f;
        ch.chpos.coil_type = 4003; // FIFFV_COIL_MAGNES_REF_MAG
        ch.unit = FIFF_UNIT_T;
        ch.unit_mul = FIFF_UNITM_NONE;
        ch.ch_name = QString("REF%1").arg(k + 1, 3, 10, QChar('0'));
        refChs.append(ch);
    }

    //
    // Write output FIFF with merged channels
    //
    QFile fOut(outFile);
    FiffStream::SPtr outStream = FiffStream::start_file(fOut);
    if (!outStream) {
        fprintf(stderr, "Cannot create output file.\n");
        return 1;
    }

    // Write measurement info with original + reference channels
    int totalCh = raw.info.nchan + nRefCh;

    outStream->start_block(FIFFB_MEAS);
    outStream->start_block(FIFFB_MEAS_INFO);

    fiff_int_t nch = totalCh;
    outStream->write_int(FIFF_NCHAN, &nch);
    float sfreq = raw.info.sfreq;
    outStream->write_float(FIFF_SFREQ, &sfreq);

    // Write original channel info
    for (int k = 0; k < raw.info.nchan; k++) {
        outStream->write_ch_info(raw.info.chs[k]);
    }

    // Write reference channel info
    for (int k = 0; k < nRefCh; k++) {
        outStream->write_ch_info(refChs[k]);
    }

    // Copy digitization data if present
    if (!raw.info.dig.isEmpty()) {
        outStream->start_block(FIFFB_ISOTRAK);
        for (int k = 0; k < raw.info.dig.size(); k++) {
            outStream->write_dig_point(raw.info.dig[k]);
        }
        outStream->end_block(FIFFB_ISOTRAK);
    }

    // Write coordinate transforms if present
    if (raw.info.dev_head_t.from != -1) {
        outStream->write_coord_trans(raw.info.dev_head_t);
    }

    outStream->end_block(FIFFB_MEAS_INFO);

    // Write raw data with merged reference channels
    outStream->start_block(FIFFB_RAW_DATA);

    int bufSize = static_cast<int>(10.0f * raw.info.sfreq);
    MatrixXd readData;
    MatrixXd times;

    for (int start = raw.first_samp; start <= raw.last_samp; start += bufSize) {
        int end = qMin(start + bufSize - 1, static_cast<int>(raw.last_samp));
        if (!raw.read_raw_segment(readData, times, start, end))
            break;

        int nSamp = static_cast<int>(readData.cols());

        // Build merged data matrix
        MatrixXf merged(totalCh, nSamp);
        merged.topRows(raw.info.nchan) = readData.cast<float>();

        // Insert reference data (aligned to this segment)
        int refStart = start - static_cast<int>(raw.first_samp);
        for (int c = 0; c < nRefCh; c++) {
            for (int s = 0; s < nSamp; s++) {
                int refIdx = refStart + s;
                if (refIdx >= 0 && refIdx < nRefSamp)
                    merged(raw.info.nchan + c, s) = refData(c, refIdx);
                else
                    merged(raw.info.nchan + c, s) = 0.0f;
            }
        }

        fiff_int_t firstSamp = start;
        outStream->write_int(FIFF_FIRST_SAMPLE, &firstSamp);
        outStream->write_float_matrix(FIFF_DATA_BUFFER, merged);
    }

    outStream->end_block(FIFFB_RAW_DATA);
    outStream->end_block(FIFFB_MEAS);
    outStream->end_file();

    printf("Wrote merged FIFF: %d channels (%d original + %d reference) to %s\n",
           totalCh, raw.info.nchan, nRefCh, outFile.toUtf8().constData());

    return 0;
}
