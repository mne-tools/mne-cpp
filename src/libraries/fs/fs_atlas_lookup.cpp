//=============================================================================================================
/**
 * @file     fs_atlas_lookup.cpp
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
 *
 * @brief    FsAtlasLookup class definition — FreeSurfer volume parcellation atlas lookup.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_atlas_lookup.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QProcess>
#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsAtlasLookup::FsAtlasLookup()
    : m_ras2vox(Matrix4f::Identity())
{
    initLookupTable();
}

//=============================================================================================================

bool FsAtlasLookup::load(const QString& sParcellationPath)
{
    m_loaded = false;

    // Support .mgz (gzip-compressed MGH) and .mgh (uncompressed)
    // For .mgz, use QProcess to decompress, or read via gzip.
    // Here we support both by trying QFile directly (works for .mgh)
    // and using a pipe for .mgz.

    QByteArray data;

    if(sParcellationPath.endsWith(QStringLiteral(".mgz"), Qt::CaseInsensitive)) {
        // Decompress mgz via gzip
        QProcess gzipProc;
        gzipProc.start(QStringLiteral("gzip"), QStringList() << QStringLiteral("-dc") << sParcellationPath);
        if(!gzipProc.waitForFinished(30000)) {
            qWarning() << "[FsAtlasLookup::load] gzip decompression timed out for" << sParcellationPath;
            return false;
        }
        if(gzipProc.exitCode() != 0) {
            qWarning() << "[FsAtlasLookup::load] gzip decompression failed for" << sParcellationPath;
            return false;
        }
        data = gzipProc.readAllStandardOutput();
    } else {
        QFile file(sParcellationPath);
        if(!file.open(QIODevice::ReadOnly)) {
            qWarning() << "[FsAtlasLookup::load] Cannot open" << sParcellationPath;
            return false;
        }
        data = file.readAll();
        file.close();
    }

    if(data.size() < 284 + 64) { // Minimum header size
        qWarning() << "[FsAtlasLookup::load] File too small:" << sParcellationPath;
        return false;
    }

    // Parse MGH header (all big-endian)
    // Offset 0: version (int32) — must be 1
    // Offset 4: width (int32)
    // Offset 8: height (int32)
    // Offset 12: depth (int32)
    // Offset 16: nframes (int32)
    // Offset 20: type (int32) — 0=uchar, 1=int, 3=float, 4=short
    // Offset 284: vox2ras matrix (12 floats, row-major 3x4, then last row implicit [0,0,0,1])

    const char* raw = data.constData();

    auto readInt32 = [&](int offset) -> qint32 {
        return qFromBigEndian<qint32>(raw + offset);
    };

    auto readFloat32 = [&](int offset) -> float {
        return qFromBigEndian<qint32>(raw + offset);  // Read as int32 bits, reinterpret
    };

    qint32 version = readInt32(0);
    if(version != 1) {
        qWarning() << "[FsAtlasLookup::load] Unsupported MGH version:" << version;
        return false;
    }

    m_dimX = readInt32(4);
    m_dimY = readInt32(8);
    m_dimZ = readInt32(12);
    qint32 nframes = readInt32(16);
    qint32 type = readInt32(20);

    if(m_dimX <= 0 || m_dimY <= 0 || m_dimZ <= 0) {
        qWarning() << "[FsAtlasLookup::load] Invalid dimensions:" << m_dimX << m_dimY << m_dimZ;
        return false;
    }

    // Read vox2ras matrix at offset 284 (stored as 4x4 float32 big-endian starting with ras_good_flag at 28)
    // Actually MGH format: offset 284 has the vox2ras 4x4 as 16 big-endian floats
    // But first check ras_good_flag at offset 284 - 4*16 = no...
    // MGH header layout:
    //   0-3:   version (int32)
    //   4-7:   width
    //   8-11:  height
    //   12-15: depth
    //   16-19: nframes
    //   20-23: type
    //   24-27: dof
    //   28-29: ras_good_flag (short)
    //   30-41: x_size, y_size, z_size (3 floats — voxel sizes)
    //   42-77: x_ras, y_ras, z_ras (3x3 floats — direction cosines)
    //   78-89: c_ras (3 floats — center RAS)
    //   The actual data starts at offset 284.

    // Read ras_good_flag
    qint16 rasGoodFlag = qFromBigEndian<qint16>(raw + 28);

    Matrix4f vox2ras = Matrix4f::Identity();

    if(rasGoodFlag > 0) {
        // Read voxel sizes and direction cosines to construct vox2ras
        // Properly read floats by memcpy + endian swap
        auto readBEFloat = [&](int offset) -> float {
            quint32 bits = qFromBigEndian<quint32>(raw + offset);
            float val;
            memcpy(&val, &bits, sizeof(float));
            return val;
        };

        float xsize = readBEFloat(30);
        float ysize = readBEFloat(34);
        float zsize = readBEFloat(38);

        // Direction cosines (column vectors)
        Vector3f x_ras(readBEFloat(42), readBEFloat(46), readBEFloat(50));
        Vector3f y_ras(readBEFloat(54), readBEFloat(58), readBEFloat(62));
        Vector3f z_ras(readBEFloat(66), readBEFloat(70), readBEFloat(74));

        // Center RAS
        Vector3f c_ras(readBEFloat(78), readBEFloat(82), readBEFloat(86));

        // Construct vox2ras from direction cosines, voxel sizes, and center
        // vox2ras maps voxel (i,j,k) to RAS (x,y,z):
        //   M = [Mdc * diag(sizes), Pcrs_c] where Pcrs_c is computed from c_ras
        Matrix3f Mdc;
        Mdc.col(0) = x_ras;
        Mdc.col(1) = y_ras;
        Mdc.col(2) = z_ras;

        Matrix3f MdcD = Mdc;
        MdcD.col(0) *= xsize;
        MdcD.col(1) *= ysize;
        MdcD.col(2) *= zsize;

        // Center voxel
        Vector3f Pcrs_center(static_cast<float>(m_dimX) / 2.0f,
                             static_cast<float>(m_dimY) / 2.0f,
                             static_cast<float>(m_dimZ) / 2.0f);

        Vector3f Pxyz_0 = c_ras - MdcD * Pcrs_center;

        vox2ras.block<3,3>(0,0) = MdcD;
        vox2ras.block<3,1>(0,3) = Pxyz_0;
    }

    // Compute ras2vox = inverse(vox2ras)
    m_ras2vox = vox2ras.inverse();

    // Read voxel data starting at offset 284
    const int headerSize = 284;
    qint64 nVoxels = static_cast<qint64>(m_dimX) * m_dimY * m_dimZ * nframes;
    m_voxelData.resize(static_cast<int>(nVoxels));

    const char* voxPtr = raw + headerSize;
    qint64 dataSize = data.size() - headerSize;

    switch(type) {
    case 0: { // MRI_UCHAR
        if(dataSize < nVoxels) {
            qWarning() << "[FsAtlasLookup::load] Insufficient data for uchar volume";
            return false;
        }
        for(qint64 i = 0; i < nVoxels; ++i)
            m_voxelData[static_cast<int>(i)] = static_cast<int>(static_cast<unsigned char>(voxPtr[i]));
        break;
    }
    case 1: { // MRI_INT
        if(dataSize < nVoxels * 4) {
            qWarning() << "[FsAtlasLookup::load] Insufficient data for int volume";
            return false;
        }
        for(qint64 i = 0; i < nVoxels; ++i)
            m_voxelData[static_cast<int>(i)] = qFromBigEndian<qint32>(voxPtr + i * 4);
        break;
    }
    case 3: { // MRI_FLOAT
        if(dataSize < nVoxels * 4) {
            qWarning() << "[FsAtlasLookup::load] Insufficient data for float volume";
            return false;
        }
        for(qint64 i = 0; i < nVoxels; ++i) {
            quint32 bits = qFromBigEndian<quint32>(voxPtr + i * 4);
            float val;
            memcpy(&val, &bits, sizeof(float));
            m_voxelData[static_cast<int>(i)] = static_cast<int>(std::round(val));
        }
        break;
    }
    case 4: { // MRI_SHORT
        if(dataSize < nVoxels * 2) {
            qWarning() << "[FsAtlasLookup::load] Insufficient data for short volume";
            return false;
        }
        for(qint64 i = 0; i < nVoxels; ++i)
            m_voxelData[static_cast<int>(i)] = qFromBigEndian<qint16>(voxPtr + i * 2);
        break;
    }
    default:
        qWarning() << "[FsAtlasLookup::load] Unsupported MGH data type:" << type;
        return false;
    }

    m_loaded = true;
    return true;
}

//=============================================================================================================

QString FsAtlasLookup::labelAtRas(const Vector3f& ras) const
{
    if(!m_loaded)
        return QStringLiteral("Unknown");

    Vector3i vox = rasToVoxel(ras);

    // Bounds check
    if(vox(0) < 0 || vox(0) >= m_dimX ||
       vox(1) < 0 || vox(1) >= m_dimY ||
       vox(2) < 0 || vox(2) >= m_dimZ)
        return QStringLiteral("Unknown");

    // MGH stores data in column-major Fortran order: index = x + dimX * (y + dimY * z)
    int idx = vox(0) + m_dimX * (vox(1) + m_dimY * vox(2));
    int label = m_voxelData.at(idx);

    return m_lookupTable.value(label, QStringLiteral("Unknown"));
}

//=============================================================================================================

QStringList FsAtlasLookup::labelsForPositions(const QVector<Vector3f>& positions) const
{
    QStringList result;
    result.reserve(positions.size());
    for(const auto& pos : positions)
        result.append(labelAtRas(pos));
    return result;
}

//=============================================================================================================

bool FsAtlasLookup::isLoaded() const
{
    return m_loaded;
}

//=============================================================================================================

Vector3i FsAtlasLookup::rasToVoxel(const Vector3f& ras) const
{
    Vector4f rasH(ras(0), ras(1), ras(2), 1.0f);
    Vector4f voxH = m_ras2vox * rasH;
    return Vector3i(static_cast<int>(std::round(voxH(0))),
                    static_cast<int>(std::round(voxH(1))),
                    static_cast<int>(std::round(voxH(2))));
}

//=============================================================================================================

void FsAtlasLookup::initLookupTable()
{
    // Subcortical / standard FreeSurfer labels
    m_lookupTable[0]  = QStringLiteral("Unknown");
    m_lookupTable[2]  = QStringLiteral("Left-Cerebral-White-Matter");
    m_lookupTable[3]  = QStringLiteral("Left-Cerebral-Cortex");
    m_lookupTable[4]  = QStringLiteral("Left-Lateral-Ventricle");
    m_lookupTable[5]  = QStringLiteral("Left-Inf-Lat-Vent");
    m_lookupTable[7]  = QStringLiteral("Left-Cerebellum-White-Matter");
    m_lookupTable[8]  = QStringLiteral("Left-Cerebellum-Cortex");
    m_lookupTable[10] = QStringLiteral("Left-Thalamus");
    m_lookupTable[11] = QStringLiteral("Left-Caudate");
    m_lookupTable[12] = QStringLiteral("Left-Putamen");
    m_lookupTable[13] = QStringLiteral("Left-Pallidum");
    m_lookupTable[14] = QStringLiteral("3rd-Ventricle");
    m_lookupTable[15] = QStringLiteral("4th-Ventricle");
    m_lookupTable[16] = QStringLiteral("Brain-Stem");
    m_lookupTable[17] = QStringLiteral("Left-Hippocampus");
    m_lookupTable[18] = QStringLiteral("Left-Amygdala");
    m_lookupTable[24] = QStringLiteral("CSF");
    m_lookupTable[26] = QStringLiteral("Left-Accumbens-area");
    m_lookupTable[28] = QStringLiteral("Left-VentralDC");
    m_lookupTable[30] = QStringLiteral("Left-vessel");
    m_lookupTable[31] = QStringLiteral("Left-choroid-plexus");
    m_lookupTable[41] = QStringLiteral("Right-Cerebral-White-Matter");
    m_lookupTable[42] = QStringLiteral("Right-Cerebral-Cortex");
    m_lookupTable[43] = QStringLiteral("Right-Lateral-Ventricle");
    m_lookupTable[44] = QStringLiteral("Right-Inf-Lat-Vent");
    m_lookupTable[46] = QStringLiteral("Right-Cerebellum-White-Matter");
    m_lookupTable[47] = QStringLiteral("Right-Cerebellum-Cortex");
    m_lookupTable[49] = QStringLiteral("Right-Thalamus");
    m_lookupTable[50] = QStringLiteral("Right-Caudate");
    m_lookupTable[51] = QStringLiteral("Right-Putamen");
    m_lookupTable[52] = QStringLiteral("Right-Pallidum");
    m_lookupTable[53] = QStringLiteral("Right-Hippocampus");
    m_lookupTable[54] = QStringLiteral("Right-Amygdala");
    m_lookupTable[58] = QStringLiteral("Right-Accumbens-area");
    m_lookupTable[60] = QStringLiteral("Right-VentralDC");
    m_lookupTable[62] = QStringLiteral("Right-vessel");
    m_lookupTable[63] = QStringLiteral("Right-choroid-plexus");
    m_lookupTable[72] = QStringLiteral("5th-Ventricle");
    m_lookupTable[77] = QStringLiteral("WM-hypointensities");
    m_lookupTable[80] = QStringLiteral("non-WM-hypointensities");
    m_lookupTable[85] = QStringLiteral("Optic-Chiasm");
    m_lookupTable[251] = QStringLiteral("CC_Posterior");
    m_lookupTable[252] = QStringLiteral("CC_Mid_Posterior");
    m_lookupTable[253] = QStringLiteral("CC_Central");
    m_lookupTable[254] = QStringLiteral("CC_Mid_Anterior");
    m_lookupTable[255] = QStringLiteral("CC_Anterior");

    // Left hemisphere cortical parcellation (aparc, Desikan-Killiany atlas)
    m_lookupTable[1001] = QStringLiteral("ctx-lh-bankssts");
    m_lookupTable[1002] = QStringLiteral("ctx-lh-caudalanteriorcingulate");
    m_lookupTable[1003] = QStringLiteral("ctx-lh-caudalmiddlefrontal");
    m_lookupTable[1005] = QStringLiteral("ctx-lh-cuneus");
    m_lookupTable[1006] = QStringLiteral("ctx-lh-entorhinal");
    m_lookupTable[1007] = QStringLiteral("ctx-lh-fusiform");
    m_lookupTable[1008] = QStringLiteral("ctx-lh-inferiorparietal");
    m_lookupTable[1009] = QStringLiteral("ctx-lh-inferiortemporal");
    m_lookupTable[1010] = QStringLiteral("ctx-lh-isthmuscingulate");
    m_lookupTable[1011] = QStringLiteral("ctx-lh-lateraloccipital");
    m_lookupTable[1012] = QStringLiteral("ctx-lh-lateralorbitofrontal");
    m_lookupTable[1013] = QStringLiteral("ctx-lh-lingual");
    m_lookupTable[1014] = QStringLiteral("ctx-lh-medialorbitofrontal");
    m_lookupTable[1015] = QStringLiteral("ctx-lh-middletemporal");
    m_lookupTable[1016] = QStringLiteral("ctx-lh-parahippocampal");
    m_lookupTable[1017] = QStringLiteral("ctx-lh-paracentral");
    m_lookupTable[1018] = QStringLiteral("ctx-lh-parsopercularis");
    m_lookupTable[1019] = QStringLiteral("ctx-lh-parsorbitalis");
    m_lookupTable[1020] = QStringLiteral("ctx-lh-parstriangularis");
    m_lookupTable[1021] = QStringLiteral("ctx-lh-pericalcarine");
    m_lookupTable[1022] = QStringLiteral("ctx-lh-postcentral");
    m_lookupTable[1023] = QStringLiteral("ctx-lh-posteriorcingulate");
    m_lookupTable[1024] = QStringLiteral("ctx-lh-precentral");
    m_lookupTable[1025] = QStringLiteral("ctx-lh-precuneus");
    m_lookupTable[1026] = QStringLiteral("ctx-lh-rostralanteriorcingulate");
    m_lookupTable[1027] = QStringLiteral("ctx-lh-rostralmiddlefrontal");
    m_lookupTable[1028] = QStringLiteral("ctx-lh-superiorfrontal");
    m_lookupTable[1029] = QStringLiteral("ctx-lh-superiorparietal");
    m_lookupTable[1030] = QStringLiteral("ctx-lh-superiortemporal");
    m_lookupTable[1031] = QStringLiteral("ctx-lh-supramarginal");
    m_lookupTable[1032] = QStringLiteral("ctx-lh-frontalpole");
    m_lookupTable[1033] = QStringLiteral("ctx-lh-temporalpole");
    m_lookupTable[1034] = QStringLiteral("ctx-lh-transversetemporal");
    m_lookupTable[1035] = QStringLiteral("ctx-lh-insula");

    // Right hemisphere cortical parcellation (aparc, Desikan-Killiany atlas)
    m_lookupTable[2001] = QStringLiteral("ctx-rh-bankssts");
    m_lookupTable[2002] = QStringLiteral("ctx-rh-caudalanteriorcingulate");
    m_lookupTable[2003] = QStringLiteral("ctx-rh-caudalmiddlefrontal");
    m_lookupTable[2005] = QStringLiteral("ctx-rh-cuneus");
    m_lookupTable[2006] = QStringLiteral("ctx-rh-entorhinal");
    m_lookupTable[2007] = QStringLiteral("ctx-rh-fusiform");
    m_lookupTable[2008] = QStringLiteral("ctx-rh-inferiorparietal");
    m_lookupTable[2009] = QStringLiteral("ctx-rh-inferiortemporal");
    m_lookupTable[2010] = QStringLiteral("ctx-rh-isthmuscingulate");
    m_lookupTable[2011] = QStringLiteral("ctx-rh-lateraloccipital");
    m_lookupTable[2012] = QStringLiteral("ctx-rh-lateralorbitofrontal");
    m_lookupTable[2013] = QStringLiteral("ctx-rh-lingual");
    m_lookupTable[2014] = QStringLiteral("ctx-rh-medialorbitofrontal");
    m_lookupTable[2015] = QStringLiteral("ctx-rh-middletemporal");
    m_lookupTable[2016] = QStringLiteral("ctx-rh-parahippocampal");
    m_lookupTable[2017] = QStringLiteral("ctx-rh-paracentral");
    m_lookupTable[2018] = QStringLiteral("ctx-rh-parsopercularis");
    m_lookupTable[2019] = QStringLiteral("ctx-rh-parsorbitalis");
    m_lookupTable[2020] = QStringLiteral("ctx-rh-parstriangularis");
    m_lookupTable[2021] = QStringLiteral("ctx-rh-pericalcarine");
    m_lookupTable[2022] = QStringLiteral("ctx-rh-postcentral");
    m_lookupTable[2023] = QStringLiteral("ctx-rh-posteriorcingulate");
    m_lookupTable[2024] = QStringLiteral("ctx-rh-precentral");
    m_lookupTable[2025] = QStringLiteral("ctx-rh-precuneus");
    m_lookupTable[2026] = QStringLiteral("ctx-rh-rostralanteriorcingulate");
    m_lookupTable[2027] = QStringLiteral("ctx-rh-rostralmiddlefrontal");
    m_lookupTable[2028] = QStringLiteral("ctx-rh-superiorfrontal");
    m_lookupTable[2029] = QStringLiteral("ctx-rh-superiorparietal");
    m_lookupTable[2030] = QStringLiteral("ctx-rh-superiortemporal");
    m_lookupTable[2031] = QStringLiteral("ctx-rh-supramarginal");
    m_lookupTable[2032] = QStringLiteral("ctx-rh-frontalpole");
    m_lookupTable[2033] = QStringLiteral("ctx-rh-temporalpole");
    m_lookupTable[2034] = QStringLiteral("ctx-rh-transversetemporal");
    m_lookupTable[2035] = QStringLiteral("ctx-rh-insula");
}
