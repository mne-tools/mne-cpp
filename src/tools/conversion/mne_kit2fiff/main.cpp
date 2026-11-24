//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2006
 * @brief    Convert KIT/Ricoh (Yokogawa) .sqd/.con MEG data to FIFF format.
 *
 *           Reads KIT system data with an optional sensor layout file and
 *           HPI coil definitions for coordinate system alignment, then
 *           writes a FIFF raw file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/byte_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/endian.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

// KIT system constants
constexpr auto KIT_HEADER_SIZE = 16384; // Typical header size
constexpr auto KIT_SYSTEM_ID_OFFSET = 16;
constexpr auto KIT_NCHAN_OFFSET = 128;
constexpr auto KIT_SFREQ_OFFSET = 136;
constexpr auto KIT_NSAMP_OFFSET = 144;
constexpr auto KIT_DATA_OFFSET_OFFSET = 152; // offset to data start

//=============================================================================================================
// Structures
//=============================================================================================================

struct KitCoilDef
{
    Vector3d position;    // in mm (KIT device coords)
    Vector3d orientation; // unit normal
};

struct KitSensorInfo
{
    std::string name;
    int type; // 1=MEG, 2=ref, 3=trigger, 0=misc
    KitCoilDef coil;
    double gain;
};

struct KitDatasetInfo
{
    int nChannels;
    int nSamples;
    double sfreq;
    int dataOffset; // byte offset to data in sqd file
    int systemId;
    std::vector<KitSensorInfo> sensors;
};

//=============================================================================================================
// Little-endian file reader
//=============================================================================================================

/**
 * Reads little-endian values from a file; reads past the end give 0 and seeks past the end stay at the end.
 */
class KitLittleEndianReader
{
public:
    explicit KitLittleEndianReader(const std::string& path)
    : m_stream(mne::FileByteStream::open(mne::path_from_utf8(path), mne::FileByteStream::Mode::Read))
    {
    }

    bool isOpen() const
    {
        return m_stream != nullptr;
    }

    void seek(std::uint64_t position)
    {
        if (!m_stream->seek(position))
            m_stream->seek(m_stream->size());
    }

    template<typename T>
    T read()
    {
        T value{};
        return m_stream->read_little_endian(value) ? value : T{};
    }

private:
    std::unique_ptr<mne::FileByteStream> m_stream;
};

//=============================================================================================================
// Parse KIT .sqd header
//=============================================================================================================

static bool parseSqdHeader(const std::string& sqdPath, KitDatasetInfo& info)
{
    KitLittleEndianReader ds(sqdPath);
    if (!ds.isOpen()) {
        Log::critical("Cannot open SQD file: {}", sqdPath);
        return false;
    }

    // Read system ID
    ds.seek(KIT_SYSTEM_ID_OFFSET);
    info.systemId = ds.read<std::int32_t>();

    // Read number of channels
    ds.seek(KIT_NCHAN_OFFSET);
    info.nChannels = ds.read<std::int32_t>();

    // Read sample rate
    ds.seek(KIT_SFREQ_OFFSET);
    info.sfreq = ds.read<double>();

    // Read number of samples
    ds.seek(KIT_NSAMP_OFFSET);
    info.nSamples = ds.read<std::int32_t>();

    // Read data offset
    ds.seek(KIT_DATA_OFFSET_OFFSET);
    info.dataOffset = ds.read<std::int32_t>();
    if (info.dataOffset <= 0)
        info.dataOffset = KIT_HEADER_SIZE;

    Log::info("KIT dataset: system={}, {} channels, {} samples, {:.1f} Hz", info.systemId, info.nChannels, info.nSamples, info.sfreq);

    // Initialize sensors with default info
    for (int c = 0; c < info.nChannels; ++c) {
        KitSensorInfo sen;
        sen.name = std::format("MEG {:03}", c + 1);
        sen.type = (c < 160) ? 1 : 0; // First 160 are typically MEG
        sen.gain = 1.0;
        sen.coil.position = Vector3d::Zero();
        sen.coil.orientation = Vector3d(0, 0, 1);
        info.sensors.push_back(sen);
    }

    return true;
}

//=============================================================================================================
// Read sensor layout file (tab/space-delimited: idx x y z ox oy oz)
//=============================================================================================================

/** @return @p text as a number, 0 if it is none. */
static double kitToDouble(std::string_view text)
{
    return mne::to_double(text).value_or(0.0);
}

//=============================================================================================================

static bool readSensorLayout(const std::string& snsPath, KitDatasetInfo& info)
{
    std::ifstream in(mne::path_from_utf8(snsPath));
    if (!in) {
        Log::critical("Cannot open sensor file: {}", snsPath);
        return false;
    }

    int count = 0;

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#') || line.starts_with('%'))
            continue;

        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() < 7)
            continue;

        int idx = mne::to_int(parts[0]).value_or(0) - 1; // 1-based to 0-based
        if (idx < 0 || idx >= info.nChannels)
            continue;

        // Positions in mm, convert to m
        info.sensors[idx].coil.position(0) = kitToDouble(parts[1]) * 0.001;
        info.sensors[idx].coil.position(1) = kitToDouble(parts[2]) * 0.001;
        info.sensors[idx].coil.position(2) = kitToDouble(parts[3]) * 0.001;
        info.sensors[idx].coil.orientation(0) = kitToDouble(parts[4]);
        info.sensors[idx].coil.orientation(1) = kitToDouble(parts[5]);
        info.sensors[idx].coil.orientation(2) = kitToDouble(parts[6]);
        info.sensors[idx].type = 1; // MEG channel
        count++;
    }

    Log::info("Read {} sensor positions from {}", count, snsPath);
    return true;
}

