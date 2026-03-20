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
 * @brief    Convert KIT/Ricoh (Yokogawa) .sqd/.con MEG data to FIFF format.
 *
 *           Reads KIT system data with an optional sensor layout file and
 *           HPI coil definitions for coordinate system alignment, then
 *           writes a FIFF raw file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_types.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QFileInfo>

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
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

// KIT system constants
#define KIT_HEADER_SIZE    16384   // Typical header size
#define KIT_SYSTEM_ID_OFFSET 16
#define KIT_NCHAN_OFFSET   128
#define KIT_SFREQ_OFFSET   136
#define KIT_NSAMP_OFFSET   144
#define KIT_DATA_OFFSET_OFFSET 152  // offset to data start

//=============================================================================================================
// Structures
//=============================================================================================================

struct KitCoilDef {
    Vector3d position;    // in mm (KIT device coords)
    Vector3d orientation; // unit normal
};

struct KitSensorInfo {
    QString name;
    int type;             // 1=MEG, 2=ref, 3=trigger, 0=misc
    KitCoilDef coil;
    double gain;
};

struct KitDatasetInfo {
    int nChannels;
    int nSamples;
    double sfreq;
    int dataOffset;       // byte offset to data in sqd file
    int systemId;
    QList<KitSensorInfo> sensors;
};

//=============================================================================================================
// Parse KIT .sqd header
//=============================================================================================================

static bool parseSqdHeader(const QString &sqdPath, KitDatasetInfo &info)
{
    QFile file(sqdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open SQD file: %s", qPrintable(sqdPath));
        return false;
    }

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    // Read system ID
    file.seek(KIT_SYSTEM_ID_OFFSET);
    qint32 sysId;
    ds >> sysId;
    info.systemId = sysId;

    // Read number of channels
    file.seek(KIT_NCHAN_OFFSET);
    qint32 nChan;
    ds >> nChan;
    info.nChannels = nChan;

    // Read sample rate
    file.seek(KIT_SFREQ_OFFSET);
    double sfreq;
    ds >> sfreq;
    info.sfreq = sfreq;

    // Read number of samples
    file.seek(KIT_NSAMP_OFFSET);
    qint32 nSamp;
    ds >> nSamp;
    info.nSamples = nSamp;

    // Read data offset
    file.seek(KIT_DATA_OFFSET_OFFSET);
    qint32 dataOff;
    ds >> dataOff;
    info.dataOffset = dataOff;
    if (info.dataOffset <= 0)
        info.dataOffset = KIT_HEADER_SIZE;

    printf("KIT dataset: system=%d, %d channels, %d samples, %.1f Hz\n",
           info.systemId, info.nChannels, info.nSamples, info.sfreq);

    // Initialize sensors with default info
    for (int c = 0; c < info.nChannels; ++c) {
        KitSensorInfo sen;
        sen.name = QString("MEG %1").arg(c + 1, 3, 10, QChar('0'));
        sen.type = (c < 160) ? 1 : 0;  // First 160 are typically MEG
        sen.gain = 1.0;
        sen.coil.position = Vector3d::Zero();
        sen.coil.orientation = Vector3d(0, 0, 1);
        info.sensors.append(sen);
    }

    file.close();
    return true;
}

//=============================================================================================================
// Read sensor layout file (tab/space-delimited: idx x y z ox oy oz)
//=============================================================================================================

static bool readSensorLayout(const QString &snsPath, KitDatasetInfo &info)
{
    QFile file(snsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open sensor file: %s", qPrintable(snsPath));
        return false;
    }

    QTextStream in(&file);
    int count = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('%'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 7)
            continue;

        int idx = parts[0].toInt() - 1;  // 1-based to 0-based
        if (idx < 0 || idx >= info.nChannels)
            continue;

        // Positions in mm, convert to m
        info.sensors[idx].coil.position(0) = parts[1].toDouble() * 0.001;
        info.sensors[idx].coil.position(1) = parts[2].toDouble() * 0.001;
        info.sensors[idx].coil.position(2) = parts[3].toDouble() * 0.001;
        info.sensors[idx].coil.orientation(0) = parts[4].toDouble();
        info.sensors[idx].coil.orientation(1) = parts[5].toDouble();
        info.sensors[idx].coil.orientation(2) = parts[6].toDouble();
        info.sensors[idx].type = 1;  // MEG channel
        count++;
    }

    file.close();
    printf("Read %d sensor positions from %s\n", count, qPrintable(snsPath));
    return true;
}

//=============================================================================================================
// Read HPI coil positions and compute Procrustes alignment (device->head transform)
//=============================================================================================================

