//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Trace view in an SDL3 window, without Qt: mne_scene's view and software renderer in another toolkit.
 *
 * The same TraceView, TraceBuffer and TraceFrameBuilder that drive the raw browser of mne_disp run in an SDL3 window.
 * Every frame is executed by the software backend of the GPU frame description and shown as an SDL texture; SDL
 * mouse, wheel and key events are translated to scene input events, so wheel zoom, drag scrolling with inertia, the
 * ruler (right button) and the crosshair (c) work as in the Qt host. A synthetic 32-channel recording streams in
 * at 1 kHz and the view follows its end until the user scrolls.
 *
 * With --panel the window shows a tool panel instead (projectors, the default, or compensator) through FormView,
 * which draws and edits any SettingsForm without Qt and without a view written for the panel.
 *
 * Usage: ex_scene_sdl [--panel [projectors|compensator]] [--frames N]; with --frames the example renders N frames
 * and exits (SDL_VIDEODRIVER=dummy runs it without a display).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/sdl/sdl_host.h>
#include <mne/viz/host/trace_view_client.h>
#include <mne/viz/panels/compensator_panel.h>
#include <mne/viz/panels/form_view.h>
#include <mne/viz/panels/projectors_panel.h>
#include <mne/viz/views/trace_buffer.h>

#include <SDL3/SDL_version.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

constexpr int kChannels = 32;
constexpr double kSfreq = 1000.0;

/** The next block of the synthetic recording: alpha-like oscillations, a slow drift and an occasional spike. */
Eigen::MatrixXd synthetic_block(int first, int samples)
{
    Eigen::MatrixXd block(kChannels, samples);
    for (int ch = 0; ch < kChannels; ++ch) {
        const double frequency = 6.0 + 0.4 * ch;
        for (int s = 0; s < samples; ++s) {
            const double t = (first + s) / kSfreq;
            double value = std::sin(2.0 * std::numbers::pi * frequency * t + ch) + 0.3 * std::sin(2.0 * std::numbers::pi * 0.3 * t);
            if ((first + s) % 2000 < 30 && ch % 4 == 0) {
                value += 2.5;
            }
            block(ch, s) = value * 20e-6;
        }
    }
    return block;
}

//=============================================================================================================
/**
 * Shows a tool panel through FormView, the toolkit-free view of any SettingsForm, without a view of its own.
 *
 * @param[in] host      The window.
 * @param[in] which     "projectors" or "compensator".
 *
 * @return The number of frames drawn.
 */
