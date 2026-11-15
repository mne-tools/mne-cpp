//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_raw_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BidsRawData — end-to-end BIDS raw-dataset read / write orchestration including all electrophysiology sidecars.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_raw_data.h"
#include <base/byte_stream.h>
#include <base/json.h>
#include <base/log.h>
#include <base/string_utils.h>
#include "bids_channel.h"
#include "bids_dataset_description.h"
#include "bids_const.h"
#include "readers/bids_edf_reader.h"
#include "readers/bids_brain_vision_reader.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// ANONYMOUS HELPERS — read side
//=============================================================================================================

namespace
{

//=========================================================================================================
/** channels.tsv units and their FIFF unit and multiplier (mne-bids UNITS_BIDS_TO_FIFF_MAP and UNITS_FIFF_TO_BIDS_MAP). */
struct BidsUnit
{
    std::string_view bids;
    int unit;
    int mul;
};
constexpr BidsUnit bidsUnits[] = {
    {"V", FIFF_UNIT_V, FIFF_UNITM_NONE},
    {"\xC2\xB5V", FIFF_UNIT_V, FIFF_UNITM_MU}, // UTF-8 micro sign
    {"uV", FIFF_UNIT_V, FIFF_UNITM_MU},
    {"mV", FIFF_UNIT_V, FIFF_UNITM_M},
    {"nV", FIFF_UNIT_V, FIFF_UNITM_N},
    {"T", FIFF_UNIT_T, FIFF_UNITM_NONE},
    {"fT", FIFF_UNIT_T, FIFF_UNITM_F},
    {"pT", FIFF_UNIT_T, FIFF_UNITM_P},
    {"T/m", FIFF_UNIT_T_M, FIFF_UNITM_NONE},
    {"rad", FIFF_UNIT_RAD, FIFF_UNITM_NONE},
    {"S", FIFF_UNIT_MHO, FIFF_UNITM_NONE},
    {"oC", FIFF_UNIT_CEL, FIFF_UNITM_NONE},
    {"M", FIFF_UNIT_MOL, FIFF_UNITM_NONE},
    {"arbitrary", FIFF_UNIT_NONE, FIFF_UNITM_NONE},
};

//=========================================================================================================
/** FIFF unit of a channels.tsv unit as mne-bids UNITS_BIDS_TO_FIFF_MAP (unknown units are left alone); the multiplier keeps the stored unit for writing. */
void bidsUnitToFiff(std::string_view sUnit, int& unit, int& unitMul)
{
    const std::string_view u = BASELIB::trimmed(sUnit);
    for (const BidsUnit& e : bidsUnits) {
        if (u == e.bids) {
            unit = e.unit;
            unitMul = e.mul;
            return;
        }
    }
}

//=========================================================================================================
std::string toUpper(std::string_view text)
{
    std::string upper(text);
    std::ranges::transform(upper, upper.begin(), [](unsigned char c) { return static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c); });
    return upper;
}

//=========================================================================================================
void applyChannelsTsv(FiffInfo& info, const std::vector<BidsChannel>& channels)
{
    if (channels.empty())
        return;

    std::map<std::string, const BidsChannel*, std::less<>> channelMap;
    for (const BidsChannel& ch : channels)
        channelMap[ch.name] = &ch;

    info.bads.clear();

    for (FiffChInfo& fiffCh : info.chs) {
        auto it = channelMap.find(fiffCh.ch_name);
        if (it == channelMap.end())
            continue;
        const BidsChannel* rec = it->second;

        if (const std::optional<int> kind = bids_lookup(BIDS_TYPE_TO_FIFF_KIND, toUpper(rec->type)))
            fiffCh.kind = *kind;

        bidsUnitToFiff(rec->units, fiffCh.unit, fiffCh.unit_mul);

        if (BASELIB::equalsIgnoreCase(rec->status, "bad"))
            info.bads.push_back(fiffCh.ch_name);
    }
}

