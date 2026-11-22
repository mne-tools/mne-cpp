//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    BIDS library: paths, sidecar TSVs, BrainVision/EDF readers and a raw-data round trip.
 *
 * Reads the BIDS dataset of the MNE-CPP test data and compares counts and
 * samples with MNE-Python 1.11 (read_raw_brainvision, read_raw_edf). Exits
 * non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/string_utils.h>
#include <mne/bids/bids_channel.h>
#include <mne/bids/bids_coordinate_system.h>
#include <mne/bids/bids_dataset_description.h>
#include <mne/bids/bids_electrode.h>
#include <mne/bids/bids_event.h>
#include <mne/bids/bids_path.h>
#include <mne/bids/bids_raw_data.h>
#include <mne/bids/bids_tsv.h>
#include <mne/bids/readers/bids_abstract_format_reader.h>
#include <mne/bids/readers/bids_brain_vision_reader.h>
#include <mne/bids/readers/bids_edf_reader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <format>
#include <print>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

bool near(double value, double expected, double relTol)
{
    return std::fabs(value - expected) <= relTol * std::fabs(expected);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption rootOption("bidsRoot", "BIDS dataset <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/BIDS");
    parser.addOption(rootOption);
    parser.process(app);
    const std::filesystem::path root = mne::path_from_utf8(parser.value(rootOption).toStdString());
    bool ok = true;

    //! [bids_path_usage]
    BIDSPath ieeg(root, "01", "01", "rest", "ieeg", "ieeg", ".vhdr");               // root, sub, ses, task, datatype, suffix, ext
    const std::filesystem::path vhdr = ieeg.file_path();                            // <root>/sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_ieeg.vhdr
    const std::filesystem::path channelsTsv = ieeg.channels_tsv_path().file_path(); // sibling sidecars share the entities
    //! [bids_path_usage]
    ok &= expect(ieeg.exists() && ieeg.basename() == "sub-01_ses-01_task-rest_ieeg.vhdr" && channelsTsv.filename() == "sub-01_ses-01_task-rest_channels.tsv",
                 "BIDSPath builds the entity filename and its channels.tsv sidecar");

    //! [bids_tsv_read]
    std::vector<std::string> headers;
    const std::vector<BidsTsvRow> rows = BidsTsv::read_tsv(ieeg.events_tsv_path().file_path(), headers);
    // each row maps column name -> cell text, e.g. rows[1].at("trial_type") == "response"
    //! [bids_tsv_read]
    ok &= expect(headers == std::vector<std::string>{"onset", "duration", "sample", "value", "trial_type"} && rows.size() == 4 && rows[1].at("trial_type") == "response" && rows[3].at("onset") == "5.250000",
                 "BidsTsv reads the 5 event columns and 4 rows");

    //! [bids_sidecar_records]
    const BidsDatasetDescription description = BidsDatasetDescription::read(root / "dataset_description.json");
    const std::vector<BidsChannel> channels = BidsChannel::read_tsv(channelsTsv);                  // one row per channel
    const std::vector<BidsEvent> events = BidsEvent::read_tsv(ieeg.events_tsv_path().file_path()); // onset, duration, sample, value, trial_type
    const std::vector<BidsElectrode> electrodes = BidsElectrode::read_tsv(ieeg.electrodes_tsv_path().file_path());
    const BidsCoordinateSystem coords = BidsCoordinateSystem::read_json(ieeg.coordsystem_json_path().file_path());
    //! [bids_sidecar_records]
    ok &= expect(description.name == "MNE-CPP Test Dataset" && description.bidsVersion == "1.9.0" && channels.size() == 32 && channels[0].name == "FP1" && channels[0].units == "\xC2\xB5V" && events.size() == 4 && events[3].sample == 5250 && events[3].value == 3 && events[3].trialType == "feedback" && electrodes.size() == 10 && electrodes[0].name == "FP1" && electrodes[0].x == "-0.0254" && coords.system == "ACPC" && coords.units == "m",
                 "dataset_description, channels/events/electrodes.tsv and coordsystem.json read as written");

    //! [brain_vision_reader_usage]
    BrainVisionReader brainVision;
    const bool bvOpen = brainVision.open(vhdr);
    const MatrixXf bvHead = brainVision.read_raw_segment(0, 200); // calibrated, volts
    //! [brain_vision_reader_usage]
    // mne.io.read_raw_brainvision: 32 channels at 1000 Hz, 7900 samples; data[0, 0] = -23.5 uV, data[5, 100] = 38.5 uV
    ok &= expect(bvOpen && brainVision.channel_count() == 32 && brainVision.sample_count() == 7900 && brainVision.sfreq() == 1000.0f && near(bvHead(0, 0), -2.35e-5, 1e-4) && near(bvHead(5, 100), 3.85e-5, 1e-4),
                 std::format("BrainVision: {} x {} samples, (0,0) = {:g} V match mne", brainVision.channel_count(), brainVision.sample_count(), bvHead(0, 0)));

    //! [brain_vision_metadata]
    const std::vector<BrainVisionChannelInfo>& bvChannels = brainVision.channel_infos(); // name, resolution, unit per Ch<n>= line
    const std::vector<BrainVisionMarker>& markers = brainVision.markers();               // Mk<n>= lines: type, description, 0-based position
    //! [brain_vision_metadata]
    // mne.io.read_raw_brainvision annotations: 13 after the "New Segment", first "Stimulus/S253" at 0.486 s
    ok &= expect(bvChannels.size() == 32 && bvChannels[0].name == "FP1" && bvChannels[0].resolution == 0.5f && markers.size() == 14 && markers[1].type == "Stimulus" && markers[1].description == "S253" && markers[1].position == 486,
                 std::format("BrainVision metadata: {} channels, {} markers, S253 at sample {} (mne 0.486 s)", bvChannels.size(), markers.size(), markers[1].position));

    //! [edf_reader_usage]
    std::unique_ptr<AbstractFormatReader> edf = BidsRawData::create_reader(".edf"); // or construct EDFReader directly
    const bool edfOpen = edf->open(BIDSPath(root, "02", "01", "rest", "eeg", "eeg", ".edf").file_path());
    const mne::FiffInfo edfInfo = edf->info();
    const MatrixXf edfHead = edf->read_raw_segment(0, 201);
    //! [edf_reader_usage]
    // mne.io.read_raw_edf: 25 channels at 128 Hz, 1228 samples, first channel "EEG Fp1"; data[3, 200] = 0.1757724
    ok &= expect(edfOpen && dynamic_cast<EDFReader*>(edf.get()) != nullptr && edf->format_name() == "EDF" && edfInfo.nchan == 25 && edfInfo.ch_names.front() == "EEG Fp1" && edf->sample_count() == 1228 && near(edfHead(0, 0), 0.17594066, 1e-5) && near(edfHead(3, 200), 0.17577240650034331, 1e-5),
                 std::format("EDF: {} channels, {} samples, (3,200) = {:.8g} match mne", edfInfo.nchan, edf->sample_count(), edfHead(3, 200)));

    //! [edf_channel_info]
    const std::vector<EDFChannelInfo>& edfChannels = static_cast<EDFReader*>(edf.get())->measurement_channel_infos(); // header fields per signal
    //! [edf_channel_info]
    // mne.io.read_raw_edf: physical_max of the first two signals 175946, -217624
    ok &= expect(edfChannels.size() == 25 && edfChannels[0].label == "EEG Fp1" && edfChannels[0].physicalMax == 175946.0f && edfChannels[1].physicalMax == -217624.0f && edfChannels[0].frequency == 128.0f,
                 std::format("EDF header: {} signals, {} physical max {:g}", edfChannels.size(), edfChannels[0].label, edfChannels[0].physicalMax));

    //! [bids_raw_data_read]
    BidsRawData data = BidsRawData::read(ieeg); // raw data plus channels, events, electrodes and sidecar metadata
    //! [bids_raw_data_read]
    ok &= expect(data.is_valid() && data.raw.info.nchan == 32 && data.raw.info.bads == std::vector<std::string>{"P4"} && data.events.size() == 4 && data.electrodes.size() == 10 && data.ieegReference == "Cz" && data.eventIdMap.at("feedback") == 3,
                 "BidsRawData applies channels.tsv (bad P4), events.tsv, electrodes.tsv and the sidecar");

    //! [bids_raw_data_write]
    QTemporaryDir out;
    BidsRawData::WriteOptions options;
    options.datasetName = "ex_bids";
    const BIDSPath written = data.write(BIDSPath(mne::path_from_utf8(out.path().toStdString()), "01", "01", "rest", "ieeg", "ieeg", ".vhdr"), vhdr, options);
    const BidsRawData readBack = BidsRawData::read(written);
    //! [bids_raw_data_write]
    // every EEG channel gets an electrodes.tsv row; the 10 measured positions must come back under the same names
    int samePositions = 0;
    for (const BidsElectrode& before : data.electrodes) {
        for (const BidsElectrode& after : readBack.electrodes) {
            samePositions += after.name == before.name && std::fabs(*mne::to_double(after.x) - *mne::to_double(before.x)) + std::fabs(*mne::to_double(after.y) - *mne::to_double(before.y)) + std::fabs(*mne::to_double(after.z) - *mne::to_double(before.z)) < 1e-6;
        }
    }
    ok &= expect(out.isValid() && written.exists() && readBack.is_valid() && readBack.raw.info.bads == data.raw.info.bads && readBack.events.size() == 4 && samePositions == 10,
                 std::format("write() then read() keeps bads, events and {}/10 electrode positions", samePositions));

    std::println("{}", ok ? "All bids checks passed." : "bids checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