int run_panel(sdl::SdlHost& host, const std::string& which)
{
    //! [form_view_usage]
    std::unique_ptr<Panel> panel;
    if (which == "compensator") {
        auto compensator = std::make_unique<CompensatorPanel>();
        std::vector<mne::FiffCtfComp> comps(3);
        for (int i = 0; i < 3; ++i) {
            comps[static_cast<std::size_t>(i)].kind = i + 1;
        }
        compensator->set_compensators(comps);
        compensator->set_selection_callback([](int to) {
            std::printf("Compensation grade %d\n", to);
        });
        panel = std::move(compensator);
    } else {
        auto projectors = std::make_unique<ProjectorsPanel>();
        std::vector<mne::FiffProj> projs(4);
        const char* descriptions[] = {"Average EEG reference", "PCA-v1 (planar)", "PCA-v2 (axial)", "ECG-planar-999--0.200-0.400-PCA-01"};
        for (std::size_t i = 0; i < projs.size(); ++i) {
            projs[i].desc = descriptions[i];
            projs[i].active = i != 2;
        }
        projectors->set_projectors(projs);
        projectors->set_selection_callback([](const std::vector<mne::FiffProj>& selected) {
            std::printf("Projectors:");
            for (const mne::FiffProj& proj : selected) {
                std::printf(" %d", proj.active ? 1 : 0);
            }
            std::printf("\n");
        });
        panel = std::move(projectors);
    }
    FormView view(*panel);
    return host.run(view);
    //! [form_view_usage]
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    sdl::SdlHost::Options options;
    options.title = "MNE-CPP trace view (SDL3, no Qt)";
    std::string panel;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--panel") == 0) {
            const bool named = i + 1 < argc && std::strncmp(argv[i + 1], "--", 2) != 0;
            panel = named ? argv[++i] : "projectors";
            options.title = "MNE-CPP " + panel + " panel (SDL3, no Qt)";
            options.width = 420;
            options.height = 360;
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            const std::string_view value(argv[i + 1]);
            if (std::from_chars(value.data(), value.data() + value.size(), options.frame_limit).ec != std::errc()) {
                std::fprintf(stderr, "--frames expects a number, got %s\n", argv[i + 1]);
                return 1;
            }
        }
    }

    //! [sdl_host_usage]
    sdl::SdlHost host(options);
    if (!host.is_open()) {
        std::fprintf(stderr, "Cannot open the window: %s\n", host.error().c_str());
        return 1;
    }
    //! [sdl_host_usage]

    if (!panel.empty()) {
        const int frames = run_panel(host, panel);
        std::printf("Rendered %d frames of the %s panel with the SDL3 backend of mne_viz\n", frames, panel.c_str());
        return 0;
    }

    // The channels and the first seconds of the recording
    TraceBuffer buffer;
    std::vector<TraceBuffer::ChannelSpec> specs;
    for (int ch = 0; ch < kChannels; ++ch) {
        TraceBuffer::ChannelSpec spec;
        spec.name = "EEG " + std::to_string(ch + 1);
        spec.type_label = "EEG";
        spec.color = Color{170, 55, 10};
        spec.default_scale = 40e-6f;
        spec.bad = ch == 5;
        specs.push_back(std::move(spec));
    }
    buffer.set_channels(std::move(specs));
    buffer.set_sample_rate(static_cast<float>(kSfreq));
    buffer.set_capacity(60 * static_cast<int>(kSfreq));
    int recorded = 4 * static_cast<int>(kSfreq);
    buffer.set_data(synthetic_block(0, recorded), 0);

    //! [trace_view_client_usage]
    TraceViewClient client;
    client.style().background = Color{250, 250, 250};
    TraceView& view = client.view();
    view.set_sfreq(kSfreq);
    view.set_first_file_sample(0);
    view.set_visible_channel_count(16);
    view.set_samples_per_pixel(5.0);
    view.set_scalebars_visible(true);
    client.set_source(&buffer);
    //! [trace_view_client_usage]

    // The view follows the end of the recording until the user scrolls
    bool followEnd = true;
    bool following = false; // the scroll below is the example's, not the user's
    TraceView::Effects effects;
    effects.scroll_changed = [&](double /*sample*/) {
        followEnd = followEnd && following;
    };
    client.set_effects(std::move(effects));

    host.set_key_handler([&](const KeyEvent& key) {
        if (key.is_character(U'c')) {
            view.set_crosshair_enabled(!view.crosshair_enabled());
            return true;
        }
        if (key.key == Key::End) {
            followEnd = true;
            return true;
        }
        return false;
    });

    const int frames = host.run(client, [&](std::int64_t now) {
        // 1 kHz streaming: append what arrived since the last frame
        const int target = static_cast<int>(4 * kSfreq + static_cast<double>(now));
        if (target > recorded) {
            const int samples = std::min(target - recorded, 200);
            buffer.append_data(synthetic_block(recorded, samples));
            recorded += samples;
            client.data_changed();
        }
        if (followEnd) {
            const double visible = view.view_width() * view.samples_per_pixel();
            following = true;
            view.set_scroll_sample(std::max(0.0, recorded - visible));
            following = false;
        }
    });

    std::printf("Rendered %d frames with the SDL3 backend of mne_viz (SDL %d.%d.%d)\n", frames, SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    return 0;
}