//=========================================================================================================
void applyElectrodePositions(FiffInfo& info,
                             const std::vector<BidsElectrode>& electrodes,
                             std::string_view coordSystemName,
                             std::string_view coordUnits)
{
    if (electrodes.empty())
        return;

    const int coordFrame = bids_lookup(BIDS_COORD_TO_FIFF_FRAME, coordSystemName).value_or(FIFFV_COORD_UNKNOWN);

    float scaleFactor = 1.0f;
    if (BASELIB::equalsIgnoreCase(coordUnits, "mm"))
        scaleFactor = 0.001f;
    else if (BASELIB::equalsIgnoreCase(coordUnits, "cm"))
        scaleFactor = 0.01f;

    // Ident = 1-based ordinal among electrode channels, the order buildElectrodeRecords() reads them back in
    std::map<std::string, int, std::less<>> chOrdinal;
    for (const auto& ch : info.chs) {
        if (ch.kind == FIFFV_EEG_CH || ch.kind == FIFFV_ECOG_CH || ch.kind == FIFFV_SEEG_CH || ch.kind == FIFFV_DBS_CH)
            chOrdinal.emplace(ch.ch_name, static_cast<int>(chOrdinal.size()) + 1);
    }

    info.dig.clear();
    int extraIdent = static_cast<int>(chOrdinal.size()) + 1;
    for (const BidsElectrode& elec : electrodes) {
        if (elec.x == "n/a" || elec.y == "n/a" || elec.z == "n/a")
            continue;

        FiffDigPoint dp;
        const auto ordinal = chOrdinal.find(elec.name);
        dp.kind = ordinal != chOrdinal.end() ? FIFFV_POINT_EEG : FIFFV_POINT_EXTRA;
        dp.ident = ordinal != chOrdinal.end() ? ordinal->second : extraIdent++;
        dp.r[0] = static_cast<float>(BASELIB::toDouble(elec.x).value_or(0.0)) * scaleFactor;
        dp.r[1] = static_cast<float>(BASELIB::toDouble(elec.y).value_or(0.0)) * scaleFactor;
        dp.r[2] = static_cast<float>(BASELIB::toDouble(elec.z).value_or(0.0)) * scaleFactor;
        dp.coord_frame = coordFrame;

        info.dig.push_back(dp);
    }
}

//=========================================================================================================
BASELIB::JsonDocument readJsonFile(const std::filesystem::path& path)
{
    const BASELIB::Expected<std::string> text = BASELIB::read_text_file(path);
    return text ? BASELIB::JsonDocument::from_json(*text) : BASELIB::JsonDocument();
}

//=========================================================================================================
void readSidecarJson(const std::filesystem::path& path,
                     FiffInfo& info,
                     BidsRawData& data)
{
    const BASELIB::JsonDocument doc = readJsonFile(path);
    const BASELIB::JsonObject& json = doc.object();
    if (json.empty())
        return;

    // Apply to FiffInfo
    const double plf = json.value("PowerLineFrequency").to_double();
    if (plf > 0.0)
        info.linefreq = static_cast<float>(plf);

    const double sf = json.value("SamplingFrequency").to_double();
    if (sf > 0.0 && std::abs(info.sfreq - static_cast<float>(sf)) > 0.5f)
        BASELIB::Log::warning("[BidsRawData::read] Sampling frequency mismatch: raw = {} sidecar = {}", info.sfreq, sf);

    // Store independent metadata fields on BidsRawData
    data.ieegReference = json.value("iEEGReference").to_string();
    data.taskDescription = json.value("TaskDescription").to_string();
    data.manufacturer = json.value("Manufacturer").to_string();
    data.manufacturerModelName = json.value("ManufacturerModelName").to_string();
    data.softwareVersions = json.value("SoftwareVersions").to_string();
    data.recordingType = json.value("RecordingType").to_string();
}

//=============================================================================================================
// ANONYMOUS HELPERS — write side
//=============================================================================================================

//=========================================================================================================
std::string fiffUnitToBidsString(int unit, int unitMul)
{
    for (const BidsUnit& e : bidsUnits) {
        if (e.unit == unit && e.mul == unitMul && e.unit != FIFF_UNIT_NONE)
            return std::string(e.bids);
    }
    // A multiplier BIDS has no name for: the SI unit
    for (const BidsUnit& e : bidsUnits) {
        if (e.unit == unit && e.mul == FIFF_UNITM_NONE && e.unit != FIFF_UNIT_NONE)
            return std::string(e.bids);
    }
    return "n/a";
}

//=========================================================================================================
std::string_view channelTypeDescription(int kind)
{
    switch (kind) {
        case FIFFV_EEG_CH:
            return "ElectroEncephaloGram";
        case FIFFV_ECOG_CH:
            return "Electrocorticography";
        case FIFFV_SEEG_CH:
            return "StereoElectroEncephaloGram";
        case FIFFV_DBS_CH:
            return "DeepBrainStimulation";
        case FIFFV_MEG_CH:
            return "MagnetoEncephaloGram";
        case FIFFV_STIM_CH:
            return "Trigger";
        case FIFFV_EOG_CH:
            return "ElectroOculoGram";
        case FIFFV_ECG_CH:
            return "ElectroCardioGram";
        case FIFFV_EMG_CH:
            return "ElectroMyoGram";
        case FIFFV_MISC_CH:
            return "Miscellaneous";
        case FIFFV_RESP_CH:
            return "Respiration";
        default:
            return "n/a";
    }
}