//=============================================================================================================
// Read HPI coil positions and compute Procrustes alignment (device->head transform)
//=============================================================================================================

static bool computeDevHeadTransform(const std::string& hpiPath, FiffCoordTrans& trans)
{
    // HPI file format: pairs of device and head coordinate lines
    // Format: x y z (in mm) - first N lines are device coords, next N are head coords
    // Or alternating lines: device_x device_y device_z head_x head_y head_z
    std::ifstream in(mne::path_from_utf8(hpiPath));
    if (!in) {
        Log::info("Warning: Cannot open HPI file: {}", hpiPath);
        return false;
    }

    std::vector<Vector3d> devicePts;
    std::vector<Vector3d> headPts;

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;

        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() >= 6) {
            // Format: dev_x dev_y dev_z head_x head_y head_z
            Vector3d dev, head;
            dev(0) = kitToDouble(parts[0]) * 0.001; // mm to m
            dev(1) = kitToDouble(parts[1]) * 0.001;
            dev(2) = kitToDouble(parts[2]) * 0.001;
            head(0) = kitToDouble(parts[3]) * 0.001;
            head(1) = kitToDouble(parts[4]) * 0.001;
            head(2) = kitToDouble(parts[5]) * 0.001;
            devicePts.push_back(dev);
            headPts.push_back(head);
        } else if (parts.size() >= 3) {
            // Alternating device/head lines
            Vector3d pt;
            pt(0) = kitToDouble(parts[0]) * 0.001;
            pt(1) = kitToDouble(parts[1]) * 0.001;
            pt(2) = kitToDouble(parts[2]) * 0.001;
            if (devicePts.size() <= headPts.size())
                devicePts.push_back(pt);
            else
                headPts.push_back(pt);
        }
    }
    const int nPts = static_cast<int>(std::min(devicePts.size(), headPts.size()));
    if (nPts < 3) {
        Log::info("Warning: Need at least 3 HPI points, got {}", nPts);
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
    Log::info("HPI alignment: {} points, RMS error: {:.2f} mm", nPts, rms * 1000.0);

    return true;
}

//=============================================================================================================
// Read KIT .sqd data
//=============================================================================================================

static bool readSqdData(const std::string& sqdPath, const KitDatasetInfo& info, MatrixXd& data)
{
    KitLittleEndianReader ds(sqdPath);
    if (!ds.isOpen()) {
        Log::critical("Cannot open SQD: {}", sqdPath);
        return false;
    }

    ds.seek(static_cast<std::uint64_t>(info.dataOffset));

    int nChan = info.nChannels;
    int nSamp = info.nSamples;
    data.resize(nChan, nSamp);

    // KIT data is typically stored as 16-bit integers, channel-interleaved
    for (int s = 0; s < nSamp; ++s) {
        for (int c = 0; c < nChan; ++c) {
            const std::int16_t val = ds.read<std::int16_t>();
            data(c, s) = static_cast<double>(val) * info.sensors[c].gain;
        }
    }

    Log::info("Read {} channels x {} samples", nChan, nSamp);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert KIT/Ricoh .sqd/.con MEG data to FIFF format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"sqd"}, "Input KIT .sqd or .con file.", "file", {}});
    parser.add_option({{"sns"}, "Sensor layout definition file.", "file", {}});
    parser.add_option({{"hpi"}, "HPI coil positions in head coords.", "file", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.add_option({{"stim"}, "Comma-separated stimulus channel indices.", "list", {}});
    parser.process(argc, argv, "mne_kit2fiff", PROGRAM_VERSION);

    const std::string sqdFile = parser.value("sqd");
    const std::string snsFile = parser.value("sns");
    const std::string hpiFile = parser.value("hpi");
    const std::string outFile = parser.value("out");
    std::vector<int> stimChannels;
    if (parser.is_set("stim")) {
        for (const std::string& p : mne::split(parser.value("stim"), ','))
            stimChannels.push_back(mne::to_int(p).value_or(0));
    }

    if (sqdFile.empty()) {
        Log::critical("--sqd is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Parse SQD header
    KitDatasetInfo info;
    if (!parseSqdHeader(sqdFile, info))
        return 1;

    // Read optional sensor layout
    if (!snsFile.empty()) {
        if (!readSensorLayout(snsFile, info))
            return 1;
    }

    // Mark stimulus channels
    for (int idx : stimChannels) {
        if (idx >= 0 && idx < info.nChannels)
            info.sensors[idx].type = 3; // stimulus
    }

    // Compute device-to-head transform from HPI coils
    FiffCoordTrans devHeadTrans;
    devHeadTrans.from = FIFFV_COORD_DEVICE;
    devHeadTrans.to = FIFFV_COORD_HEAD;
    devHeadTrans.trans = Matrix4f::Identity();
    devHeadTrans.invtrans = Matrix4f::Identity();

    if (!hpiFile.empty())
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
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    fiffInfo.meas_date[0] = static_cast<fiff_int_t>(now.time_since_epoch().count());
    fiffInfo.meas_date[1] = 0;
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

        fiffInfo.chs.push_back(ch);
        fiffInfo.ch_names.push_back(ch.ch_name);
    }

    // Write FIFF
    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(mne::path_from_utf8(outFile), fiffInfo, cals);
    if (!stream) {
        Log::critical("Cannot open output: {}", outFile);
        return 1;
    }

    const int nSamples = static_cast<int>(data.cols());
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    Log::info("Written FIFF: {} ({} channels, {} samples, {:.1f} Hz)", outFile, nChan, nSamples, info.sfreq);

    return 0;
}
