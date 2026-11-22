//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    LSL library: publish a stream, discover it and receive its samples over loopback.
 *
 * An outlet publishes a 3-channel stream; an inlet connected to it on
 * 127.0.0.1 must receive every pushed sample in order. Discovery by name is
 * checked when multicast is available. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/lsl/lsl_stream_discovery.h>
#include <mne/lsl/lsl_stream_info.h>
#include <mne/lsl/lsl_stream_inlet.h>
#include <mne/lsl/lsl_stream_outlet.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <format>
#include <print>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    //! [lsl_stream_outlet_usage]
    stream_info info("ex_lsl_eeg", "EEG", 3, 250.0, ChannelFormat::Float32, "ex_lsl"); // name, type, channels, Hz
    stream_outlet outlet(info);                                                        // starts serving immediately
    //! [lsl_stream_outlet_usage]

    //! [lsl_channel_description_usage]
    stream_info described("ex_lsl_labelled", "EEG", 2, 250.0, ChannelFormat::Float32, "ex_lsl_labelled");
    described.set_channels({{"Fz", "EEG", "microvolts"}, {"Cz", "EEG", "microvolts"}}); // sent with the full info
    stream_outlet labelled(described);
    //! [lsl_channel_description_usage]
    stream_info labelledSource = labelled.info();
    labelledSource.set_data_host("127.0.0.1");
    stream_inlet labelledInlet(labelledSource);
    const stream_info full = labelledInlet.info(); // the full info, fetched from the outlet once
    ok &= expect(full.channels().size() == 2 && full.channels()[0].label == "Fz" && full.channels()[1].unit == "microvolts",
                 "channel_description: labels and units travel with the stream's full info");
    ok &= expect(outlet.info().name() == "ex_lsl_eeg" && outlet.info().channel_count() == 3 && outlet.info().nominal_srate() == 250.0 && outlet.info().data_port() > 0 && !outlet.info().uid().empty(),
                 std::format("stream_info: 3 channels at 250 Hz, served on port {}", outlet.info().data_port()));

    //! [lsl_stream_inlet_usage]
    stream_info source = outlet.info(); // normally an entry of resolve_stream("name", "ex_lsl_eeg")
    source.set_data_host("127.0.0.1");
    stream_inlet inlet(source);
    inlet.open_stream();
    QElapsedTimer clock;
    clock.start();
    while (!outlet.have_consumers() && clock.elapsed() < 5000) { // samples pushed before the outlet accepts the inlet are not delivered
        QThread::msleep(5);
    }
    outlet.push_sample({1.0f, 2.0f, 3.0f});
    outlet.push_chunk({{4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}});

    std::vector<std::vector<float>> received;
    clock.restart();
    while (received.size() < 3 && clock.elapsed() < 5000) {
        QThread::msleep(20);
        for (const std::vector<float>& sample : inlet.pull_chunk<float>()) { // one vector per sample
            received.push_back(sample);
        }
    }
    inlet.close_stream();
    //! [lsl_stream_inlet_usage]
    ok &= expect(received == std::vector<std::vector<float>>({{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}}),
                 std::format("stream_inlet received {} of 3 samples in order", received.size()));

    //! [lsl_resolve_stream_usage]
    const std::vector<stream_info> found = resolve_stream("name", "ex_lsl_eeg", 1.5); // multicast discovery
    //! [lsl_resolve_stream_usage]
    if (found.empty()) {
        std::println("  skip  resolve_stream (multicast discovery not available here)");
    } else {
        ok &= expect(found.front().uid() == outlet.info().uid() && found.front().data_port() == outlet.info().data_port(),
                     "resolve_stream finds the outlet by name");
    }

    std::println("{}", ok ? "All lsl checks passed." : "lsl checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