//=========================================================================================================
std::vector<BidsChannel> buildChannelRecords(const FiffInfo& info)
{
    std::vector<BidsChannel> records;
    records.reserve(info.chs.size());

    for (const FiffChInfo& ch : info.chs) {
        BidsChannel rec;
        rec.name = ch.ch_name;
        rec.type = bids_lookup(FIFF_KIND_TO_BIDS_TYPE, ch.kind).value_or("MISC");
        rec.units = fiffUnitToBidsString(ch.unit, ch.unit_mul);
        rec.samplingFreq = std::format("{:.10g}", static_cast<double>(info.sfreq));
        rec.lowCutoff = info.highpass > 0.0f ? std::format("{:.10g}", static_cast<double>(info.highpass)) : "n/a";
        rec.highCutoff = info.lowpass > 0.0f ? std::format("{:.10g}", static_cast<double>(info.lowpass)) : "n/a";
        rec.notch = "n/a";
        rec.status = info.isBad(ch.ch_name) ? "bad" : "good";
        rec.description = channelTypeDescription(ch.kind);

        records.push_back(std::move(rec));
    }
    return records;
}

//=========================================================================================================
std::vector<BidsElectrode> buildElectrodeRecords(const FiffInfo& info)
{
    std::vector<BidsElectrode> records;

    std::map<int, const FiffDigPoint*> digByIdent;
    for (const auto& dp : info.dig) {
        if (dp.kind == FIFFV_POINT_EEG || dp.kind == FIFFV_POINT_EXTRA)
            digByIdent[dp.ident] = &dp;
    }

    const auto coordinate = [](float value) {
        return std::format("{:.8g}", static_cast<double>(value));
    };
    int digIdx = 0;

    for (const FiffChInfo& ch : info.chs) {
        if (ch.kind != FIFFV_EEG_CH && ch.kind != FIFFV_ECOG_CH &&
            ch.kind != FIFFV_SEEG_CH && ch.kind != FIFFV_DBS_CH)
            continue;

        BidsElectrode rec;
        rec.name = ch.ch_name;

        ++digIdx;
        bool hasPosition = false;

        if (const auto it = digByIdent.find(digIdx); it != digByIdent.end()) {
            const FiffDigPoint* dp = it->second;
            if (std::isfinite(dp->r[0]) && std::isfinite(dp->r[1]) && std::isfinite(dp->r[2])) {
                rec.x = coordinate(dp->r[0]);
                rec.y = coordinate(dp->r[1]);
                rec.z = coordinate(dp->r[2]);
                hasPosition = true;
            }
        }

        if (!hasPosition) {
            const Eigen::Vector3f& r0 = ch.chpos.r0;
            if (r0.squaredNorm() > 0.0f && std::isfinite(r0[0])) {
                rec.x = coordinate(r0[0]);
                rec.y = coordinate(r0[1]);
                rec.z = coordinate(r0[2]);
            } else {
                rec.x = "n/a";
                rec.y = "n/a";
                rec.z = "n/a";
            }
        }

        rec.size = "n/a";
        rec.type = "n/a";
        rec.material = "n/a";
        rec.impedance = "n/a";

        records.push_back(std::move(rec));
    }
    return records;
}

