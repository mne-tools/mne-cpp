//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     April 2006
 * @brief    Implements mne_make_cor_set: create MRI description FIFF from FreeSurfer data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/command_line.h>
#include <mne/base/endian.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

constexpr auto COR_NSLICE = 256;
constexpr auto COR_NPIX = 256;
#define COR_SLICE_SIZE (COR_NPIX * COR_NPIX)

//=============================================================================================================

/**
 * Reads big-endian values from memory; reads past the end give 0.
 */
class CorBigEndianCursor
{
public:
    explicit CorBigEndianCursor(const std::vector<char>& data)
    : m_data(data)
    {
    }

    template<typename T>
    T read()
    {
        T value{};
        if (m_pos + sizeof(T) > m_data.size()) {
            m_pos = m_data.size();
            return T{};
        }
        std::memcpy(&value, m_data.data() + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return mne::from_big_endian(value);
    }

    void readRaw(char* destination, std::size_t bytes)
    {
        const std::size_t n = std::min(bytes, m_data.size() - m_pos);
        std::memcpy(destination, m_data.data() + m_pos, n);
        m_pos += n;
    }

    void skip(std::size_t bytes)
    {
        m_pos += std::min(bytes, m_data.size() - m_pos);
    }

private:
    const std::vector<char>& m_data;
    std::size_t m_pos = 0;
};

//=============================================================================================================

/** @return @p value limited to [@p low, @p high]; NaN gives @p low. */
template<typename T>
static T corBound(T low, T value, T high)
{
    const T upper = (high < value) ? high : value;
    return (low < upper) ? upper : low;
}

//=============================================================================================================

static bool readCorDirectory(const std::string& dirPath, std::vector<std::vector<unsigned char>>& slices)
{
    const std::filesystem::path dir = mne::path_from_utf8(dirPath);
    slices.resize(COR_NSLICE);

    for (int k = 0; k < COR_NSLICE; k++) {
        const std::string fname = mne::utf8_from_path(dir / std::format("COR-{:03}", k + 1));
        std::ifstream file(mne::path_from_utf8(fname), std::ios::binary);
        if (!file) {
            Log::critical("Cannot open COR file: {}", fname);
            return false;
        }
        const std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (std::ssize(data) != COR_SLICE_SIZE) {
            Log::critical("Unexpected COR slice size: {} expected {}", data.size(), COR_SLICE_SIZE);
            return false;
        }
        slices[k].assign(data.begin(), data.end());
    }
    Log::info("Read {} COR slices from {}", COR_NSLICE, dirPath);
    return true;
}

//=============================================================================================================

static bool writeMriDescription(const std::string& filename,
                                const std::vector<std::vector<unsigned char>>& slices,
                                int width, int height, int nslice)
{
    auto stream = FiffStream::start_file(mne::path_from_utf8(filename));
    if (!stream) {
        Log::critical("Cannot create output FIFF file: {}", filename);
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
        // Write as FIFF_MRI_PIXEL_DATA
        FiffTag tag;
        tag.kind = FIFF_MRI_PIXEL_DATA;
        tag.type = FIFFT_BYTE;
        tag.resize(slices[k].size());
        if (!slices[k].empty())
            std::memcpy(tag.data(), slices[k].data(), slices[k].size());
        stream->write_tag(tag);
    }

    stream->end_block(FIFFB_MRI);
    stream->end_file();

    Log::info("Wrote MRI description to {} ({}x{}x{})", filename, width, height, nslice);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Create MRI description FIFF from FreeSurfer COR or mgh/mgz data.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"dir"}, "FreeSurfer COR directory.", "directory", {}});
    parser.add_option({{"mgh"}, "FreeSurfer mgh/mgz volume file.", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_make_cor_set", PROGRAM_VERSION);

    const std::string corDir = parser.value("dir");
    const std::string mghFile = parser.value("mgh");
    const std::string outFile = parser.value("out");

    if (corDir.empty() && mghFile.empty()) {
        Log::info("Either --dir or --mgh must be specified.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    std::vector<std::vector<unsigned char>> slices;
    int width = COR_NPIX, height = COR_NPIX, nslice = COR_NSLICE;

    if (!corDir.empty()) {
        if (!readCorDirectory(corDir, slices))
            return 1;
    } else if (!mghFile.empty()) {
        // Read mgh/mgz file
        std::ifstream file(mne::path_from_utf8(mghFile), std::ios::binary);
        bool isGz = mghFile.ends_with(".mgz") || mghFile.ends_with(".mgh.gz");

        if (!file) {
            Log::critical("Cannot open mgh file: {}", mghFile);
            return 1;
        }

        const std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // The tool only ever decompressed zlib streams with a Qt length prefix, which gzipped mgz files are not
        if (isGz) {
            Log::critical("Failed to decompress mgz file. Try using an uncompressed .mgh file.");
            return 1;
        }

        // Parse MGH header (big-endian)
        CorBigEndianCursor in(data);

        const std::int32_t version = in.read<std::int32_t>();
        const std::int32_t w = in.read<std::int32_t>();
        const std::int32_t h = in.read<std::int32_t>();
        const std::int32_t d = in.read<std::int32_t>();
        [[maybe_unused]] const std::int32_t nframes = in.read<std::int32_t>();
        const std::int32_t type = in.read<std::int32_t>();

        if (version != 1) {
            Log::info("Unsupported MGH version: {}", version);
            return 1;
        }

        width = w;
        height = h;
        nslice = d;

        // Skip rest of header (284 bytes total - 24 already read)
        in.skip(284 - 24);

        Log::info("Read MGH: {}x{}x{}, type={}", width, height, nslice, type);

        slices.resize(nslice);
        int sliceSize = width * height;

        for (int k = 0; k < nslice; k++) {
            slices[k].resize(sliceSize);
            if (type == 0) { // MRI_UCHAR
                in.readRaw(reinterpret_cast<char*>(slices[k].data()), static_cast<std::size_t>(sliceSize));
            } else if (type == 1) { // MRI_INT
                for (int j = 0; j < sliceSize; j++) {
                    const std::int32_t val = in.read<std::int32_t>();
                    slices[k][j] = static_cast<unsigned char>(corBound(0, val, 255));
                }
            } else if (type == 3) { // MRI_FLOAT
                for (int j = 0; j < sliceSize; j++) {
                    // Read as 8-byte doubles, as the tool always has
                    const float val = static_cast<float>(in.read<double>());
                    slices[k][j] = static_cast<unsigned char>(corBound(0.0f, val, 255.0f));
                }
            } else if (type == 4) { // MRI_SHORT
                for (int j = 0; j < sliceSize; j++) {
                    const std::int16_t val = in.read<std::int16_t>();
                    slices[k][j] = static_cast<unsigned char>(corBound(0, static_cast<int>(val), 255));
                }
            } else {
                Log::info("Unsupported MGH data type: {}", type);
                return 1;
            }
        }
    }

    return writeMriDescription(outFile, slices, width, height, nslice) ? 0 : 1;
}