static bool computeDevHeadTransform(const QString &hpiPath, FiffCoordTrans &trans)
{
    // HPI file format: pairs of device and head coordinate lines
    // Format: x y z (in mm) - first N lines are device coords, next N are head coords
    // Or alternating lines: device_x device_y device_z head_x head_y head_z
    QFile file(hpiPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        printf("Warning: Cannot open HPI file: %s\n", qPrintable(hpiPath));
        return false;
    }

    QTextStream in(&file);
    QList<Vector3d> devicePts;
    QList<Vector3d> headPts;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= 6) {
            // Format: dev_x dev_y dev_z head_x head_y head_z
            Vector3d dev, head;
            dev(0) = parts[0].toDouble() * 0.001;   // mm to m
            dev(1) = parts[1].toDouble() * 0.001;
            dev(2) = parts[2].toDouble() * 0.001;
            head(0) = parts[3].toDouble() * 0.001;
            head(1) = parts[4].toDouble() * 0.001;
            head(2) = parts[5].toDouble() * 0.001;
            devicePts.append(dev);
            headPts.append(head);
        } else if (parts.size() >= 3) {
            // Alternating device/head lines
            Vector3d pt;
            pt(0) = parts[0].toDouble() * 0.001;
            pt(1) = parts[1].toDouble() * 0.001;
            pt(2) = parts[2].toDouble() * 0.001;
            if (devicePts.size() <= headPts.size())
                devicePts.append(pt);
            else
                headPts.append(pt);
        }
    }
    file.close();

    int nPts = std::min(devicePts.size(), headPts.size());
    if (nPts < 3) {
        printf("Warning: Need at least 3 HPI points, got %d\n", nPts);
        return false;
    }

    // Procrustes alignment: find rotation R and translation t such that
    // head = R * device + t  (minimizing least squares)

    // Compute centroids
    Vector3d devCenter = Vector3d::Zero();
    Vector3d headCenter = Vector3d::Zero();
    for (int i = 0; i < nPts; ++i) {
        devCenter += devicePts[i];
        headCenter += headPts[i];
    }
    devCenter /= nPts;
    headCenter /= nPts;

    // Center the points
    MatrixXd D(3, nPts), H(3, nPts);
    for (int i = 0; i < nPts; ++i) {
        D.col(i) = devicePts[i] - devCenter;
        H.col(i) = headPts[i] - headCenter;
    }

    // SVD of cross-covariance: H * D^T = U * S * V^T
    Matrix3d crossCov = H * D.transpose();
    JacobiSVD<Matrix3d> svd(crossCov, ComputeFullU | ComputeFullV);
    Matrix3d U = svd.matrixU();
    Matrix3d V = svd.matrixV();

    // Handle reflection
    double det = (U * V.transpose()).determinant();
    Matrix3d S = Matrix3d::Identity();
    if (det < 0)
        S(2, 2) = -1.0;

    Matrix3d R = U * S * V.transpose();
    Vector3d t = headCenter - R * devCenter;

    // Build 4x4 transform
    trans.from = FIFFV_COORD_DEVICE;
    trans.to = FIFFV_COORD_HEAD;
    trans.trans = Matrix4f::Identity();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            trans.trans(i, j) = static_cast<float>(R(i, j));
        trans.trans(i, 3) = static_cast<float>(t(i));
    }

    // Inverse
    Matrix3d Ri = R.transpose();
    Vector3d ti = -Ri * t;
    trans.invtrans = Matrix4f::Identity();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            trans.invtrans(i, j) = static_cast<float>(Ri(i, j));
        trans.invtrans(i, 3) = static_cast<float>(ti(i));
    }

    // Compute alignment error
    double rms = 0.0;
    for (int i = 0; i < nPts; ++i) {
        Vector3d fitted = R * devicePts[i] + t;
        rms += (fitted - headPts[i]).squaredNorm();
    }
    rms = sqrt(rms / nPts);
    printf("HPI alignment: %d points, RMS error: %.2f mm\n", nPts, rms * 1000.0);

    return true;
}

//=============================================================================================================
// Read KIT .sqd data
//=============================================================================================================

