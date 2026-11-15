//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_atlas_lookup.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::FsAtlasLookup: RAS → region-name resolver over a FreeSurfer volumetric parcellation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_atlas_lookup.h"
#include <base/log.h>

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/byte_stream.h>
#include <base/endian.h>
#include <base/string_utils.h>

#include <Eigen/LU>


#include <cmath>
#include <cstring>
#include <unordered_map>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

const std::string& unknown()
{
    static const std::string name = "Unknown";
    return name;
}

// Label index -> region name, after $FREESURFER_HOME/FreeSurferColorLUT.txt
const std::unordered_map<int, std::string>& lookupTable()
{
    static const std::unordered_map<int, std::string> table{
        // Subcortical / standard FreeSurfer labels
        {0, "Unknown"},
        {2, "Left-Cerebral-White-Matter"},
        {3, "Left-Cerebral-Cortex"},
        {4, "Left-Lateral-Ventricle"},
        {5, "Left-Inf-Lat-Vent"},
        {7, "Left-Cerebellum-White-Matter"},
        {8, "Left-Cerebellum-Cortex"},
        {10, "Left-Thalamus"},
        {11, "Left-Caudate"},
        {12, "Left-Putamen"},
        {13, "Left-Pallidum"},
        {14, "3rd-Ventricle"},
        {15, "4th-Ventricle"},
        {16, "Brain-Stem"},
        {17, "Left-Hippocampus"},
        {18, "Left-Amygdala"},
        {24, "CSF"},
        {26, "Left-Accumbens-area"},
        {28, "Left-VentralDC"},
        {30, "Left-vessel"},
        {31, "Left-choroid-plexus"},
        {41, "Right-Cerebral-White-Matter"},
        {42, "Right-Cerebral-Cortex"},
        {43, "Right-Lateral-Ventricle"},
        {44, "Right-Inf-Lat-Vent"},
        {46, "Right-Cerebellum-White-Matter"},
        {47, "Right-Cerebellum-Cortex"},
        {49, "Right-Thalamus"},
        {50, "Right-Caudate"},
        {51, "Right-Putamen"},
        {52, "Right-Pallidum"},
        {53, "Right-Hippocampus"},
        {54, "Right-Amygdala"},
        {58, "Right-Accumbens-area"},
        {60, "Right-VentralDC"},
        {62, "Right-vessel"},
        {63, "Right-choroid-plexus"},
        {72, "5th-Ventricle"},
        {77, "WM-hypointensities"},
        {80, "non-WM-hypointensities"},
        {85, "Optic-Chiasm"},
        {251, "CC_Posterior"},
        {252, "CC_Mid_Posterior"},
        {253, "CC_Central"},
        {254, "CC_Mid_Anterior"},
        {255, "CC_Anterior"},
        // Left hemisphere cortical parcellation (aparc, Desikan-Killiany atlas)
        {1001, "ctx-lh-bankssts"},
        {1002, "ctx-lh-caudalanteriorcingulate"},
        {1003, "ctx-lh-caudalmiddlefrontal"},
        {1005, "ctx-lh-cuneus"},
        {1006, "ctx-lh-entorhinal"},
        {1007, "ctx-lh-fusiform"},
        {1008, "ctx-lh-inferiorparietal"},
        {1009, "ctx-lh-inferiortemporal"},
        {1010, "ctx-lh-isthmuscingulate"},
        {1011, "ctx-lh-lateraloccipital"},
        {1012, "ctx-lh-lateralorbitofrontal"},
        {1013, "ctx-lh-lingual"},
        {1014, "ctx-lh-medialorbitofrontal"},
        {1015, "ctx-lh-middletemporal"},
        {1016, "ctx-lh-parahippocampal"},
        {1017, "ctx-lh-paracentral"},
        {1018, "ctx-lh-parsopercularis"},
        {1019, "ctx-lh-parsorbitalis"},
        {1020, "ctx-lh-parstriangularis"},
        {1021, "ctx-lh-pericalcarine"},
        {1022, "ctx-lh-postcentral"},
        {1023, "ctx-lh-posteriorcingulate"},
        {1024, "ctx-lh-precentral"},
        {1025, "ctx-lh-precuneus"},
        {1026, "ctx-lh-rostralanteriorcingulate"},
        {1027, "ctx-lh-rostralmiddlefrontal"},
        {1028, "ctx-lh-superiorfrontal"},
        {1029, "ctx-lh-superiorparietal"},
        {1030, "ctx-lh-superiortemporal"},
        {1031, "ctx-lh-supramarginal"},
        {1032, "ctx-lh-frontalpole"},
        {1033, "ctx-lh-temporalpole"},
        {1034, "ctx-lh-transversetemporal"},
        {1035, "ctx-lh-insula"},
        // Right hemisphere cortical parcellation (aparc, Desikan-Killiany atlas)
        {2001, "ctx-rh-bankssts"},
        {2002, "ctx-rh-caudalanteriorcingulate"},
        {2003, "ctx-rh-caudalmiddlefrontal"},
        {2005, "ctx-rh-cuneus"},
        {2006, "ctx-rh-entorhinal"},
        {2007, "ctx-rh-fusiform"},
        {2008, "ctx-rh-inferiorparietal"},
        {2009, "ctx-rh-inferiortemporal"},
        {2010, "ctx-rh-isthmuscingulate"},
        {2011, "ctx-rh-lateraloccipital"},
        {2012, "ctx-rh-lateralorbitofrontal"},
        {2013, "ctx-rh-lingual"},
        {2014, "ctx-rh-medialorbitofrontal"},
        {2015, "ctx-rh-middletemporal"},
        {2016, "ctx-rh-parahippocampal"},
        {2017, "ctx-rh-paracentral"},
        {2018, "ctx-rh-parsopercularis"},
        {2019, "ctx-rh-parsorbitalis"},
        {2020, "ctx-rh-parstriangularis"},
        {2021, "ctx-rh-pericalcarine"},
        {2022, "ctx-rh-postcentral"},
        {2023, "ctx-rh-posteriorcingulate"},
        {2024, "ctx-rh-precentral"},
        {2025, "ctx-rh-precuneus"},
        {2026, "ctx-rh-rostralanteriorcingulate"},
        {2027, "ctx-rh-rostralmiddlefrontal"},
        {2028, "ctx-rh-superiorfrontal"},
        {2029, "ctx-rh-superiorparietal"},
        {2030, "ctx-rh-superiortemporal"},
        {2031, "ctx-rh-supramarginal"},
        {2032, "ctx-rh-frontalpole"},
        {2033, "ctx-rh-temporalpole"},
        {2034, "ctx-rh-transversetemporal"},
        {2035, "ctx-rh-insula"},
    };
    return table;
}

