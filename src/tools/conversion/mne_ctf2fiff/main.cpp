//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2005
 * @brief    Convert CTF .ds MEG datasets to FIFF format.
 *
 *           Reads the CTF .ds directory structure (res4 header + meg4 data)
 *           and writes a standard FIFF raw file.
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
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
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

// CTF sensor types (from res4 file)
constexpr auto CTF_SEN_TYPE_MEG = 5;      // MEG magnetometer/gradiometer
constexpr auto CTF_SEN_TYPE_REF_MAG = 0;  // Reference magnetometer
constexpr auto CTF_SEN_TYPE_REF_GRAD = 1; // Reference gradiometer
constexpr auto CTF_SEN_TYPE_EEG = 9;      // EEG channel
constexpr auto CTF_SEN_TYPE_STIM = 11;    // Stimulus/trigger

// CTF res4 structure sizes
constexpr auto CTF_RES4_HEADER_SIZE = 1844;
constexpr auto CTF_SENSOR_RECORD_SIZE = 1328;
constexpr auto CTF_SENSOR_NAME_SIZE = 32;
constexpr auto CTF_MAX_COILS = 8;

//=============================================================================================================
// CTF data structures
//=============================================================================================================

struct CtfCoilPos
{
    double x, y, z;    // position in m
    double ox, oy, oz; // orientation
    double area;       // coil area in m^2
};

struct CtfSensorInfo
{
    char name[CTF_SENSOR_NAME_SIZE];
    int sensorType;
    int nCoils;
    CtfCoilPos coils[CTF_MAX_COILS];
    double properGain; // sensor gain
    double qGain;      // electronics gain
    double ioGain;     // I/O unit gain
    double ioOffset;
};

struct CtfDatasetInfo
{
    std::string dsPath;
    std::string res4Path;
    std::string meg4Path;

    int nChannels;
    int nSamples; // total samples per channel per trial
    int nTrials;
    double sfreq;
    int dataFormat; // 1=short, 4=int, 8=double

    std::vector<CtfSensorInfo> sensors;
};

//=============================================================================================================
// Big-endian file reader
//=============================================================================================================

/**
 * Reads big-endian values from a file; reads past the end give 0 and seeks past the end stay at the end.
 */
class CtfBigEndianReader
{
public:
    explicit CtfBigEndianReader(const std::string& path)
    : m_stream(mne::FileByteStream::open(mne::path_from_utf8(path), mne::FileByteStream::Mode::Read))
    {
    }

    bool isOpen() const
    {
        return m_stream != nullptr;
    }

    std::uint64_t pos() const
    {
        return m_stream->pos();
    }

    void seek(std::uint64_t position)
    {
        if (!m_stream->seek(position))
            m_stream->seek(m_stream->size());
    }

    void skip(std::uint64_t bytes)
    {
        seek(pos() + bytes);
    }

    void readRaw(char* data, std::size_t bytes)
    {
        m_stream->read(data, bytes);
    }

    template<typename T>
    T read()
    {
        T value{};
        if (!m_stream->read_big_endian(value))
            value = T{};
        return value;
    }

private:
    std::unique_ptr<mne::FileByteStream> m_stream;
};

//=============================================================================================================
// Parse CTF res4 header
//=============================================================================================================