//=========================================================================================================
BASELIB::JsonObject buildSidecarJson(const BidsRawData& data,
                                     const BIDSPath& bidsPath)
{
    BASELIB::JsonObject json;
    const FiffInfo& info = data.raw.info;
    const std::string& datatype = bidsPath.datatype();

    // Required for every electrophysiology datatype
    json.insert("TaskName", bidsPath.task());
    json.insert("SamplingFrequency", static_cast<double>(info.sfreq));
    if (info.linefreq > 0.0f)
        json.insert("PowerLineFrequency", static_cast<double>(info.linefreq));
    else
        json.insert("PowerLineFrequency", "n/a");
    json.insert("SoftwareFilters", "n/a");

    // Required per datatype
    const std::string reference = data.ieegReference.empty() ? std::string("n/a") : data.ieegReference;
    if (datatype == "meg") {
        json.insert("DewarPosition", "n/a");
        json.insert("DigitizedLandmarks", false);
        json.insert("DigitizedHeadPoints", !info.dig.empty());
    } else if (datatype == "eeg") {
        json.insert("EEGReference", reference);
    } else {
        json.insert("iEEGReference", reference);
    }

    // Channel counts — computed from FiffInfo
    int meg = 0, megRef = 0, ecog = 0, seeg = 0, dbs = 0, eeg = 0, eog = 0, ecg = 0, emg = 0, misc = 0, trig = 0;
    for (const auto& ch : info.chs) {
        switch (ch.kind) {
            case FIFFV_MEG_CH:
                ++meg;
                break;
            case FIFFV_REF_MEG_CH:
                ++megRef;
                break;
            case FIFFV_ECOG_CH:
                ++ecog;
                break;
            case FIFFV_SEEG_CH:
                ++seeg;
                break;
            case FIFFV_DBS_CH:
                ++dbs;
                break;
            case FIFFV_EEG_CH:
                ++eeg;
                break;
            case FIFFV_EOG_CH:
                ++eog;
                break;
            case FIFFV_ECG_CH:
                ++ecg;
                break;
            case FIFFV_EMG_CH:
                ++emg;
                break;
            case FIFFV_MISC_CH:
                ++misc;
                break;
            case FIFFV_STIM_CH:
                ++trig;
                break;
            default:
                break;
        }
    }
    if (datatype == "meg") {
        json.insert("MEGChannelCount", meg);
        json.insert("MEGREFChannelCount", megRef);
    }
    if (datatype == "ieeg") {
        json.insert("ECOGChannelCount", ecog);
        json.insert("SEEGChannelCount", seeg);
        if (dbs > 0)
            json.insert("DBSChannelCount", dbs);
    }
    if (eeg > 0 || datatype == "eeg")
        json.insert("EEGChannelCount", eeg);
    if (eog > 0)
        json.insert("EOGChannelCount", eog);
    if (ecg > 0)
        json.insert("ECGChannelCount", ecg);
    if (emg > 0)
        json.insert("EMGChannelCount", emg);
    if (misc > 0)
        json.insert("MiscChannelCount", misc);
    if (trig > 0)
        json.insert("TriggerChannelCount", trig);

    // Recording metadata
    json.insert("RecordingType", data.recordingType.empty() ? std::string("continuous") : data.recordingType);

    // mne-bids writes raw.times[-1]
    if (info.sfreq > 0.0f && data.raw.last_samp >= data.raw.first_samp) {
        const double dur = static_cast<double>(data.raw.last_samp - data.raw.first_samp) / static_cast<double>(info.sfreq);
        json.insert("RecordingDuration", dur);
    }

    // Optional strings from BidsRawData
    if (!data.taskDescription.empty())
        json.insert("TaskDescription", data.taskDescription);
    if (!data.manufacturer.empty())
        json.insert("Manufacturer", data.manufacturer);
    if (!data.manufacturerModelName.empty())
        json.insert("ManufacturerModelName", data.manufacturerModelName);
    if (!data.softwareVersions.empty())
        json.insert("SoftwareVersions", data.softwareVersions);

    return json;
}

//=========================================================================================================
bool fileExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error);
}

//=========================================================================================================
bool copyFile(const std::filesystem::path& src, const std::filesystem::path& dst, bool overwrite)
{
    if (!fileExists(src)) {
        BASELIB::Log::warning("[BidsRawData::write] Source file does not exist: {}", BASELIB::utf8FromPath(src));
        return false;
    }
    if (fileExists(dst) && !overwrite) {
        BASELIB::Log::warning("[BidsRawData::write] Target file already exists: {}", BASELIB::utf8FromPath(dst));
        return false;
    }
    std::error_code error;
    return std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, error);
}

//=========================================================================================================
/** Replaces every occurrence of @p from in @p text by @p to. */
void replaceAll(std::string& text, std::string_view from, std::string_view to)
{
    if (from.empty())
        return;
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
        text.replace(at, from.size(), to);
}

//=========================================================================================================
/** The value after "key=" if @p line (trimmed) starts with it, compared case-insensitively. */
std::optional<std::string_view> headerValue(std::string_view line, std::string_view key)
{
    line = BASELIB::trimmed(line);
    if (line.size() < key.size() || !BASELIB::equalsIgnoreCase(line.substr(0, key.size()), key))
        return std::nullopt;
    return BASELIB::trimmed(line.substr(key.size()));
}