template<mne::Swappable T>
T readBigEndian(const std::byte* data)
{
    T value;
    std::memcpy(&value, data, sizeof(T));
    return mne::fromBigEndian(value);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool FsAtlasLookup::load(const std::filesystem::path& parcellation)
{
    m_loaded = false;

    const bool gzip = mne::toLower(mne::utf8FromPath(parcellation.extension())) == ".mgz";
    const auto bytes = mne::read_file_bytes(parcellation, gzip ? mne::Compression::Gzip : mne::Compression::None);
    if (!bytes) {
        mne::Log::warning("[FsAtlasLookup::load] {}", bytes.error().what());
        return false;
    }

    // MGH header, all big-endian: version, width, height, depth, nframes, type, dof (int32), ras_good_flag
    // (int16), voxel sizes, x/y/z direction cosines and centre RAS (float32); the voxels start at byte 284.
    constexpr std::size_t kHeaderSize = 284;
    if (bytes->size() < kHeaderSize) {
        mne::Log::warning("[FsAtlasLookup::load] File too small: {}", mne::utf8FromPath(parcellation));
        return false;
    }
    const std::byte* raw = bytes->data();
    if (readBigEndian<std::int32_t>(raw) != 1) {
        mne::Log::warning("[FsAtlasLookup::load] Unsupported MGH version: {}", readBigEndian<std::int32_t>(raw));
        return false;
    }
    m_dimX = readBigEndian<std::int32_t>(raw + 4);
    m_dimY = readBigEndian<std::int32_t>(raw + 8);
    m_dimZ = readBigEndian<std::int32_t>(raw + 12);
    const std::int32_t type = readBigEndian<std::int32_t>(raw + 20);
    if (m_dimX <= 0 || m_dimY <= 0 || m_dimZ <= 0 || readBigEndian<std::int32_t>(raw + 16) <= 0) {
        mne::Log::warning("[FsAtlasLookup::load] Invalid dimensions: {} {} {}", m_dimX, m_dimY, m_dimZ);
        return false;
    }

    Matrix4f vox2ras = Matrix4f::Identity();
    if (readBigEndian<std::int16_t>(raw + 28) > 0) {
        const auto f = [raw](std::size_t offset) {
            return readBigEndian<float>(raw + offset);
        };
        Matrix3f MdcD;
        MdcD << f(42), f(54), f(66),
            f(46), f(58), f(70),
            f(50), f(62), f(74);
        MdcD.col(0) *= f(30);
        MdcD.col(1) *= f(34);
        MdcD.col(2) *= f(38);
        const Vector3f center(static_cast<float>(m_dimX) / 2.0f, static_cast<float>(m_dimY) / 2.0f, static_cast<float>(m_dimZ) / 2.0f);
        vox2ras.block<3, 3>(0, 0) = MdcD;
        vox2ras.block<3, 1>(0, 3) = Vector3f(f(78), f(82), f(86)) - MdcD * center;
    }
    m_ras2vox = vox2ras.inverse();

    // Only the first frame holds labels
    const std::size_t nVoxels = std::size_t(m_dimX) * std::size_t(m_dimY) * std::size_t(m_dimZ);
    const std::size_t width = type == 0 ? 1 : type == 4 ? 2
        : (type == 1 || type == 3)                      ? 4
                                                        : 0;
    if (width == 0) {
        mne::Log::warning("[FsAtlasLookup::load] Unsupported MGH data type: {}", type);
        return false;
    }
    if ((bytes->size() - kHeaderSize) / width < nVoxels) {
        mne::Log::warning("[FsAtlasLookup::load] Truncated voxel data in {}", mne::utf8FromPath(parcellation));
        return false;
    }
    const std::byte* voxels = raw + kHeaderSize;
    m_voxelData.resize(nVoxels);
    for (std::size_t i = 0; i < nVoxels; ++i) {
        switch (type) {
            case 0:
                m_voxelData[i] = std::to_integer<int>(voxels[i]);
                break;
            case 1:
                m_voxelData[i] = readBigEndian<std::int32_t>(voxels + 4 * i);
                break;
            case 3:
                m_voxelData[i] = static_cast<int>(std::lround(readBigEndian<float>(voxels + 4 * i)));
                break;
            default:
                m_voxelData[i] = readBigEndian<std::int16_t>(voxels + 2 * i);
                break;
        }
    }

    m_loaded = true;
    return true;
}

//=============================================================================================================

const std::string& FsAtlasLookup::label_at_ras(const Vector3f& ras) const
{
    if (!m_loaded) {
        return unknown();
    }
    const Vector3i vox = ras_to_voxel(ras);
    if (vox(0) < 0 || vox(0) >= m_dimX || vox(1) < 0 || vox(1) >= m_dimY || vox(2) < 0 || vox(2) >= m_dimZ) {
        return unknown();
    }
    // MGH stores the voxels in column-major order: index = x + dimX * (y + dimY * z)
    const std::size_t index = std::size_t(vox(0)) + std::size_t(m_dimX) * (std::size_t(vox(1)) + std::size_t(m_dimY) * std::size_t(vox(2)));
    const auto& table = lookupTable();
    const auto it = table.find(m_voxelData[index]);
    return it == table.end() ? unknown() : it->second;
}

//=============================================================================================================

std::vector<std::string> FsAtlasLookup::labels_for_positions(std::span<const Vector3f> positions) const
{
    std::vector<std::string> result;
    result.reserve(positions.size());
    for (const Vector3f& pos : positions) {
        result.push_back(label_at_ras(pos));
    }
    return result;
}

//=============================================================================================================

bool FsAtlasLookup::is_loaded() const
{
    return m_loaded;
}

//=============================================================================================================

Vector3i FsAtlasLookup::ras_to_voxel(const Vector3f& ras) const
{
    const Vector4f vox = m_ras2vox * Vector4f(ras(0), ras(1), ras(2), 1.0f);
    return Vector3i(static_cast<int>(std::round(vox(0))), static_cast<int>(std::round(vox(1))), static_cast<int>(std::round(vox(2))));
}
