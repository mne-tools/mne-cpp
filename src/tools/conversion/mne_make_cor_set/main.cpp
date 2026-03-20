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
 * @brief    Implements mne_make_cor_set: create MRI description FIFF from FreeSurfer data.
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
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
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

#define COR_NSLICE 256
#define COR_NPIX   256
#define COR_SLICE_SIZE (COR_NPIX * COR_NPIX)

//=============================================================================================================

static bool readCorDirectory(const QString& dirPath, std::vector<std::vector<unsigned char>>& slices)
{
    QDir dir(dirPath);
    slices.resize(COR_NSLICE);

    for (int k = 0; k < COR_NSLICE; k++) {
        QString fname = dir.filePath(QString("COR-%1").arg(k + 1, 3, 10, QChar('0')));
        QFile file(fname);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Cannot open COR file:" << fname;
            return false;
        }
        QByteArray data = file.readAll();
        if (data.size() != COR_SLICE_SIZE) {
            qCritical() << "Unexpected COR slice size:" << data.size() << "expected" << COR_SLICE_SIZE;
            return false;
        }
        slices[k].assign(data.constData(), data.constData() + data.size());
    }
    printf("Read %d COR slices from %s\n", COR_NSLICE, dirPath.toUtf8().constData());
    return true;
}

//=============================================================================================================

static bool writeMriDescription(const QString& filename,
                                const std::vector<std::vector<unsigned char>>& slices,
                                int width, int height, int nslice)
{
    QFile file(filename);
    FiffStream::SPtr stream = FiffStream::start_file(file);
    if (!stream) {
        qCritical() << "Cannot create output FIFF file:" << filename;
        return false;
    }

    // Write MRI data block
    stream->start_block(FIFFB_MRI);

    // Write dimensions
    fiff_int_t val;
    val = width;
    stream->write_int(FIFF_MRI_WIDTH, &val);
    val = height;
    stream->write_int(FIFF_MRI_HEIGHT, &val);
    val = nslice;
    stream->write_int(FIFF_MRI_DEPTH, &val);

    // Pixel size (1mm for COR)
    float fval = 0.001f; // in meters
    stream->write_float(FIFF_MRI_WIDTH_M, &fval);
    stream->write_float(FIFF_MRI_HEIGHT_M, &fval);
    stream->write_float(FIFF_MRI_DEPTH_M, &fval);

    // Voxel-to-RAS coordinate transform (identity * 0.001 for COR data)
    FiffCoordTrans t;
    t.from = FIFFV_COORD_MRI;
    t.to = FIFFV_MNE_COORD_RAS;
    t.trans = Matrix4f::Identity();
    // COR convention: center at (128,128,128), 1mm voxels
    t.trans(0, 3) = -0.128f;
    t.trans(1, 3) = 0.128f;
    t.trans(2, 3) = 0.128f;
    // Axes: COR x = -R, y = A, z = S in RAS
    t.trans(0, 0) = -0.001f;
    t.trans(1, 1) = 0.0f;
    t.trans(1, 2) = 0.001f;
    t.trans(2, 1) = -0.001f;
    t.trans(2, 2) = 0.0f;
    t.invtrans = t.trans.inverse();
    stream->write_coord_trans(t);

    // Write pixel encoding type
    val = 1; // unsigned byte
    stream->write_int(FIFF_MRI_PIXEL_ENCODING, &val);

    // Write slices
    for (int k = 0; k < nslice; k++) {
        QByteArray sliceData(reinterpret_cast<const char*>(slices[k].data()),
                             static_cast<int>(slices[k].size()));

        // Write as FIFF_MRI_PIXEL_DATA
        FiffTag::SPtr tag(new FiffTag());
        tag->kind = FIFF_MRI_PIXEL_DATA;
        tag->type = FIFFT_BYTE;
        tag->resize(sliceData.size());
        memcpy(tag->data(), sliceData.constData(), sliceData.size());
        stream->write_tag(tag);
    }

    stream->end_block(FIFFB_MRI);
    stream->end_file();

    printf("Wrote MRI description to %s (%dx%dx%d)\n",
           filename.toUtf8().constData(), width, height, nslice);
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_cor_set");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Create MRI description FIFF from FreeSurfer COR or mgh/mgz data.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dirOpt("dir", "FreeSurfer COR directory.", "directory");
    parser.addOption(dirOpt);
    QCommandLineOption mghOpt("mgh", "FreeSurfer mgh/mgz volume file.", "file");
    parser.addOption(mghOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString corDir = parser.value(dirOpt);
    QString mghFile = parser.value(mghOpt);
    QString outFile = parser.value(outOpt);

    if (corDir.isEmpty() && mghFile.isEmpty()) {
        fprintf(stderr, "Either --dir or --mgh must be specified.\n");
        return 1;
    }
    if (outFile.isEmpty()) {
        fprintf(stderr, "Output file (--out) is required.\n");
        return 1;
    }

    std::vector<std::vector<unsigned char>> slices;
    int width = COR_NPIX, height = COR_NPIX, nslice = COR_NSLICE;

    if (!corDir.isEmpty()) {
        if (!readCorDirectory(corDir, slices))
            return 1;
    } else if (!mghFile.isEmpty()) {
        // Read mgh/mgz file
        QFile file(mghFile);
        bool isGz = mghFile.endsWith(".mgz") || mghFile.endsWith(".mgh.gz");

        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open mgh file: %s\n", mghFile.toUtf8().constData());
            return 1;
        }

        QByteArray rawData = file.readAll();
        file.close();

        QByteArray data;
        if (isGz) {
            data = qUncompress(rawData);
            if (data.isEmpty()) {
                fprintf(stderr, "Failed to decompress mgz file. Try using an uncompressed .mgh file.\n");
                return 1;
            }
        } else {
            data = rawData;
        }

        // Parse MGH header (big-endian)
        QDataStream in(data);
        in.setByteOrder(QDataStream::BigEndian);

        qint32 version, w, h, d, nframes, type;
        in >> version >> w >> h >> d >> nframes >> type;

        if (version != 1) {
            fprintf(stderr, "Unsupported MGH version: %d\n", version);
            return 1;
        }

        width = w;
        height = h;
        nslice = d;

        // Skip rest of header (284 bytes total - 24 already read)
        in.skipRawData(284 - 24);

        printf("Read MGH: %dx%dx%d, type=%d\n", width, height, nslice, type);

        slices.resize(nslice);
        int sliceSize = width * height;

        for (int k = 0; k < nslice; k++) {
            slices[k].resize(sliceSize);
            if (type == 0) { // MRI_UCHAR
                in.readRawData(reinterpret_cast<char*>(slices[k].data()), sliceSize);
            } else if (type == 1) { // MRI_INT
                for (int j = 0; j < sliceSize; j++) {
                    qint32 val;
                    in >> val;
                    slices[k][j] = static_cast<unsigned char>(qBound(0, val, 255));
                }
            } else if (type == 3) { // MRI_FLOAT
                for (int j = 0; j < sliceSize; j++) {
                    float val;
                    in >> val;
                    slices[k][j] = static_cast<unsigned char>(qBound(0.0f, val, 255.0f));
                }
            } else if (type == 4) { // MRI_SHORT
                for (int j = 0; j < sliceSize; j++) {
                    qint16 val;
                    in >> val;
                    slices[k][j] = static_cast<unsigned char>(qBound(0, static_cast<int>(val), 255));
                }
            } else {
                fprintf(stderr, "Unsupported MGH data type: %d\n", type);
                return 1;
            }
        }
    }

    return writeMriDescription(outFile, slices, width, height, nslice) ? 0 : 1;
}