//=========================================================================================================
bool copyBrainVisionFiles(const std::filesystem::path& srcVhdr, const BIDSPath& bidsPath, bool overwrite)
{
    const BASELIB::Expected<std::string> vhdrContent = BASELIB::read_text_file(srcVhdr);
    if (!vhdrContent) {
        BASELIB::Log::warning("[BidsRawData::write] Cannot open .vhdr file: {}", BASELIB::utf8FromPath(srcVhdr));
        return false;
    }

    std::string dataFileName;
    std::string markerFileName;
    for (const std::string& line : BASELIB::split(*vhdrContent, '\n')) {
        if (const auto value = headerValue(line, "DataFile="))
            dataFileName = *value;
        else if (const auto value = headerValue(line, "MarkerFile="))
            markerFileName = *value;
    }

    const std::filesystem::path srcDir = std::filesystem::absolute(srcVhdr).parent_path();
    const std::filesystem::path dstDir = bidsPath.directory();
    std::string dstBase = bidsPath.basename();
    dstBase = dstBase.substr(0, dstBase.rfind('.'));

    const std::filesystem::path dstVhdr = bidsPath.file_path();
    const std::string newDataFile = dstBase + ".eeg";
    const std::string newMarkerFile = dstBase + ".vmrk";

    std::string vhdr = *vhdrContent;
    if (!dataFileName.empty())
        replaceAll(vhdr, "DataFile=" + dataFileName, "DataFile=" + newDataFile);
    if (!markerFileName.empty())
        replaceAll(vhdr, "MarkerFile=" + markerFileName, "MarkerFile=" + newMarkerFile);

    if (fileExists(dstVhdr) && !overwrite) {
        BASELIB::Log::warning("[BidsRawData::write] Target file already exists: {}", BASELIB::utf8FromPath(dstVhdr));
        return false;
    }
    if (!BASELIB::write_text_file(dstVhdr, vhdr)) {
        BASELIB::Log::warning("[BidsRawData::write] Cannot write .vhdr file: {}", BASELIB::utf8FromPath(dstVhdr));
        return false;
    }

    if (!dataFileName.empty()) {
        const std::filesystem::path srcData = srcDir / BASELIB::pathFromUtf8(dataFileName);
        if (!copyFile(srcData, dstDir / BASELIB::pathFromUtf8(newDataFile), overwrite)) {
            BASELIB::Log::warning("[BidsRawData::write] Failed to copy data file: {}", BASELIB::utf8FromPath(srcData));
            return false;
        }
    }

    if (!markerFileName.empty()) {
        const std::filesystem::path srcMarker = srcDir / BASELIB::pathFromUtf8(markerFileName);
        const std::filesystem::path dstMarker = dstDir / BASELIB::pathFromUtf8(newMarkerFile);

        if (BASELIB::Expected<std::string> marker = BASELIB::read_text_file(srcMarker)) {
            replaceAll(*marker, "DataFile=" + dataFileName, "DataFile=" + newDataFile);

            if (fileExists(dstMarker) && !overwrite) {
                BASELIB::Log::warning("[BidsRawData::write] Target marker file already exists: {}", BASELIB::utf8FromPath(dstMarker));
                return false;
            }
            if (!BASELIB::write_text_file(dstMarker, *marker))
                BASELIB::Log::warning("[BidsRawData::write] Cannot write marker file: {}", BASELIB::utf8FromPath(dstMarker));
        }
    }

    return true;
}

//=========================================================================================================
bool copyRawDataFile(const std::filesystem::path& sourcePath, const BIDSPath& bidsPath, bool overwrite)
{
    if (sourcePath.empty())
        return true;

    const std::string ext = BASELIB::toLower(bidsPath.extension());
    if (ext == ".vhdr" || ext == ".ahdr")
        return copyBrainVisionFiles(sourcePath, bidsPath, overwrite);

    return copyFile(sourcePath, bidsPath.file_path(), overwrite);
}

} // anonymous namespace

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

BidsRawData::BidsRawData(BidsRawData&& other) noexcept
: raw(std::move(other.raw))
, events(std::move(other.events))
, eventIdMap(std::move(other.eventIdMap))
, electrodes(std::move(other.electrodes))
, coordinateSystem(std::move(other.coordinateSystem))
, reader(std::move(other.reader))
, ieegReference(std::move(other.ieegReference))
, taskDescription(std::move(other.taskDescription))
, manufacturer(std::move(other.manufacturer))
, manufacturerModelName(std::move(other.manufacturerModelName))
, softwareVersions(std::move(other.softwareVersions))
, recordingType(std::move(other.recordingType))
, m_bIsValid(other.m_bIsValid)
{
    other.m_bIsValid = false;
}

//=============================================================================================================

