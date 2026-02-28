//=============================================================================================================
/**
 * @file     mri_mgh_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 *
 * @brief    MriMghIO class definition.
 *
 *           Reader for FreeSurfer MGH/MGZ volume files.
 *           Ported from make_mgh_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_mgh_io.h"

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDebug>
#include <QRegularExpression>

#include <zlib.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MriMghIO::read(const QString& mgzFile,
                    MriVolData& volData,
                    QVector<FiffCoordTrans>& additionalTrans,
                    const QString& subjectMriDir,
                    bool verbose)
{
    volData.fileName = mgzFile;

    // Step 1: Get raw (decompressed) bytes
    bool isCompressed = mgzFile.endsWith(".mgz", Qt::CaseInsensitive);
    QByteArray fileData;

    if (isCompressed) {
        if (!decompress(mgzFile, fileData)) {
            return false;
        }
    } else {
        QFile file(mgzFile);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "MriMghIO::read - Could not open" << mgzFile;
            return false;
        }
        fileData = file.readAll();
        file.close();
    }

    if (fileData.size() < MRI_MGH_DATA_OFFSET) {
        qCritical() << "MriMghIO::read - File" << mgzFile
                     << "is too small to be a valid MGH file ("
                     << fileData.size() << "bytes)";
        return false;
    }

    // Step 2: Parse header
    if (!parseHeader(fileData, volData, verbose)) {
        return false;
    }

    // Step 3: Build the voxel -> surface RAS transform
    Matrix4f vox2ras = volData.computeVox2Ras();
    volData.voxelSurfRasT = FiffCoordTrans(
        FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, vox2ras, true);

    if (verbose) {
        printf("Voxel -> Surface RAS transform:\n");
        for (int r = 0; r < 4; ++r) {
            printf("  %10.6f %10.6f %10.6f %10.6f\n",
                   vox2ras(r, 0), vox2ras(r, 1), vox2ras(r, 2), vox2ras(r, 3));
        }
    }

    // Step 4: Read voxel data
    if (!readVoxelData(fileData, volData)) {
        return false;
    }

    // Step 5: Parse footer (optional)
    parseFooter(fileData, volData, additionalTrans, subjectMriDir, verbose);

    if (verbose) {
        printf("Read %d slices from %s (%dx%d pixels)\n",
               static_cast<int>(volData.slices.size()), qPrintable(mgzFile),
               volData.width, volData.height);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::decompress(const QString& mgzFile, QByteArray& rawData)
{
    QFile file(mgzFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "MriMghIO::decompress - Could not open" << mgzFile;
        return false;
    }
    QByteArray compressedData = file.readAll();
    file.close();

    if (compressedData.isEmpty()) {
        qCritical() << "MriMghIO::decompress - File is empty:" << mgzFile;
        return false;
    }

    // Use zlib to decompress gzip data in memory
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    // MAX_WBITS + 16 tells zlib to detect and handle gzip headers
    int ret = inflateInit2(&strm, MAX_WBITS + 16);
    if (ret != Z_OK) {
        qCritical() << "MriMghIO::decompress - inflateInit2 failed";
        return false;
    }

    strm.next_in = reinterpret_cast<Bytef*>(compressedData.data());
    strm.avail_in = static_cast<uInt>(compressedData.size());

    const int chunkSize = 256 * 1024;  // 256 KB chunks
    rawData.clear();

    do {
        rawData.resize(rawData.size() + chunkSize);
        strm.next_out = reinterpret_cast<Bytef*>(rawData.data() + rawData.size() - chunkSize);
        strm.avail_out = chunkSize;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            qCritical() << "MriMghIO::decompress - inflate failed for" << mgzFile
                         << "- zlib error:" << ret;
            inflateEnd(&strm);
            return false;
        }
    } while (ret != Z_STREAM_END);

    // Trim to actual decompressed size
    rawData.resize(rawData.size() - static_cast<int>(strm.avail_out));
    inflateEnd(&strm);

    return true;
}

//=============================================================================================================

bool MriMghIO::parseHeader(const QByteArray& data, MriVolData& volData, bool verbose)
{
    //
    // MGH header layout (all big-endian):
    //   Bytes 0-3:    version (int32)
    //   Bytes 4-7:    width (int32)
    //   Bytes 8-11:   height (int32)
    //   Bytes 12-15:  depth (int32)
    //   Bytes 16-19:  nframes (int32)
    //   Bytes 20-23:  type (int32)
    //   Bytes 24-27:  dof (int32)
    //   Bytes 28-29:  goodRASflag (int16)
    //   Bytes 30-41:  spacingX/Y/Z (3×float32) — only if goodRASflag > 0
    //   Bytes 42-77:  Mdc (9×float32) — direction cosines, only if goodRASflag > 0
    //   Bytes 78-89:  c_ras (3×float32) — center RAS, only if goodRASflag > 0
    //   Bytes 90-283: unused (padding)
    //

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    qint32 version, width, height, depth, nframes, type, dof;
    stream >> version >> width >> height >> depth >> nframes >> type >> dof;

    if (version != MRI_MGH_VERSION) {
        qCritical() << "MriMghIO::parseHeader - Unknown MGH version:" << version;
        return false;
    }

    volData.version = version;
    volData.width   = width;
    volData.height  = height;
    volData.depth   = depth;
    volData.nframes = nframes;
    volData.type    = type;
    volData.dof     = dof;

    if (verbose) {
        printf("MGH file: %dx%dx%d, %d frame(s), type=%d\n",
               width, height, depth, nframes, type);
    }

    // goodRASflag (2 bytes short)
    qint16 goodRASflag;
    stream >> goodRASflag;
    volData.rasGood = (goodRASflag > 0);

    if (goodRASflag > 0) {
        // Voxel sizes
        stream >> volData.xsize >> volData.ysize >> volData.zsize;

        // Direction cosines (Mdc matrix):
        //   xr, xa, xs  (x-direction cosines)
        //   yr, ya, ys  (y-direction cosines)
        //   zr, za, zs  (z-direction cosines)
        stream >> volData.x_ras[0] >> volData.x_ras[1] >> volData.x_ras[2];
        stream >> volData.y_ras[0] >> volData.y_ras[1] >> volData.y_ras[2];
        stream >> volData.z_ras[0] >> volData.z_ras[1] >> volData.z_ras[2];

        // Center RAS
        stream >> volData.c_ras[0] >> volData.c_ras[1] >> volData.c_ras[2];
    }
    // Else: default values from MriVolData constructor are used

    if (verbose) {
        printf("Voxel sizes: %.4f x %.4f x %.4f mm\n",
               volData.xsize, volData.ysize, volData.zsize);
        printf("goodRAS: %d\n", goodRASflag);
        printf("c_ras: %.4f %.4f %.4f\n",
               volData.c_ras[0], volData.c_ras[1], volData.c_ras[2]);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::readVoxelData(const QByteArray& data, MriVolData& volData)
{
    //
    // Read voxel data starting at byte 284 (MRI_MGH_DATA_OFFSET).
    // Data layout in MGH: [width][height][depth][frames] in Fortran order (x fastest).
    // Only the first frame is read.
    //

    int bytesPerVoxel;
    switch (volData.type) {
        case MRI_UCHAR: bytesPerVoxel = 1; break;
        case MRI_SHORT: bytesPerVoxel = 2; break;
        case MRI_INT:   bytesPerVoxel = 4; break;
        case MRI_FLOAT: bytesPerVoxel = 4; break;
        default:
            qCritical() << "MriMghIO::readVoxelData - Unsupported MGH data type:" << volData.type;
            return false;
    }

    qint64 frameSize = static_cast<qint64>(volData.width) * volData.height * volData.depth * bytesPerVoxel;
    if (data.size() < MRI_MGH_DATA_OFFSET + frameSize) {
        qCritical() << "MriMghIO::readVoxelData - File too small for expected data size";
        return false;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.device()->seek(MRI_MGH_DATA_OFFSET);

    int nslice = volData.depth;
    int nPixels = volData.width * volData.height;
    volData.slices.resize(nslice);

    // Build the vox2ras transform for per-slice transforms
    Matrix4f vox2ras = volData.computeVox2Ras();

    for (int k = 0; k < nslice; ++k) {
        MriSlice& slice = volData.slices[k];
        slice.width  = volData.width;
        slice.height = volData.height;
        slice.dimx   = volData.xsize / 1000.0f;  // mm -> meters
        slice.dimy   = volData.ysize / 1000.0f;

        // Read pixel data for this slice
        switch (volData.type) {
            case MRI_UCHAR: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_BYTE;
                slice.pixels.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    quint8 val;
                    stream >> val;
                    slice.pixels[p] = val;
                }
                slice.scale = 1.0f;
                break;
            }
            case MRI_SHORT: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_WORD;
                slice.pixelsWord.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    qint16 val;
                    stream >> val;
                    slice.pixelsWord[p] = static_cast<unsigned short>(val < 0 ? 0 : val);
                }
                slice.scale = 1.0f;
                break;
            }
            case MRI_INT: {
                // Convert INT to FLOAT
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                slice.pixelsFloat.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    qint32 val;
                    stream >> val;
                    slice.pixelsFloat[p] = static_cast<float>(val);
                }
                slice.scale = 1.0f;
                break;
            }
            case MRI_FLOAT: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                slice.pixelsFloat.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    float val;
                    stream >> val;
                    slice.pixelsFloat[p] = val;
                }
                slice.scale = 1.0f;
                break;
            }
        }

        //
        // Build per-slice coordinate transform (slice -> MRI surface RAS).
        // For each slice k:
        //   sliceOrigin = vox2ras * [0, 0, k, 1]^T
        //   sliceRot    = vox2ras rotation columns (x, y, z pixel axes)
        //
        Vector3f sliceOrigin;
        sliceOrigin(0) = vox2ras(0, 2) * k + vox2ras(0, 3);
        sliceOrigin(1) = vox2ras(1, 2) * k + vox2ras(1, 3);
        sliceOrigin(2) = vox2ras(2, 2) * k + vox2ras(2, 3);

        Matrix3f sliceRot;
        sliceRot.col(0) = vox2ras.block<3, 1>(0, 0);   // x-pixel direction
        sliceRot.col(1) = vox2ras.block<3, 1>(0, 1);   // y-pixel direction
        sliceRot.col(2) = vox2ras.block<3, 1>(0, 2);   // z (normal) direction

        Vector3f sliceMove;
        sliceMove << sliceOrigin(0), sliceOrigin(1), sliceOrigin(2);

        slice.trans = FiffCoordTrans(FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, sliceRot, sliceMove);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::parseFooter(const QByteArray& data,
                           MriVolData& volData,
                           QVector<FiffCoordTrans>& additionalTrans,
                           const QString& subjectMriDir,
                           bool verbose)
{
    //
    // The footer starts after the voxel data.
    // It may contain:
    //   1. Scan parameters: TR(f32), flipAngle(f32), TE(f32), TI(f32), FoV(f32)
    //   2. Tags: tagType(i32) + tagLen(i32 or i64) + tagData
    //

    int bytesPerVoxel;
    switch (volData.type) {
        case MRI_UCHAR: bytesPerVoxel = 1; break;
        case MRI_SHORT: bytesPerVoxel = 2; break;
        case MRI_INT:   bytesPerVoxel = 4; break;
        case MRI_FLOAT: bytesPerVoxel = 4; break;
        default: bytesPerVoxel = 1; break;
    }

    qint64 frameSize = static_cast<qint64>(volData.width) * volData.height * volData.depth * bytesPerVoxel;
    qint64 footerPos = MRI_MGH_DATA_OFFSET + frameSize;

    if (data.size() <= footerPos) {
        // No footer — that's fine
        return true;
    }

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.device()->seek(footerPos);

    // Parse tags
    while (!stream.atEnd()) {
        qint32 tagType;
        stream >> tagType;
        if (stream.atEnd()) break;

        qint64 tagLen;
        // For TAG_OLD_SURF_GEOM (20) and TAG_OLD_MGH_XFORM (30), length is 4 bytes
        // For newer tags, length is 8 bytes
        if (tagType == MGH_TAG_OLD_SURF_GEOM || tagType == MGH_TAG_OLD_MGH_XFORM) {
            qint32 len32;
            stream >> len32;
            tagLen = len32;
        } else {
            qint64 len64;
            stream >> len64;
            tagLen = len64;
        }

        if (tagLen <= 0 || tagLen > data.size()) break;

        QByteArray tagData(tagLen, '\0');
        if (stream.readRawData(tagData.data(), tagLen) != tagLen) break;

        if (tagType == MGH_TAG_MGH_XFORM) {
            // TAG_MGH_XFORM: contains path to talairach.xfm
            QString xfmPath = QString::fromLatin1(tagData).trimmed();
            volData.talairachXfmPath = xfmPath;

            if (verbose) {
                printf("Found Talairach transform reference: %s\n", qPrintable(xfmPath));
            }

            // Resolve relative paths using subject MRI directory
            if (!QFileInfo(xfmPath).isAbsolute() && !subjectMriDir.isEmpty()) {
                xfmPath = subjectMriDir + "/transforms/" + xfmPath;
            }

            if (QFileInfo::exists(xfmPath)) {
                QFile xfmFile(xfmPath);
                if (xfmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    // Parse Linear_Transform from .xfm file
                    // Format:
                    //   MNI Transform File
                    //   ...
                    //   Linear_Transform =
                    //   mat[0][0] mat[0][1] mat[0][2] mat[0][3]
                    //   mat[1][0] mat[1][1] mat[1][2] mat[1][3]
                    //   mat[2][0] mat[2][1] mat[2][2] mat[2][3] ;
                    QString xfmContent = xfmFile.readAll();
                    xfmFile.close();

                    int ltIdx = xfmContent.indexOf("Linear_Transform");
                    if (ltIdx >= 0) {
                        int eqIdx = xfmContent.indexOf('=', ltIdx);
                        if (eqIdx >= 0) {
                            QString matStr = xfmContent.mid(eqIdx + 1).trimmed();
                            matStr.remove(';');

                            QStringList vals = matStr.split(QRegularExpression("\\s+"),
                                                           Qt::SkipEmptyParts);

                            if (vals.size() >= 12) {
                                // RAS -> MNI Talairach (3×4 matrix, in mm)
                                Matrix4f rasMniTal = Matrix4f::Identity();
                                for (int r = 0; r < 3; ++r) {
                                    for (int c = 0; c < 4; ++c) {
                                        rasMniTal(r, c) = vals[r * 4 + c].toFloat();
                                    }
                                }

                                // Convert translation from mm to meters
                                rasMniTal(0, 3) /= 1000.0f;
                                rasMniTal(1, 3) /= 1000.0f;
                                rasMniTal(2, 3) /= 1000.0f;

                                // Create RAS -> MNI Talairach transform
                                FiffCoordTrans talTrans(
                                    FIFFV_COORD_MRI, FIFFV_COORD_MRI_DISPLAY,
                                    rasMniTal, true);

                                additionalTrans.append(talTrans);

                                if (verbose) {
                                    printf("Read Talairach transform from %s\n", qPrintable(xfmPath));
                                }
                            }
                        }
                    }
                }
            } else {
                if (verbose) {
                    printf("Talairach transform file not found: %s\n", qPrintable(xfmPath));
                }
            }
        }
    }

    return true;
}