static bool parseRes4(CtfDatasetInfo& info)
{
    CtfBigEndianReader ds(info.res4Path);
    if (!ds.isOpen()) {
        Log::critical("Cannot open res4: {}", info.res4Path);
        return false;
    }

    // Skip header identifier (8 bytes: "MEG41RS" or "MEG42RS")
    char headerTag[9] = {0};
    ds.readRaw(headerTag, 8);
    Log::info("Res4 tag: {}", headerTag);

    // Skip appName (256 bytes), DataOrigin (256 bytes), DataDescription (256 bytes)
    ds.skip(256 + 256 + 256);

    // General header fields
    info.nChannels = ds.read<std::int16_t>();

    // Skip some reserved bytes (2 bytes)
    ds.skip(2);

    // Read sample rate and sample count
    // After the padding, at offset 1844 - some_offset
    // The res4 layout varies by version, but typically:
    // Offset 780: nSamples (int32)
    // Offset 784: nTrials (int16)
    // Offset 796: sampleRate (double)
    ds.seek(776);
    info.nSamples = ds.read<std::int32_t>();
    info.nTrials = ds.read<std::int16_t>();

    ds.skip(6); // padding to align to 8-byte boundary

    // Skip to sample rate position
    ds.seek(788);
    info.sfreq = ds.read<double>();

    // Read data time (epoch length in seconds)
    double epochTime = ds.read<double>();

    // Skip to number of samples if still 0
    if (info.nSamples <= 0 || info.sfreq <= 0) {
        // Try alternative approach: compute from epoch time
        if (info.sfreq > 0 && epochTime > 0)
            info.nSamples = static_cast<int>(epochTime * info.sfreq);
    }

    Log::info("Res4: {} channels, {} samples/trial, {} trials, {:.1f} Hz", info.nChannels, info.nSamples, info.nTrials, info.sfreq);

    // Seek to sensor records (at fixed offset after general header)
    ds.seek(CTF_RES4_HEADER_SIZE);

    // Read sensor information records
    for (int c = 0; c < info.nChannels; ++c) {
        const std::uint64_t startPos = ds.pos();

        CtfSensorInfo sen;
        memset(&sen, 0, sizeof(sen));

        // Sensor name (32 bytes)
        ds.readRaw(sen.name, CTF_SENSOR_NAME_SIZE);

        // Sensor type (2 bytes)
        sen.sensorType = ds.read<std::int16_t>();

        // Skip to gains (fixed offsets within sensor record)
        // original_run_no(2), ctfCoilType(4), reserved(4)
        ds.skip(2 + 4 + 4);

        // properGain, qGain, ioGain, ioOffset (each 8 bytes)
        sen.properGain = ds.read<double>();
        sen.qGain = ds.read<double>();
        sen.ioGain = ds.read<double>();
        sen.ioOffset = ds.read<double>();

        // Number of coils (2 bytes)
        const std::int16_t nCoils = ds.read<std::int16_t>();
        sen.nCoils = std::min(static_cast<int>(nCoils), CTF_MAX_COILS);

        ds.skip(2); // padding

        // Read coil positions
        for (int j = 0; j < sen.nCoils; ++j) {
            // CTF coordinates are in cm, convert to m
            sen.coils[j].x = ds.read<double>() * 0.01;
            sen.coils[j].y = ds.read<double>() * 0.01;
            sen.coils[j].z = ds.read<double>() * 0.01;
            sen.coils[j].ox = ds.read<double>();
            sen.coils[j].oy = ds.read<double>();
            sen.coils[j].oz = ds.read<double>();
            sen.coils[j].area = ds.read<double>() * 1e-4; // cm^2 to m^2
        }

        info.sensors.push_back(sen);

        // Skip to next sensor record
        ds.seek(startPos + CTF_SENSOR_RECORD_SIZE);
    }

    return true;
}

//=============================================================================================================
// Read CTF meg4 data
//=============================================================================================================

static bool readMeg4Data(const CtfDatasetInfo& info, MatrixXd& data)
{
    CtfBigEndianReader ds(info.meg4Path);
    if (!ds.isOpen()) {
        Log::critical("Cannot open meg4: {}", info.meg4Path);
        return false;
    }

    // Skip meg4 header (8 bytes: "MEG41CP" or similar)
    ds.seek(8);

    int nChan = info.nChannels;
    int totalSamples = info.nSamples * info.nTrials;
    data.resize(nChan, totalSamples);

    // Read trial by trial
    for (int trial = 0; trial < info.nTrials; ++trial) {
        int colOffset = trial * info.nSamples;
        // Data is stored channel-major within each trial
        for (int c = 0; c < nChan; ++c) {
            double gainFactor = 1.0;
            const CtfSensorInfo& sen = info.sensors[c];

            // Total gain: properGain * qGain * ioGain
            if (sen.properGain != 0.0 && sen.qGain != 0.0 && sen.ioGain != 0.0)
                gainFactor = 1.0 / (sen.properGain * sen.qGain * sen.ioGain);

            for (int s = 0; s < info.nSamples; ++s) {
                double val = 0.0;
                // CTF datasets typically use 32-bit integers
                const std::int32_t ival = ds.read<std::int32_t>();
                val = static_cast<double>(ival) * gainFactor;
                data(c, colOffset + s) = val;
            }
        }
    }

    Log::info("Read {} channels x {} samples", nChan, totalSamples);
    return true;
}

//=============================================================================================================
// Map CTF sensor type to FIFF channel kind
//=============================================================================================================

