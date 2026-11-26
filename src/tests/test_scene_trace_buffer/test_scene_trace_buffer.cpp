//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_trace_buffer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests TraceBuffer, SweepBuffer and the evoked condition bookkeeping of mne_scene, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/models/evoked_conditions.h>
#include <mne/viz/views/sweep_buffer.h>
#include <mne/viz/views/trace_buffer.h>
#include <mne/viz/views/trace_geometry.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

int g_failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                                      \
        }                                                                                      \
    } while (false)

bool near(double a, double b, double tolerance = 1e-6)
{
    return std::abs(a - b) <= tolerance;
}

// Channels x samples with value = 10 * channel + sample
Eigen::MatrixXd ramp(int channels, int samples, int first = 0)
{
    Eigen::MatrixXd data(channels, samples);
    for (int ch = 0; ch < channels; ++ch) {
        for (int s = 0; s < samples; ++s)
            data(ch, s) = 10.0 * ch + first + s;
    }
    return data;
}

std::vector<TraceBuffer::ChannelSpec> specs()
{
    TraceBuffer::ChannelSpec grad{"MEG 0112", "MEG grad", Color{20, 90, 180, 255}, {-1, 1}, 4e-11f, false, false};
    TraceBuffer::ChannelSpec eeg{"EEG 001", "EEG", std::nullopt, {2}, 1.f, true, false};
    TraceBuffer::ChannelSpec misc{"MISC 001", "MISC", std::nullopt, {3}, 1.f, false, false};
    TraceBuffer::ChannelSpec derived{"", "", std::nullopt, {}, 0.f, false, true};
    return {grad, eeg, misc, derived};
}

//=============================================================================================================

void test_channel_resolution()
{
    TraceBuffer buffer;
    buffer.set_scale_map({{1, 2e-12f}, {2, 5e-5f}});
    buffer.set_signal_color(Color{0, 128, 0, 255});
    buffer.set_channels(specs());

    CHECK(buffer.channel_count() == 4);
    // The first scale key found wins, else the default; a default <= 0 becomes 1
    CHECK(buffer.channel(0)->amplitude_max == 2e-12f);
    CHECK(buffer.channel(1)->amplitude_max == 5e-5f);
    CHECK(buffer.channel(2)->amplitude_max == 1.f);
    CHECK(buffer.channel(3)->amplitude_max == 1.f);
    CHECK((buffer.channel(0)->color == Color{20, 90, 180, 255}));
    CHECK((buffer.channel(1)->color == Color{0, 128, 0, 255}));
    CHECK(buffer.channel(1)->bad && !buffer.channel(0)->bad);
    CHECK(buffer.channel(3)->name == "Virtual 1" && buffer.channel(3)->type_label == "MISC");
    CHECK(buffer.is_virtual(3) && !buffer.is_virtual(0) && !buffer.is_virtual(9));
    CHECK(!buffer.channel(4) && !buffer.channel(-1));

    // Without the grad key the common key applies
    buffer.set_scale_map({{1, 3e-12f}});
    CHECK(buffer.channel(0)->amplitude_max == 3e-12f && buffer.channel(1)->amplitude_max == 1.f);
    buffer.set_scale_map({{-1, 4e-13f}, {1, 3e-12f}});
    CHECK(buffer.channel(0)->amplitude_max == 4e-13f);
    CHECK(buffer.scale_map().size() == 2);

    // A new signal colour reaches the channels without their own colour
    buffer.set_signal_color(Color{1, 2, 3, 255});
    CHECK((buffer.channel(2)->color == Color{1, 2, 3, 255}) && (buffer.channel(0)->color == Color{20, 90, 180, 255}));

    // Rows beyond the described channels; resolved with the next channel change
    buffer.set_data(ramp(6, 3));
    CHECK(buffer.channel_count() == 6 && !buffer.channel(5));
    buffer.set_signal_color(Color{1, 2, 3, 255});
    CHECK(buffer.channel(5)->name == "CH 6" && buffer.channel(5)->type_label == "MISC" && buffer.channel(5)->amplitude_max == 1.f);
}

//=============================================================================================================

void test_bad_flags_and_provider()
{
    TraceBuffer buffer;
    std::vector<TraceBuffer::ChannelSpec> described = specs();
    buffer.set_spec_provider([&described] { return described; });
    buffer.reload_channels(true);

    // A bad mark holds until the channels are resolved again from the provider
    buffer.set_channel_bad(3, true);
    CHECK(buffer.channel(3)->bad);
    buffer.set_channel_bad(42, true);
    buffer.set_scale_map({});
    CHECK(!buffer.channel(3)->bad);

    // The provider is read on every resolution
    described[0].bad = true;
    buffer.set_signal_color(Color::black());
    CHECK(buffer.channel(0)->bad);
    described.pop_back();
    buffer.reload_channels(false);
    CHECK(buffer.channel_count() == 3);
}

//=============================================================================================================

