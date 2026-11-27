//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Record an acquisition source to FIFF and to a BIDS dataset while it runs, and read the file back.
 *
 * Opens the seeded oddball simulator (8 EEG and 2 MEG channels plus the trigger channel, 10 s), records it with
 * AcqRecorder to a FIFF file while the source produces, and reads the file back with FiffRawData: the samples
 * equal what the source produced, the events are annotations, and the measurement info names the device and
 * its clock. A second run records the same session into a BIDS dataset (sub-01/ses-1/meg/..._meg.fif with
 * channels.tsv, events.tsv and meg.json), which mne_bids.read_raw_bids opens. The figure shows two seconds
 * of the recording as stored on disk.
 *
 * Run it without arguments to record into a temporary directory, or with a directory to keep the recordings.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq.h>
#include <mne/acq/fiff/acq_fiff_replay.h>
#include <mne/acq/fiff/acq_recorder.h>
#include <mne/viz/backends/qt/plots/helpers/figureexporter.h>
#include <mne/viz/backends/qt/plots/lineplot.h>
#include <mne/fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    const QString directory = argc > 1 ? QString::fromLocal8Bit(argv[1]) : temporary.path();
    QDir().mkpath(directory);
    const char* uri = "synthetic://?eeg=8&meg=2&sfreq=1000&duration=10&realtime=0&seed=7";

    //! [acq_record_fiff]
    auto device = AcqDevice::open(uri);
    if (!device) {
        std::fprintf(stderr, "%s\n", device.error().text().c_str());
        return 1;
    }
    AcqRecorderOptions options;
    options.device = device->description(); // device and URI go into the file's measurement info
    const std::string path = QDir(directory).filePath("oddball_raw.fif").toStdString();
    auto recorder = AcqRecorder::start(device->stream(), path, options); // attaches its own Lossless reader
    if (!recorder || !device->start().ok()) {
        std::fprintf(stderr, "%s\n", recorder ? "cannot start the source" : recorder.error().text().c_str());
        return 1;
    }
    while (recorder->isRecording()) { // a session of fixed duration ends the stream, which ends the recording
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    device->close();
    if (const AcqError error = recorder->stop(); !error.ok()) {
        std::fprintf(stderr, "%s\n", error.text().c_str());
        return 1;
    }
    const AcqRecorderStats stats = recorder->stats();
    std::printf("recorded %llu samples (%llu lost) and %llu events in %llu buffers, %.1f MB, direct I/O %s\n",
                static_cast<unsigned long long>(stats.samples), static_cast<unsigned long long>(stats.lost),
                static_cast<unsigned long long>(stats.events), static_cast<unsigned long long>(stats.buffers),
                static_cast<double>(stats.bytes) / 1e6, stats.directIo ? "on" : "off");
    //! [acq_record_fiff]

    //! [acq_record_read]
    FiffRawData raw(QString::fromStdString(path).toStdString());
    const FiffDeviceInfo& recorded = raw.info.device_info; // backend, device and source id of the recording
    std::printf("%s: %d channels, samples %d to %d, device %s '%s' (%s)\n", path.c_str(), raw.info.nchan,
                raw.first_samp, raw.last_samp, recorded.type.c_str(), recorded.model.c_str(),
                recorded.serial.c_str());
    //! [acq_record_read]

    //! [acq_replay_fiff]
    // fiff:// replays a raw file as a source in its native format, with the trigger channel's changes as events.
    registerFiffScheme();                                                                // keeps a static link from dropping the backend
    auto replay = AcqDevice::open(AcqUri{"fiff", path, {{"realtime", "0"}}}.toString()); // as fast as possible
    AcqReaderOptions gating;
    gating.gating = true; // the replay waits for this reader, so it reads the whole file
    auto reader = replay ? replay->stream().reader(AcqReader::ReadMode::Lossless, gating) : replay.error();
    if (!reader || !replay->start().ok()) {
        return 1;
    }
    std::uint64_t replayed = 0;
    std::uint64_t triggers = 0;
    while (auto view = reader->acquire(1, std::chrono::seconds(5))) {
        replayed += view->sampleCount();
        triggers += view->events().size();
        static_cast<void>(reader->release(*view));
    }
    replay->close();
    std::printf("replayed %llu samples and %llu trigger events from %s\n", static_cast<unsigned long long>(replayed),
                static_cast<unsigned long long>(triggers), path.c_str());
    //! [acq_replay_fiff]

    //! [acq_record_bids]
    auto session = AcqDevice::open(uri);
    if (!session) {
        return 1;
    }
    AcqBidsPath bids;
    bids.root = QDir(directory).filePath("bids").toStdString();
    bids.subject = "01";
    bids.session = "1";
    bids.task = "oddball";
    bids.datasetName = "Simulated oddball";
    auto bidsRecorder = AcqRecorder::start(session->stream(), bids, options);
    if (!bidsRecorder || !session->start().ok()) {
        return 1;
    }
    while (bidsRecorder->isRecording()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    session->close();
    if (!bidsRecorder->stop().ok()) { // writes channels.tsv, events.tsv, meg.json and dataset_description.json
        return 1;
    }
    std::printf("BIDS: %s\n", bidsRecorder->files().front().c_str());
    //! [acq_record_bids]

    Eigen::MatrixXd data, times;
    if (stats.lost != 0 || !raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 1999)) {
        return 1;
    }
    LinePlot plot;
    plot.setTitle("Two seconds of the recording, read back from FIFF");
    plot.setXLabel("Time [s]");
    plot.setYLabel("Amplitude [uV], offset per channel");
    std::vector<double> t(static_cast<std::size_t>(data.cols()));
    for (int s = 0; s < std::ssize(t); ++s) {
        t[static_cast<std::size_t>(s)] = times(0, s);
    }
    for (int c = 0; c < 4; ++c) {
        std::vector<double> y(t.size());
        for (int s = 0; s < std::ssize(y); ++s) {
            y[static_cast<std::size_t>(s)] = data(c, s) * 1e6 - 40.0 * c;
        }
        plot.addSeries(t, y, QString::fromStdString(raw.info.ch_names[static_cast<std::size_t>(c)]));
    }
    std::vector<double> trigger(t.size());
    for (int s = 0; s < std::ssize(trigger); ++s) {
        trigger[static_cast<std::size_t>(s)] = data(raw.info.nchan - 1, s) * 10.0 - 200.0;
    }
    plot.addSeries(t, trigger, QString("%1 x 10").arg(raw.info.ch_names.back()), QColor(), Qt::DashLine);
    FigureExporter::save(plot, "acq_record",
                         QString("The first EEG channels and the trigger channel of a 10 s simulated oddball session, as "
                                 "AcqRecorder stored them while the source ran (%1 samples, %2 events, none lost).")
                             .arg(stats.samples)
                             .arg(stats.events)
                             .toStdString());
    return 0;
}