BidsRawData& BidsRawData::operator=(BidsRawData&& other) noexcept
{
    if (this != &other) {
        raw = std::move(other.raw);
        events = std::move(other.events);
        eventIdMap = std::move(other.eventIdMap);
        electrodes = std::move(other.electrodes);
        coordinateSystem = std::move(other.coordinateSystem);
        reader = std::move(other.reader);
        ieegReference = std::move(other.ieegReference);
        taskDescription = std::move(other.taskDescription);
        manufacturer = std::move(other.manufacturer);
        manufacturerModelName = std::move(other.manufacturerModelName);
        softwareVersions = std::move(other.softwareVersions);
        recordingType = std::move(other.recordingType);
        m_bIsValid = other.m_bIsValid;
        other.m_bIsValid = false;
    }
    return *this;
}

//=============================================================================================================

void BidsRawData::clear()
{
    raw = FiffRawData();
    events.clear();
    eventIdMap.clear();
    electrodes.clear();
    reader.reset();

    ieegReference.clear();
    taskDescription.clear();
    manufacturer.clear();
    manufacturerModelName.clear();
    softwareVersions.clear();
    recordingType.clear();

    coordinateSystem = BidsCoordinateSystem();

    m_bIsValid = false;
}

//=============================================================================================================

std::unique_ptr<AbstractFormatReader> BidsRawData::create_reader(std::string_view sExtension)
{
    const std::string ext = BASELIB::toLower(sExtension);
    if (ext == ".vhdr" || ext == ".ahdr")
        return std::make_unique<BrainVisionReader>();
    if (ext == ".edf" || ext == ".bdf")
        return std::make_unique<EDFReader>();
    return nullptr;
}

//=============================================================================================================
// BidsRawData::read()
//=============================================================================================================

BidsRawData BidsRawData::read(const BIDSPath& bidsPath)
{
    BidsRawData result;

    //=========================================================================================================
    // Step 1 — Validate BIDSPath
    //=========================================================================================================
    if (bidsPath.root().empty()) {
        BASELIB::Log::warning("[BidsRawData::read] BIDSPath root is not set");
        return result;
    }
    if (bidsPath.subject().empty()) {
        BASELIB::Log::warning("[BidsRawData::read] BIDSPath subject is not set");
        return result;
    }
    if (bidsPath.extension().empty()) {
        BASELIB::Log::warning("[BidsRawData::read] BIDSPath extension is not set");
        return result;
    }

    //=========================================================================================================
    // Step 2 — Resolve raw file path
    //=========================================================================================================
    std::filesystem::path rawFilePath = bidsPath.file_path();

    if (!fileExists(rawFilePath) && BASELIB::toLower(bidsPath.extension()) == ".edf") {
        BIDSPath altPath(bidsPath);
        altPath.set_extension(".EDF");
        if (fileExists(altPath.file_path()))
            rawFilePath = altPath.file_path();
    }

    if (!fileExists(rawFilePath)) {
        BASELIB::Log::warning("[BidsRawData::read] Raw data file not found: {}", BASELIB::utf8FromPath(rawFilePath));
        return result;
    }

    //=========================================================================================================
    // Step 3 — Create and open the format reader
    //=========================================================================================================
    result.reader = create_reader(bidsPath.extension());
    if (!result.reader) {
        BASELIB::Log::warning("[BidsRawData::read] Unsupported file extension: {}", bidsPath.extension());
        return result;
    }

    if (!result.reader->open(QString::fromStdString(BASELIB::utf8FromPath(rawFilePath)))) {
        BASELIB::Log::warning("[BidsRawData::read] Failed to open raw file: {}", BASELIB::utf8FromPath(rawFilePath));
        return result;
    }

    //=========================================================================================================
    // Step 4 — Build FiffRawData from the reader
    //=========================================================================================================
    result.raw = result.reader->toFiffRawData();

    //=========================================================================================================
    // Step 5 — Read and apply *_channels.tsv
    //=========================================================================================================
    if (const std::filesystem::path channelsPath = bidsPath.channels_tsv_path().file_path(); fileExists(channelsPath))
        applyChannelsTsv(result.raw.info, BidsChannel::read_tsv(channelsPath));

    //=========================================================================================================
    // Step 6 — Read *_coordsystem.json (before electrodes, for scale/frame info)
    //=========================================================================================================
    if (const std::filesystem::path coordsysPath = bidsPath.coordsystem_json_path().file_path(); fileExists(coordsysPath))
        result.coordinateSystem = BidsCoordinateSystem::read_json(coordsysPath);

    //=========================================================================================================
    // Step 7 — Read and apply *_electrodes.tsv
    //=========================================================================================================
    if (const std::filesystem::path electrodesPath = bidsPath.electrodes_tsv_path().file_path(); fileExists(electrodesPath)) {
        result.electrodes = BidsElectrode::read_tsv(electrodesPath);
        applyElectrodePositions(result.raw.info, result.electrodes,
                                result.coordinateSystem.system, result.coordinateSystem.units);
    }

    //=========================================================================================================
    // Step 8 — Read *_events.tsv
    //=========================================================================================================
    if (const std::filesystem::path eventsPath = bidsPath.events_tsv_path().file_path(); fileExists(eventsPath)) {
        result.events = BidsEvent::read_tsv(eventsPath);

        // Compute sample from onset*sfreq if sample column was absent
        const float sfreq = result.raw.info.sfreq;
        for (BidsEvent& ev : result.events) {
            if (ev.sample == 0 && ev.onset > 0.0f && sfreq > 0)
                ev.sample = static_cast<int>(ev.onset * sfreq);
        }

        // Build eventIdMap from trial_type → value
        for (const BidsEvent& ev : result.events) {
            if (!ev.trialType.empty() && ev.trialType != "n/a")
                result.eventIdMap.insert_or_assign(ev.trialType, ev.value);
        }
    }

    //=========================================================================================================
    // Step 9 — Read sidecar *_{datatype}.json
    //=========================================================================================================
    if (const std::filesystem::path sidecarPath = bidsPath.sidecar_json_path().file_path(); fileExists(sidecarPath))
        readSidecarJson(sidecarPath, result.raw.info, result);

    //=========================================================================================================
    // Done
    //=========================================================================================================
    result.m_bIsValid = true;
    return result;
}