void test_listeners()
{
    TraceBuffer buffer;
    std::vector<TraceBuffer::Change> changes;
    const int id = buffer.add_listener([&changes](TraceBuffer::Change change) { changes.push_back(change); });
    buffer.set_spec_provider([] { return specs(); });

    using C = TraceBuffer::Change;
    buffer.reload_channels(true);
    CHECK((changes == std::vector<C>{C::Channels}));
    changes.clear();
    buffer.reload_channels(false);
    CHECK((changes == std::vector<C>{C::Channels, C::Data}));
    changes.clear();

    buffer.set_data(Eigen::MatrixXd());
    buffer.append_data(Eigen::MatrixXd(4, 0));
    buffer.set_capacity(10);
    buffer.set_sample_rate(250.f);
    CHECK(changes.empty() && buffer.sample_rate() == 250.f);

    buffer.set_data(ramp(4, 2));
    buffer.append_data(ramp(4, 2));
    buffer.clear_data();
    buffer.set_detrend(Detrend::Mean);
    buffer.set_channel_bad(0, true);
    CHECK((changes == std::vector<C>{C::Data, C::Data, C::Data, C::Data, C::Channels}));
    CHECK(buffer.detrend() == Detrend::Mean);

    buffer.remove_listener(id);
    changes.clear();
    buffer.set_data(ramp(4, 2));
    CHECK(changes.empty());
}

//=============================================================================================================

void test_samples()
{
    TraceBuffer buffer;
    buffer.set_data(ramp(2, 5), 100);
    CHECK(buffer.first_sample() == 100 && buffer.sample_count() == 5);
    CHECK(buffer.value_at(1, 102) == 12.f && buffer.value_at(1, 99) == 0.f && buffer.value_at(1, 105) == 0.f);
    CHECK(buffer.value_at(2, 100) == 0.f && buffer.value_at(-1, 100) == 0.f);

    // Appending beyond the capacity drops the oldest samples
    buffer.set_capacity(6);
    buffer.append_data(ramp(2, 3, 5));
    CHECK(buffer.sample_count() == 6 && buffer.first_sample() == 102);
    CHECK(buffer.value_at(0, 102) == 2.f && buffer.value_at(0, 107) == 7.f);

    buffer.clear_data();
    CHECK(buffer.sample_count() == 0 && buffer.first_sample() == 0 && buffer.channel_count() == 2);
}

//=============================================================================================================