static void mapCtfToFiff(const CtfSensorInfo& sen, FiffChInfo& ch)
{
    ch.ch_name = mne::latin1_to_utf8(std::string_view(sen.name, strnlen(sen.name, CTF_SENSOR_NAME_SIZE)));

    switch (sen.sensorType) {
        case CTF_SEN_TYPE_MEG:
            ch.kind = FIFFV_MEG_CH;
            // CTF axial gradiometers have 2 coils, magnetometers have 1
            if (sen.nCoils >= 2)
                ch.chpos.coil_type = FIFFV_COIL_CTF_GRAD;
            else
                ch.chpos.coil_type = FIFFV_COIL_CTF_REF_MAG;
            ch.unit = FIFF_UNIT_T;
            break;

        case CTF_SEN_TYPE_REF_MAG:
            ch.kind = FIFFV_REF_MEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_CTF_REF_MAG;
            ch.unit = FIFF_UNIT_T;
            break;

        case CTF_SEN_TYPE_REF_GRAD:
            ch.kind = FIFFV_REF_MEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_CTF_REF_GRAD;
            ch.unit = FIFF_UNIT_T;
            break;

        case CTF_SEN_TYPE_EEG:
            ch.kind = FIFFV_EEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
            ch.unit = FIFF_UNIT_V;
            break;

        case CTF_SEN_TYPE_STIM:
            ch.kind = FIFFV_STIM_CH;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
            ch.unit = FIFF_UNIT_V;
            break;

        default:
            ch.kind = FIFFV_MISC_CH;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
            ch.unit = FIFF_UNIT_V;
            break;
    }

    // Set coil location from first coil
    if (sen.nCoils > 0) {
        // FIFF loc: r0(3), ex(3), ey(3), ez(3) = 12 values
        ch.chpos.r0(0) = sen.coils[0].x;
        ch.chpos.r0(1) = sen.coils[0].y;
        ch.chpos.r0(2) = sen.coils[0].z;
        // Orientation as ez
        ch.chpos.ez(0) = sen.coils[0].ox;
        ch.chpos.ez(1) = sen.coils[0].oy;
        ch.chpos.ez(2) = sen.coils[0].oz;
    }

    ch.cal = 1.0;
    ch.range = 1.0;
}

//=============================================================================================================

/**
 * @return The path of the first non-hidden file in @p dir whose name ends with @p extension, ignoring case, in
 *         case-insensitive name order; empty if there is none.
 */
static std::string ctfFindFile(const std::filesystem::path& dir, const std::string& extension)
{
    // (lower-case name, name) of each candidate; the smallest pair is the first in case-insensitive order
    std::optional<std::pair<std::string, std::string>> first;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(dir, error)) {
        std::string name = mne::utf8_from_path(entry.path().filename());
        std::string lower = mne::to_lower(name);
        std::error_code typeError;
        if (name.starts_with('.') || !entry.is_regular_file(typeError) || !lower.ends_with(extension))
            continue;
        std::pair<std::string, std::string> candidate(std::move(lower), std::move(name));
        if (!first || candidate < *first)
            first = std::move(candidate);
    }
    if (!first)
        return {};
    return mne::utf8_from_path(dir / mne::path_from_utf8(first->second));
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert CTF .ds MEG datasets to FIFF format.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"ds"}, "Input CTF .ds directory.", "dir", {}});
    parser.add_option({{"out"}, "Output FIFF file.", "file", {}});
    parser.process(argc, argv, "mne_ctf2fiff", PROGRAM_VERSION);

    const std::string dsPath = parser.value("ds");
    const std::string outFile = parser.value("out");

    if (dsPath.empty()) {
        Log::critical("--ds is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Locate res4 and meg4 files in .ds directory
    const std::filesystem::path dsDir = mne::path_from_utf8(dsPath);
    std::error_code dirError;
    if (!std::filesystem::is_directory(dsDir, dirError)) {
        Log::critical("Dataset directory does not exist: {}", dsPath);
        return 1;
    }

    CtfDatasetInfo info;
    info.dsPath = dsPath;

    // Find res4 file
    info.res4Path = ctfFindFile(dsDir, ".res4");
    if (info.res4Path.empty()) {
        Log::critical("No .res4 file found in {}", dsPath);
        return 1;
    }

    // Find meg4 file
    info.meg4Path = ctfFindFile(dsDir, ".meg4");
    if (info.meg4Path.empty()) {
        Log::critical("No .meg4 file found in {}", dsPath);
        return 1;
    }

    Log::info("CTF dataset: {}", dsPath);
    Log::info("  res4: {}", info.res4Path);
    Log::info("  meg4: {}", info.meg4Path);

    // Parse res4 header
    if (!parseRes4(info))
        return 1;

    // Read meg4 data
    MatrixXd data;
    if (!readMeg4Data(info, data))
        return 1;

    // Build FiffInfo
    int nChan = info.nChannels;
    FiffInfo fiffInfo;
    fiffInfo.sfreq = info.sfreq;
    fiffInfo.nchan = nChan;
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    fiffInfo.meas_date[0] = static_cast<fiff_int_t>(now.time_since_epoch().count());
    fiffInfo.meas_date[1] = 0;

    // Set coordinate transform (CTF device -> head, identity for now)
    FiffCoordTrans devHeadTrans;
    devHeadTrans.from = FIFFV_COORD_DEVICE;
    devHeadTrans.to = FIFFV_COORD_HEAD;
    devHeadTrans.trans = Matrix4f::Identity();
    devHeadTrans.invtrans = Matrix4f::Identity();
    fiffInfo.dev_head_t = devHeadTrans;

    for (int c = 0; c < nChan; ++c) {
        FiffChInfo ch;
        ch.scanNo = c + 1;
        ch.logNo = c + 1;
        mapCtfToFiff(info.sensors[c], ch);
        fiffInfo.chs.push_back(ch);
        fiffInfo.ch_names.push_back(ch.ch_name);
    }

    // Write FIFF output
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