static bool readSqdData(const QString &sqdPath, const KitDatasetInfo &info, MatrixXd &data)
{
    QFile file(sqdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open SQD: %s", qPrintable(sqdPath));
        return false;
    }

    file.seek(info.dataOffset);

    int nChan = info.nChannels;
    int nSamp = info.nSamples;
    data.resize(nChan, nSamp);

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);

    // KIT data is typically stored as 16-bit integers, channel-interleaved
    for (int s = 0; s < nSamp; ++s) {
        for (int c = 0; c < nChan; ++c) {
            qint16 val;
            ds >> val;
            data(c, s) = static_cast<double>(val) * info.sensors[c].gain;
        }
    }

    file.close();
    printf("Read %d channels x %d samples\n", nChan, nSamp);
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_kit2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert KIT/Ricoh .sqd/.con MEG data to FIFF format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sqdOpt("sqd", "Input KIT .sqd or .con file.", "file");
    parser.addOption(sqdOpt);

    QCommandLineOption snsOpt("sns", "Sensor layout definition file.", "file");
    parser.addOption(snsOpt);

    QCommandLineOption hpiOpt("hpi", "HPI coil positions in head coords.", "file");
    parser.addOption(hpiOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption stimOpt("stim", "Comma-separated stimulus channel indices.", "list");
    parser.addOption(stimOpt);

    parser.process(app);

    QString sqdFile = parser.value(sqdOpt);
    QString snsFile = parser.value(snsOpt);
    QString hpiFile = parser.value(hpiOpt);
    QString outFile = parser.value(outOpt);
    QList<int> stimChannels;
    if (parser.isSet(stimOpt)) {
        QStringList parts = parser.value(stimOpt).split(',');
        for (const QString &p : parts)
            stimChannels.append(p.trimmed().toInt());
    }

    if (sqdFile.isEmpty()) { qCritical("--sqd is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Parse SQD header
    KitDatasetInfo info;
    if (!parseSqdHeader(sqdFile, info))
        return 1;

    // Read optional sensor layout
    if (!snsFile.isEmpty()) {
        if (!readSensorLayout(snsFile, info))
            return 1;
    }

    // Mark stimulus channels
    for (int idx : stimChannels) {
        if (idx >= 0 && idx < info.nChannels)
            info.sensors[idx].type = 3;  // stimulus
    }

    // Compute device-to-head transform from HPI coils
    FiffCoordTrans devHeadTrans;
    devHeadTrans.from = FIFFV_COORD_DEVICE;
    devHeadTrans.to = FIFFV_COORD_HEAD;
    devHeadTrans.trans = Matrix4f::Identity();
    devHeadTrans.invtrans = Matrix4f::Identity();

    if (!hpiFile.isEmpty())
        computeDevHeadTransform(hpiFile, devHeadTrans);

    // Read data
    MatrixXd data;
    if (!readSqdData(sqdFile, info, data))
        return 1;

    // Build FiffInfo
    int nChan = info.nChannels;
    FiffInfo fiffInfo;
    fiffInfo.sfreq = info.sfreq;
    fiffInfo.nchan = nChan;
    fiffInfo.meas_date[0] = static_cast<fiff_int_t>(QDateTime::currentDateTime().toSecsSinceEpoch()); fiffInfo.meas_date[1] = 0;
    fiffInfo.dev_head_t = devHeadTrans;

    for (int c = 0; c < nChan; ++c) {
        FiffChInfo ch;
        ch.scanNo = c + 1;
        ch.logNo = c + 1;
        ch.ch_name = info.sensors[c].name;
        ch.cal = 1.0;
        ch.range = 1.0;

        switch (info.sensors[c].type) {
        case 1: // MEG
            ch.kind = FIFFV_MEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_KIT_GRAD;
            ch.unit = FIFF_UNIT_T;
            break;
        case 2: // Reference
            ch.kind = FIFFV_REF_MEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_KIT_GRAD;
            ch.unit = FIFF_UNIT_T;
            break;
        case 3: // Stimulus
            ch.kind = FIFFV_STIM_CH;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
            ch.unit = FIFF_UNIT_V;
            break;
        default: // Misc
            ch.kind = FIFFV_MISC_CH;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
            ch.unit = FIFF_UNIT_V;
            break;
        }

        // Set coil position
        ch.chpos.r0(0) = static_cast<float>(info.sensors[c].coil.position(0));
        ch.chpos.r0(1) = static_cast<float>(info.sensors[c].coil.position(1));
        ch.chpos.r0(2) = static_cast<float>(info.sensors[c].coil.position(2));
        ch.chpos.ez(0) = static_cast<float>(info.sensors[c].coil.orientation(0));
        ch.chpos.ez(1) = static_cast<float>(info.sensors[c].coil.orientation(1));
        ch.chpos.ez(2) = static_cast<float>(info.sensors[c].coil.orientation(2));

        fiffInfo.chs.append(ch);
        fiffInfo.ch_names.append(ch.ch_name);
    }

    // Write FIFF
    QFile outF(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(outF, fiffInfo, cals);
    if (!stream) {
        qCritical("Cannot open output: %s", qPrintable(outFile));
        return 1;
    }

    int nSamples = data.cols();
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    printf("Written FIFF: %s (%d channels, %d samples, %.1f Hz)\n",
           qPrintable(outFile), nChan, nSamples, info.sfreq);

    return 0;
}