void test_vertices_and_rms()
{
    TraceBuffer buffer;
    buffer.set_data(ramp(1, 20), 10);

    // A window from before the stored samples starts at the first one
    TraceVertices vertices = buffer.vertices(0, 5, 15, 100);
    CHECK(vertices.first_sample == 10 && vertices.vertex_count() == 5);
    CHECK(vertices.xy[0] == 0.f && vertices.xy[1] == 0.f && vertices.xy[9] == 4.f);

    // Detrending and decimation are those of decimate_trace()
    buffer.set_detrend(Detrend::Linear);
    const std::vector<float> samples{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    vertices = buffer.vertices(0, 10, 30, 4);
    CHECK(vertices.first_sample == 10 && vertices.xy == decimate_trace(samples, 4, Detrend::Linear));

    CHECK(buffer.vertices(0, 40, 50, 10).xy.empty() && buffer.vertices(0, 40, 50, 10).first_sample == 30);
    CHECK(buffer.vertices(0, 12, 12, 10).xy.empty() && buffer.vertices(0, 10, 20, 0).xy.empty());
    CHECK(buffer.vertices(3, 10, 20, 10).xy.empty() && buffer.vertices(3, 10, 20, 10).first_sample == 10);

    // RMS over the window, from at most its last 1000 samples
    Eigen::MatrixXd data = Eigen::MatrixXd::Constant(1, 1500, 3.0);
    data.leftCols(500).setConstant(100.0);
    buffer.set_data(data);
    CHECK(near(buffer.rms(0, 500, 1500), 3.0));
    CHECK(near(buffer.rms(0, -100, 2000), 3.0));
    CHECK(near(buffer.rms(0, 0, 2), 100.0));
    CHECK(buffer.rms(0, 10, 10) == 0.f && buffer.rms(1, 0, 10) == 0.f);
}

//=============================================================================================================

void test_wrapped_span()
{
    std::vector<std::tuple<int, int, int>> pieces;
    const auto collect = [&pieces](int column, int offset, int length) {
        pieces.emplace_back(column, offset, length);
    };
    for_wrapped_span(8, 5, 10, collect);
    CHECK((pieces == std::vector<std::tuple<int, int, int>>{{8, 0, 2}, {0, 2, 3}}));
    pieces.clear();
    for_wrapped_span(-3, 4, 10, collect);
    CHECK((pieces == std::vector<std::tuple<int, int, int>>{{7, 0, 3}, {0, 3, 1}}));
    pieces.clear();
    for_wrapped_span(2, 25, 10, collect);
    CHECK(pieces.size() == 3 && std::get<2>(pieces[0]) == 8 && std::get<2>(pieces[2]) == 7);
}

//=============================================================================================================

void test_sweep()
{
    SweepBuffer sweep;
    CHECK(sweep.window() == 1024 && sweep.sample_at_column(5) == -1);
    sweep.resize(2, 5, true);

    int sweeps = 0;
    int sample = 0;
    const auto write = [&](int count) {
        const Eigen::MatrixXd block = ramp(2, count, sample);
        sweep.write(count, [&sweeps] { ++sweeps; }, [&](int column, int offset, int length) {
            sweep.raw().middleCols(column, length) = block.middleCols(offset, length);
            sweep.filtered().middleCols(column, length) = -block.middleCols(offset, length); });
        sample += count;
    };

    write(3);
    CHECK(sweep.write_position() == 3 && sweep.sweep_start() == 0 && sweeps == 0 && sweep.block_size() == 3);
    CHECK(sweep.sample_at_column(0) == 0 && sweep.sample_at_column(2) == 2 && sweep.sample_at_column(3) == -1);

    // The end of the window is reached first, the wrap starts the new sweep
    write(2);
    CHECK(sweep.write_position() == 5 && sweeps == 0);
    write(4);
    CHECK(sweeps == 1 && sweep.write_position() == 4 && sweep.sweep_start() == 5);
    CHECK(sweep.first_value(1, false) == 10.0 && sweep.first_value(1, true) == -10.0 && sweep.first_value(2, false) == 0.0);
    CHECK(sweep.raw()(0, 0) == 5.0 && sweep.raw()(0, 4) == 4.0);
    CHECK(sweep.sample_at_column(3) == 8 && sweep.sample_at_column(4) == 4 && sweep.sample_at_column(5) == -1);

    // The last block unwrapped
    const Eigen::MatrixXd block = sweep.last_block(false);
    CHECK(block.cols() == 4 && block(0, 0) == 5.0 && block(1, 3) == 18.0);
    CHECK(sweep.last_block(true)(0, 3) == -8.0);

    // A block across the wrap
    write(3);
    CHECK(sweeps == 2 && sweep.write_position() == 2 && sweep.sweep_start() == 10);
    const Eigen::MatrixXd wrapped = sweep.last_block(false);
    CHECK(wrapped(0, 0) == 9.0 && wrapped(0, 1) == 10.0 && wrapped(0, 2) == 11.0);

    // Freezing keeps the layers and write position for display while writing goes on
    sweep.set_frozen(true);
    write(2);
    CHECK(sweep.frozen() && sweep.frozen_write_position() == 2 && sweep.write_position() == 4);
    CHECK(sweep.layer(false)(0, 2) == 7.0 && sweep.raw()(0, 2) == 12.0 && sweep.layer(true)(0, 2) == -7.0);
    CHECK(sweep.sample_at_column(2) == 7);
    write(3);
    CHECK(sweeps == 3 && sweep.first_value(0, false) == 5.0);
    sweep.set_frozen(false);
    CHECK(&sweep.layer(false) == &sweep.raw() && &sweep.layer(true) == &sweep.filtered());

    // Shrinking the window behind the write position starts a new sweep, keeping the values
    CHECK(sweep.write_position() == 2);
    sweep.resize(2, 1, false);
    CHECK(sweep.window() == 1 && sweep.write_position() == 0 && sweep.sweep_start() == 17 && sweep.raw()(0, 0) == 15.0);

    sweep.clear();
    CHECK(sweep.raw().isZero() && sweep.first_value(0, false) == 0.0);
}

//=============================================================================================================

void test_evoked_conditions()
{
    std::map<std::string, int> current;
    std::map<std::string, int> retired;
    CHECK(reconcile_conditions(current, retired, {"aud", "vis"}, 1));
    CHECK(current.size() == 2 && current["aud"] == 1);

    current["aud"] = 7;
    CHECK(reconcile_conditions(current, retired, {"vis"}, 1));
    CHECK(current.size() == 1 && retired["aud"] == 7);

    // A condition shown again gets its last setting back
    CHECK(reconcile_conditions(current, retired, {"vis", "aud"}, 1));
    CHECK(current["aud"] == 7);
    CHECK(!reconcile_conditions(current, retired, {"aud", "vis"}, 1));

    // Swapping one condition for another keeps the count
    CHECK(!reconcile_conditions(current, retired, {"aud", "som"}, 3));
    CHECK(current["som"] == 3 && retired.contains("vis"));

    const std::vector<float> times{-0.2f, -0.1f, 0.0f, 0.1f};
    CHECK(pre_stimulus_samples(times) == 2);
    const std::vector<float> after{0.1f, 0.2f};
    CHECK(pre_stimulus_samples(after) == 2);
    CHECK(pre_stimulus_samples({}) == 0);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"channel_resolution", test_channel_resolution},
        {"bad_flags_and_provider", test_bad_flags_and_provider},
        {"listeners", test_listeners},
        {"samples", test_samples},
        {"vertices_and_rms", test_vertices_and_rms},
        {"wrapped_span", test_wrapped_span},
        {"sweep", test_sweep},
        {"evoked_conditions", test_evoked_conditions},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
