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
 * Usage: ex_scene_sdl [--frames N]; with --frames the example renders N frames and exits (SDL_VIDEODRIVER=dummy
 * runs it without a display).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/scene/software_renderer.h>
#include <mne/scene/text_metrics.h>
#include <mne/scene/trace_buffer.h>
#include <mne/scene/trace_frame.h>
#include <mne/scene/trace_markers.h>
#include <mne/scene/trace_view.h>
#include <mne/scene/view_animator.h>

#include <SDL3/SDL.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numbers>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

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

mne::scene::Modifier modifiers_of(SDL_Keymod mod)
{
    Modifier result = Modifier::None;
    if ((mod & SDL_KMOD_SHIFT) != 0) {
        result = result | Modifier::Shift;
    }
    if ((mod & SDL_KMOD_CTRL) != 0) {
        result = result | Modifier::Ctrl;
    }
    if ((mod & SDL_KMOD_ALT) != 0) {
        result = result | Modifier::Alt;
    }
    if ((mod & SDL_KMOD_GUI) != 0) {
        result = result | Modifier::Meta;
    }
    return result;
}

MouseButton button_of(std::uint8_t button)
{
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        default:
            return MouseButton::None;
    }
}

MouseButtons buttons_of(SDL_MouseButtonFlags flags)
{
    MouseButtons result = MouseButtons::None;
    if ((flags & SDL_BUTTON_LMASK) != 0) {
        result = result | MouseButtons::Left;
    }
    if ((flags & SDL_BUTTON_RMASK) != 0) {
        result = result | MouseButtons::Right;
    }
    if ((flags & SDL_BUTTON_MMASK) != 0) {
        result = result | MouseButtons::Middle;
    }
    return result;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    int frameLimit = -1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0) {
            frameLimit = std::atoi(argv[i + 1]);
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("MNE-CPP trace view (SDL3, no Qt)", 1000, 640, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    // Any renderer only shows a texture; SDL's software renderer works without a GPU, e.g. on the dummy driver
    SDL_Renderer* sdlRenderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (window && !sdlRenderer) {
        sdlRenderer = SDL_CreateRenderer(window, SDL_SOFTWARE_RENDERER);
    }
    if (!sdlRenderer) {
        std::fprintf(stderr, "Cannot create the window: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
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

    TraceView view;
    ViewAnimator animator;
    TraceFrameBuilder builder;
    bool followEnd = true;
    bool following = false; // the scroll below is the example's, not the user's
    TraceView::Effects effects;
    effects.animate_scroll = [&](const TraceView::ScrollAnimation& animation) {
        animator.start_scroll(animation, static_cast<std::int64_t>(SDL_GetTicks()));
    };
    effects.animate_zoom = [&](const TraceView::ZoomAnimation& animation) {
        animator.start_zoom(animation, static_cast<std::int64_t>(SDL_GetTicks()));
    };
    effects.stop_animations = [&] {
        animator.stop_scroll();
    };
    effects.scroll_changed = [&](double /*sample*/) {
        followEnd = followEnd && following;
    };
    effects.zoom_changed = [&](double /*samples_per_pixel*/) {
        builder.geometry().invalidate();
    };
    effects.channel_offset_changed = [&](int /*first_channel*/) {
        builder.geometry().invalidate();
    };
    view.set_effects(std::move(effects));
    view.set_source(&buffer);
    view.set_sfreq(kSfreq);
    view.set_first_file_sample(0);
    view.set_visible_channel_count(16);
    view.set_samples_per_pixel(5.0);
    view.set_scalebars_visible(true);

    TraceStyle style;
    style.background = Color{250, 250, 250};
    TraceMarkers markers;
    const ApproximateTextMetrics metrics;
    SoftwareRenderer renderer;
    SDL_Texture* texture = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;

    bool running = true;
    for (int frame = 0; running && (frameLimit < 0 || frame < frameLimit); ++frame) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const auto now = static_cast<std::int64_t>(SDL_GetTicks());
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    PointerEvent pointer;
                    pointer.type = event.type == SDL_EVENT_MOUSE_BUTTON_UP ? PointerEvent::Type::Release
                        : event.button.clicks == 2                         ? PointerEvent::Type::DoubleClick
                                                                           : PointerEvent::Type::Press;
                    pointer.x = event.button.x;
                    pointer.y = event.button.y;
                    pointer.button = button_of(event.button.button);
                    pointer.buttons = buttons_of(SDL_GetMouseState(nullptr, nullptr));
                    pointer.modifiers = modifiers_of(SDL_GetModState());
                    pointer.timestamp_ms = now;
                    view.handle_pointer(pointer);
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION: {
                    PointerEvent pointer;
                    pointer.x = event.motion.x;
                    pointer.y = event.motion.y;
                    pointer.buttons = buttons_of(event.motion.state);
                    pointer.modifiers = modifiers_of(SDL_GetModState());
                    pointer.timestamp_ms = now;
                    view.handle_pointer(pointer);
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL: {
                    WheelEvent wheel;
                    wheel.x = event.wheel.mouse_x;
                    wheel.y = event.wheel.mouse_y;
                    // SDL counts notches, the scene eighths of a degree (120 per notch)
                    wheel.angle_delta_x = -120.0 * event.wheel.x;
                    wheel.angle_delta_y = 120.0 * event.wheel.y;
                    wheel.modifiers = modifiers_of(SDL_GetModState());
                    wheel.timestamp_ms = now;
                    view.handle_wheel(wheel);
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_C) {
                        view.set_crosshair_enabled(!view.crosshair_enabled());
                    } else if (event.key.key == SDLK_END) {
                        followEnd = true;
                    } else if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        }

        // 1 kHz streaming: append what arrived since the last frame
        const int target = static_cast<int>(4 * kSfreq + static_cast<double>(SDL_GetTicks()));
        if (target > recorded) {
            const int samples = std::min(target - recorded, 200);
            buffer.append_data(synthetic_block(recorded, samples));
            recorded += samples;
            builder.geometry().invalidate();
        }

        int logicalWidth = 0;
        int logicalHeight = 0;
        int pixelWidth = 0;
        int pixelHeight = 0;
        SDL_GetWindowSize(window, &logicalWidth, &logicalHeight);
        SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight);
        view.set_view_size(logicalWidth, logicalHeight);
        animator.tick(static_cast<std::int64_t>(SDL_GetTicks()), view);
        if (followEnd) {
            const double visible = logicalWidth * view.samples_per_pixel();
            following = true;
            view.set_scroll_sample(std::max(0.0, recorded - visible));
            following = false;
        }

        GpuFrameTarget frameTarget;
        frameTarget.pixel_width = pixelWidth;
        frameTarget.pixel_height = pixelHeight;
        frameTarget.device_pixel_ratio = logicalWidth > 0 ? static_cast<double>(pixelWidth) / logicalWidth : 1.0;
        const Image image = renderer.render(builder.build(view, buffer, style, markers, metrics, frameTarget), builder.resources());

        if (!texture || textureWidth != image.width || textureHeight != image.height) {
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, image.width, image.height);
            textureWidth = image.width;
            textureHeight = image.height;
        }
        SDL_UpdateTexture(texture, nullptr, image.rgba.data(), image.width * 4);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderTexture(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::printf("Rendered with the software backend of mne_scene in SDL %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    return 0;
}