//=============================================================================================================
// BidsRawData::write()
//=============================================================================================================

BIDSPath BidsRawData::write(const BIDSPath& bidsPath,
                            const std::filesystem::path& sourcePath,
                            const WriteOptions& options) const
{
    BIDSPath result;

    //=========================================================================================================
    // Step 1 — Validate
    //=========================================================================================================
    if (bidsPath.root().empty()) {
        BASELIB::Log::warning("[BidsRawData::write] BIDSPath root is not set");
        return result;
    }
    if (bidsPath.subject().empty()) {
        BASELIB::Log::warning("[BidsRawData::write] BIDSPath subject is not set");
        return result;
    }
    if (bidsPath.task().empty()) {
        BASELIB::Log::warning("[BidsRawData::write] BIDSPath task is not set");
        return result;
    }
    if (bidsPath.datatype().empty()) {
        BASELIB::Log::warning("[BidsRawData::write] BIDSPath datatype is not set");
        return result;
    }
    if (raw.info.empty()) {
        BASELIB::Log::warning("[BidsRawData::write] FiffRawData info is empty");
        return result;
    }

    //=========================================================================================================
    // Step 2 — Create directory structure
    //=========================================================================================================
    if (!bidsPath.mkdirs()) {
        BASELIB::Log::warning("[BidsRawData::write] Failed to create directory: {}", BASELIB::utf8FromPath(bidsPath.directory()));
        return result;
    }

    //=========================================================================================================
    // Step 3 — Copy raw data file
    //=========================================================================================================
    if (options.copyData && !sourcePath.empty()) {
        if (!copyRawDataFile(sourcePath, bidsPath, options.overwrite)) {
            BASELIB::Log::warning("[BidsRawData::write] Failed to copy raw data file");
            return result;
        }
    }

    //=========================================================================================================
    // Step 4 — Write *_channels.tsv
    //=========================================================================================================
    {
        const std::filesystem::path channelsPath = bidsPath.channels_tsv_path().file_path();
        if (!options.overwrite && fileExists(channelsPath)) {
            BASELIB::Log::warning("[BidsRawData::write] channels.tsv already exists: {}", BASELIB::utf8FromPath(channelsPath));
            return result;
        }

        if (!BidsChannel::write_tsv(channelsPath, buildChannelRecords(raw.info))) {
            BASELIB::Log::warning("[BidsRawData::write] Failed to write channels.tsv");
            return result;
        }
    }

    //=========================================================================================================
    // Step 5 — Write *_electrodes.tsv + *_coordsystem.json
    //=========================================================================================================
    {
        std::vector<BidsElectrode> electrodeRecords = buildElectrodeRecords(raw.info);
        // Electrodes without any position say nothing a channels.tsv does not.
        if (std::ranges::none_of(electrodeRecords, [](const BidsElectrode& e) { return e.x != "n/a"; }))
            electrodeRecords.clear();

        if (!electrodeRecords.empty()) {
            const std::filesystem::path electrodesPath = bidsPath.electrodes_tsv_path().file_path();
            if (options.overwrite || !fileExists(electrodesPath)) {
                if (!BidsElectrode::write_tsv(electrodesPath, electrodeRecords))
                    BASELIB::Log::warning("[BidsRawData::write] Failed to write electrodes.tsv");
            }
        }

        // MEG always states its coordinate system (sensor positions live in the data file).
        const std::string& datatype = bidsPath.datatype();
        const bool isMeg = datatype == "meg";
        const std::filesystem::path coordsysPath = bidsPath.coordsystem_json_path().file_path();
        if ((isMeg || !electrodeRecords.empty()) && (options.overwrite || !fileExists(coordsysPath))) {
            // Build coordinate system, deriving defaults from FiffInfo if needed
            BidsCoordinateSystem cs = coordinateSystem;
            if (cs.system.empty()) {
                if (raw.info.dig.empty()) {
                    cs.system = "Other";
                    cs.units = isMeg ? "m" : "n/a";
                } else {
                    cs.system = bids_lookup(FIFF_FRAME_TO_BIDS_COORD, raw.info.dig.front().coord_frame).value_or("Other");
                    cs.units = "m";
                }
            }
            if (cs.description.empty() && !raw.info.dig.empty())
                cs.description = "Coordinate system derived from recording data";
            // REQUIRED by BIDS for "Other".
            if (cs.description.empty() && cs.system == "Other")
                cs.description = isMeg ? "Device coordinates of the acquisition system"
                                       : "Coordinate system of the electrode positions";

            const std::string_view modality = isMeg ? "MEG" : datatype == "eeg" ? "EEG"
                                                                                : "iEEG";
            if (!BidsCoordinateSystem::write_json(coordsysPath, cs, modality))
                BASELIB::Log::warning("[BidsRawData::write] Failed to write coordsystem.json");
        }
    }

    //=========================================================================================================
    // Step 6 — Write *_events.tsv
    //=========================================================================================================
    if (!events.empty()) {
        const std::filesystem::path eventsPath = bidsPath.events_tsv_path().file_path();
        if (!options.overwrite && fileExists(eventsPath)) {
            BASELIB::Log::warning("[BidsRawData::write] events.tsv already exists: {}", BASELIB::utf8FromPath(eventsPath));
            return result;
        }

        std::vector<BidsEvent> eventsToWrite = events;

        // Apply trial_type fallback from eventIdMap
        std::map<int, std::string> valueToType;
        for (const auto& [type, value] : eventIdMap)
            valueToType[value] = type;
        for (BidsEvent& ev : eventsToWrite) {
            if (ev.trialType.empty() || ev.trialType == "n/a") {
                const auto it = valueToType.find(ev.value);
                ev.trialType = it != valueToType.end() ? it->second : "n/a";
            }
        }

        if (!BidsEvent::write_tsv(eventsPath, eventsToWrite))
            BASELIB::Log::warning("[BidsRawData::write] Failed to write events.tsv");
    }

    //=========================================================================================================
    // Step 7 — Write *_{datatype}.json sidecar
    //=========================================================================================================
    {
        const std::filesystem::path sidecarPath = bidsPath.sidecar_json_path().file_path();
        if (!options.overwrite && fileExists(sidecarPath)) {
            BASELIB::Log::warning("[BidsRawData::write] Sidecar JSON already exists: {}", BASELIB::utf8FromPath(sidecarPath));
            return result;
        }

        if (!BASELIB::write_text_file(sidecarPath, BASELIB::JsonDocument(buildSidecarJson(*this, bidsPath)).to_json())) {
            BASELIB::Log::warning("[BidsRawData::write] Failed to write sidecar JSON");
            return result;
        }
    }

    //=========================================================================================================
    // Step 8 — Write dataset_description.json (never overwrite)
    //=========================================================================================================
    {
        const std::filesystem::path descPath = bidsPath.root() / "dataset_description.json";

        if (!fileExists(descPath)) {
            BidsDatasetDescription desc;
            desc.name = options.datasetName.empty() ? std::string("[Unspecified]") : options.datasetName;
            desc.bidsVersion = "1.9.0";
            desc.datasetType = "raw";

            if (!BidsDatasetDescription::write(descPath, desc))
                BASELIB::Log::warning("[BidsRawData::write] Failed to write dataset_description.json");
        }
    }

    //=========================================================================================================
    // Done
    //=========================================================================================================
    result = bidsPath;
    return result;
}
