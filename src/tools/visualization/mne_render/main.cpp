//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Renders raw data traces or a surface with a source estimate to PNG or SVG, without Qt or a GPU.
 *
 * mne_render drives the views of mne_scene headless: the trace view (TraceView, TraceBuffer, TraceFrameBuilder)
 * for a raw FIFF file and the brain view (BrainFrameBuilder) for a FreeSurfer surface with an optional source
 * estimate, both executed by the software backend of the GPU frame description.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/raster_image.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fs/fs_surface.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/viz/gpu/brain_frame.h>
#include <mne/viz/views/camera.h>
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/colormap.h>
#include <mne/viz/gpu/software_renderer.h>
#include <mne/viz/figures/svg_renderer.h>
#include <mne/viz/figures/font_library.h>
#include <mne/viz/figures/font_text_metrics.h>
#include <mne/viz/figures/text_metrics.h>
#include <mne/viz/views/trace_buffer.h>
#include <mne/viz/gpu/trace_frame.h>
#include <mne/viz/views/trace_geometry.h>
#include <mne/viz/views/trace_markers.h>
#include <mne/viz/views/trace_view.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <print>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

namespace
{

/** Trace colour, type label and amplitude scale of a channel, as the raw browser shows them. */
TraceBuffer::ChannelSpec channel_spec(const FiffChInfo& info, const std::string& name, bool bad)
{
    TraceBuffer::ChannelSpec spec;
    spec.name = name;
    spec.bad = bad;
    switch (info.kind) {
        case FIFFV_MEG_CH:
            spec.type_label = info.unit == FIFF_UNIT_T_M ? "MEG grad" : "MEG mag";
            spec.color = Color{20, 90, 180};
            spec.default_scale = info.unit == FIFF_UNIT_T_M ? 400e-13f : 1.2e-12f;
            break;
        case FIFFV_EEG_CH:
            spec.type_label = "EEG";
            spec.color = Color{170, 55, 10};
            spec.default_scale = 30e-6f;
            break;
        case FIFFV_EOG_CH:
            spec.type_label = "EOG";
            spec.color = Color{130, 0, 130};
            spec.default_scale = 150e-6f;
            break;
        case FIFFV_ECG_CH:
            spec.type_label = "ECG";
            spec.color = Color{190, 15, 45};
            spec.default_scale = 1e-3f;
            break;
        case FIFFV_EMG_CH:
            spec.type_label = "EMG";
            spec.color = Color{20, 110, 20};
            spec.default_scale = 1e-3f;
            break;
        case FIFFV_STIM_CH:
            spec.type_label = "STIM";
            spec.color = Color{180, 100, 0};
            spec.default_scale = 5.f;
            break;
        default:
            spec.type_label = "MISC";
            spec.default_scale = 1.f;
            break;
    }
    return spec;
}

/** Sets the scale of every channel type to five times the median standard deviation of its channels. */
void autoscale(std::vector<TraceBuffer::ChannelSpec>& specs, const Eigen::MatrixXd& data)
{
    std::map<std::string, std::vector<double>> deviations;
    for (std::size_t ch = 0; ch < specs.size() && static_cast<Eigen::Index>(ch) < data.rows(); ++ch) {
        const Eigen::RowVectorXd row = data.row(static_cast<Eigen::Index>(ch));
        const double deviation = std::sqrt((row.array() - row.mean()).square().mean());
        if (deviation > 0.0) {
            deviations[specs[ch].type_label].push_back(deviation);
        }
    }
    for (TraceBuffer::ChannelSpec& spec : specs) {
        std::vector<double>& values = deviations[spec.type_label];
        if (!values.empty()) {
            std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2), values.end());
            spec.default_scale = static_cast<float>(5.0 * values[values.size() / 2]);
        }
    }
}

/** RGB of an RGBA image on its own (opaque) background. */
RasterImage to_rgb(const Image& image)
{
    RasterImage rgb(image.width, image.height);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Color c = image.pixel(x, y);
            rgb.set_pixel(x, y, {c.r, c.g, c.b});
        }
    }
    return rgb;
}

bool write_text(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
    return static_cast<bool>(out.flush());
}

bool is_svg(const std::string& file)
{
    return file.size() > 4 && to_lower(file.substr(file.size() - 4)) == ".svg";
}

//=============================================================================================================

int render_raw(const CommandLineParser& parser, const std::string& outFile, int width, int height, double scale)
{
    const std::string rawFile = parser.value("raw");
    FiffRawData raw(path_from_utf8(rawFile));
    if (raw.info.nchan <= 0) {
        Log::critical("Cannot read raw data from {}", rawFile);
        return 1;
    }
    const double sfreq = raw.info.sfreq;
    const double start = to_double(parser.value("start")).value_or(0.0);
    const double duration = std::max(to_double(parser.value("duration")).value_or(10.0), 1.0 / sfreq);
    const int first = std::clamp(raw.first_samp + static_cast<int>(std::lround(start * sfreq)), raw.first_samp, raw.last_samp);
    const int last = std::clamp(first + static_cast<int>(std::lround(duration * sfreq)) - 1, first, raw.last_samp);

    Eigen::MatrixXd data;
    Eigen::MatrixXd times;
    if (!raw.read_raw_segment(data, times, first, last)) {
        Log::critical("Cannot read samples {} to {} of {}", first, last, rawFile);
        return 1;
    }

    TraceBuffer buffer;
    std::vector<TraceBuffer::ChannelSpec> specs;
    for (int ch = 0; ch < raw.info.nchan; ++ch) {
        const std::string& name = raw.info.ch_names[static_cast<std::size_t>(ch)];
        specs.push_back(channel_spec(raw.info.chs[static_cast<std::size_t>(ch)], name, std::ranges::contains(raw.info.bads, name)));
    }
    if (parser.is_set("autoscale")) {
        autoscale(specs, data);
    }
    buffer.set_channels(std::move(specs));
    buffer.set_sample_rate(static_cast<float>(sfreq));
    buffer.set_data(data, first);

    TraceView view;
    view.set_source(&buffer);
    view.set_view_size(width, height);
    view.set_sfreq(sfreq);
    view.set_first_file_sample(raw.first_samp);
    view.set_last_file_sample(raw.last_samp);
    view.set_samples_per_pixel(static_cast<double>(last - first + 1) / width);
    view.set_scroll_sample(first);
    view.set_visible_channel_count(std::max(to_int(parser.value("channels")).value_or(20), 1));
    view.set_first_visible_channel(std::max(to_int(parser.value("first-channel")).value_or(0), 0));
    view.set_scalebars_visible(parser.is_set("scalebars"));
    TraceStyle style;
    style.background = Color::white();
    const TraceMarkers markers;
    // PNG text in the bundled fonts on request; the default bitmap font keeps images identical across versions
    const bool fonts = parser.is_set("fonts") && !is_svg(outFile);
    const ApproximateTextMetrics approximateMetrics;
    const FontTextMetrics fontMetrics(FontLibrary::shared());
    const TextMetrics& metrics = fonts ? static_cast<const TextMetrics&>(fontMetrics) : approximateMetrics;

    if (is_svg(outFile)) {
        Canvas canvas(width, height, style.background);
        draw_trace_figure(canvas, metrics, Font{}, view, buffer, markers, style);
        if (view.overlays_visible()) {
            view.draw_overlays(canvas, metrics);
        }
        if (!write_text(path_from_utf8(outFile), to_svg(canvas.take()))) {
            Log::critical("Cannot write {}", outFile);
            return 1;
        }
    } else {
        TraceFrameBuilder builder;
        GpuFrameTarget target;
        target.pixel_width = static_cast<int>(std::lround(width * scale));
        target.pixel_height = static_cast<int>(std::lround(height * scale));
        target.device_pixel_ratio = scale;
        const GpuFrame frame = builder.build(view, buffer, style, markers, metrics, target);
        SoftwareRenderer renderer;
        if (fonts) {
            renderer.set_fonts(FontLibrary::shared());
        }
        const Expected<void> written = to_rgb(renderer.render(frame, builder.resources())).save_png(path_from_utf8(outFile));
        if (!written) {
            Log::critical("{}", written.error().what());
            return 1;
        }
    }
    Log::info("Rendered {} channels, samples {} to {}, into {}", raw.info.nchan, first, last, outFile);
    return 0;
}

//=============================================================================================================

/**
 * Spreads the values of the source vertices over the surface: every step gives each vertex without a value the
 * mean of its neighbours that have one, as the smoothing steps of MNE-C.
 */
Eigen::VectorXf spread(const FsSurface& surface, const Eigen::VectorXi& vertices, const Eigen::VectorXd& values, int steps)
{
    const auto count = static_cast<std::size_t>(surface.rr().rows());
    std::vector<std::vector<int>> neighbours(count);
    for (Eigen::Index t = 0; t < surface.tris().rows(); ++t) {
        for (int k = 0; k < 3; ++k) {
            const int a = surface.tris()(t, k);
            const int b = surface.tris()(t, (k + 1) % 3);
            neighbours[static_cast<std::size_t>(a)].push_back(b);
            neighbours[static_cast<std::size_t>(b)].push_back(a);
        }
    }
    Eigen::VectorXf value = Eigen::VectorXf::Zero(static_cast<Eigen::Index>(count));
    std::vector<bool> known(count, false);
    for (Eigen::Index i = 0; i < vertices.size() && i < values.size(); ++i) {
        if (vertices[i] >= 0 && static_cast<std::size_t>(vertices[i]) < count) {
            value[vertices[i]] = static_cast<float>(values[i]);
            known[static_cast<std::size_t>(vertices[i])] = true;
        }
    }
    for (int step = 0; step < steps; ++step) {
        Eigen::VectorXf next = value;
        std::vector<bool> nextKnown = known;
        for (std::size_t v = 0; v < count; ++v) {
            if (known[v]) {
                continue;
            }
            float sum = 0.f;
            int n = 0;
            for (const int w : neighbours[v]) {
                if (known[static_cast<std::size_t>(w)]) {
                    sum += value[w];
                    ++n;
                }
            }
            if (n > 0) {
                next[static_cast<Eigen::Index>(v)] = sum / static_cast<float>(n);
                nextKnown[v] = true;
            }
        }
        value = std::move(next);
        known = std::move(nextKnown);
    }
    return value;
}

int render_brain(const CommandLineParser& parser, const std::string& outFile, int width, int height, double scale)
{
    if (is_svg(outFile)) {
        Log::critical("The brain view renders PNG files only");
        return 1;
    }
    const std::string surfFile = parser.value("surf");
    FsSurface surface;
    if (!FsSurface::read(path_from_utf8(surfFile), surface, true) || surface.rr().rows() == 0) {
        Log::critical("Cannot read the surface {}", surfFile);
        return 1;
    }
    const Eigen::Index vertexCount = surface.rr().rows();

    // Source estimate at the requested time, thresholded and mapped to a colour scale
    Eigen::VectorXf activity = Eigen::VectorXf::Zero(vertexCount);
    float fmax = 0.f;
    if (parser.is_set("stc")) {
        const Expected<InvSourceEstimate> stc = InvSourceEstimate::read(path_from_utf8(parser.value("stc")));
        if (!stc || stc->data.cols() == 0) {
            Log::critical("Cannot read the source estimate {}", parser.value("stc"));
            return 1;
        }
        const double time = to_double(parser.value("time")).value_or(stc->tmin);
        const auto column = static_cast<Eigen::Index>(
            std::clamp<double>(std::lround((time - stc->tmin) / std::max(stc->tstep, 1e-9f)), 0.0, static_cast<double>(stc->data.cols() - 1)));
        activity = spread(surface, stc->vertices, stc->data.col(column).cwiseAbs(), std::max(to_int(parser.value("smooth")).value_or(10), 0));
        fmax = static_cast<float>(to_double(parser.value("fmax")).value_or(activity.maxCoeff()));
    }
    const float fthresh = static_cast<float>(to_double(parser.value("fthresh")).value_or(0.3 * fmax));
    const std::optional<Colormap> colormap = colormap_from_name(parser.value("colormap"));
    if (!colormap) {
        Log::critical("Unknown colour map {}", parser.value("colormap"));
        return 1;
    }

    std::vector<float> geometry;
    std::vector<std::uint32_t> colors;
    geometry.reserve(static_cast<std::size_t>(vertexCount) * 6);
    for (Eigen::Index v = 0; v < vertexCount; ++v) {
        const Eigen::Vector3f position = surface.rr().row(v).transpose() + surface.offset();
        const Eigen::Vector3f normal = surface.nn().row(v);
        geometry.insert(geometry.end(), {position.x(), position.y(), position.z(), normal.x(), normal.y(), normal.z()});
        const bool sulcus = v < surface.curv().size() && surface.curv()[v] > 0.f;
        const std::uint8_t grey = sulcus ? 90 : 160;
        Color color{grey, grey, grey};
        if (fmax > 0.f && activity[v] >= fthresh) {
            color = map_color(*colormap, std::clamp((activity[v] - fthresh) / std::max(fmax - fthresh, 1e-12f), 0.f, 1.f));
        }
        colors.push_back(std::uint32_t{color.r} | (std::uint32_t{color.g} << 8) | (std::uint32_t{color.b} << 16) | (std::uint32_t{grey} << 24));
    }
    const std::vector<std::uint32_t> annotations(static_cast<std::size_t>(vertexCount), 0u);
    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(surface.tris().size()));
    for (Eigen::Index t = 0; t < surface.tris().rows(); ++t) {
        for (int k = 0; k < 3; ++k) {
            indices.push_back(static_cast<std::uint32_t>(surface.tris()(t, k)));
        }
    }
    BrainSurfaceStreams streams;
    streams.geometry = {std::as_bytes(std::span(geometry)), 1};
    streams.colors = {std::as_bytes(std::span(colors)), 1};
    streams.annotations = {std::as_bytes(std::span(annotations)), 1};
    streams.indices = {std::as_bytes(std::span(indices)), 1};
    streams.index_count = static_cast<std::uint32_t>(indices.size());

    // Camera on the surface's bounding box from the requested side
    const std::string viewName = to_lower(parser.value("view"));
    int preset = -1;
    for (int p = 0; p < view_preset_count; ++p) {
        if (to_lower(std::string(view_preset_name(p))) == viewName) {
            preset = p;
        }
    }
    if (preset < 0) {
        Log::critical("Unknown view {}", parser.value("view"));
        return 1;
    }
    Eigen::Vector3f low = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
    Eigen::Vector3f high = -low;
    for (Eigen::Index v = 0; v < vertexCount; ++v) {
        const Eigen::Vector3f position = surface.rr().row(v).transpose() + surface.offset();
        low = low.cwiseMin(position);
        high = high.cwiseMax(position);
    }
    OrbitCamera camera;
    camera.set_scene_center((low + high) * 0.5f);
    camera.set_scene_size((high - low).maxCoeff());
    PaneCamera pane;
    pane.preset = preset;
    const CameraMatrices matrices = camera.pane_view(pane, static_cast<float>(width) / static_cast<float>(height));

    BrainSurfaceUniforms uniforms;
    const Eigen::Matrix4f pvm = matrices.pvm();
    std::copy(pvm.data(), pvm.data() + 16, uniforms.mvp);
    std::copy(matrices.eye.data(), matrices.eye.data() + 3, uniforms.camera_pos);
    const Eigen::Vector3f light = matrices.eye.normalized();
    std::copy(light.data(), light.data() + 3, uniforms.light_dir);
    uniforms.overlay_mode = 3.f;

    GpuFrameTarget target;
    target.pixel_width = static_cast<int>(std::lround(width * scale));
    target.pixel_height = static_cast<int>(std::lround(height * scale));
    target.device_pixel_ratio = scale;
    BrainFrameBuilder builder;
    builder.begin(target, Color::black());
    builder.draw_surface(1, streams, BrainShader::Standard, uniforms, GpuRect{0, 0, target.pixel_width, target.pixel_height});
    const GpuFrame frame = builder.finish();
    SoftwareRenderer renderer;
    const Expected<void> written = to_rgb(renderer.render(frame, builder.resources())).save_png(path_from_utf8(outFile));
    if (!written) {
        Log::critical("{}", written.error().what());
        return 1;
    }
    Log::info("Rendered {} vertices into {}", vertexCount, outFile);
    return 0;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Render raw data traces or a surface with a source estimate to PNG or SVG, without Qt or a GPU.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"raw"}, "Raw FIFF file whose traces are rendered.", "file", {}});
    parser.add_option({{"surf"}, "FreeSurfer surface to render, e.g. lh.white (the curvature next to it is read too).", "file", {}});
    parser.add_option({{"out"}, "Output image, .png or (raw data only) .svg.", "file", {}});
    parser.add_option({{"width"}, "Image width in pixels.", "pixels", "1200"});
    parser.add_option({{"height"}, "Image height in pixels.", "pixels", "800"});
    parser.add_option({{"scale"}, "Pixels per figure pixel of a PNG, e.g. 2 for high resolution.", "factor", "1"});
    parser.add_option({{"start"}, "Raw data: first second shown, from the start of the file.", "sec", "0"});
    parser.add_option({{"duration"}, "Raw data: seconds shown.", "sec", "10"});
    parser.add_option({{"channels"}, "Raw data: number of channel rows.", "n", "20"});
    parser.add_option({{"first-channel"}, "Raw data: first channel row.", "n", "0"});
    parser.add_option({{"autoscale"}, "Raw data: scale every channel type to its signals instead of the raw browser's defaults.", {}, {}});
    parser.add_option({{"scalebars"}, "Raw data: draw scalebars.", {}, {}});
    parser.add_option({{"fonts"}, "Raw data: draw the text of a PNG anti-aliased in the bundled fonts instead of the bitmap font.", {}, {}});
    parser.add_option({{"stc"}, "Surface: source estimate (.stc) of the hemisphere.", "file", {}});
    parser.add_option({{"time"}, "Surface: time of the source estimate in seconds (default: the first sample).", "sec", {}});
    parser.add_option({{"smooth"}, "Surface: smoothing steps that spread the source values over the surface.", "n", "10"});
    parser.add_option({{"fthresh"}, "Surface: lowest absolute value shown (default: 30 % of fmax).", "value", {}});
    parser.add_option({{"fmax"}, "Surface: value at the top of the colour scale (default: the largest value).", "value", {}});
    parser.add_option({{"colormap"}, "Surface: colour map (Hot, Jet, Viridis, ...).", "name", "Hot"});
    parser.add_option({{"view"}, "Surface: camera, one of top, perspective, front, left, bottom, back, right.", "name", "left"});
    parser.process(argc, argv, "mne_render", PROGRAM_VERSION);

    const std::string outFile = parser.value("out");
    if (outFile.empty() || parser.is_set("raw") == parser.is_set("surf")) {
        Log::critical("Give --out and either --raw or --surf.");
        std::print("{}", parser.help_text("mne_render"));
        return 1;
    }
    const int width = std::clamp(to_int(parser.value("width")).value_or(1200), 16, 16384);
    const int height = std::clamp(to_int(parser.value("height")).value_or(800), 16, 16384);
    const double scale = std::clamp(to_double(parser.value("scale")).value_or(1.0), 0.25, 8.0);
    return parser.is_set("raw") ? render_raw(parser, outFile, width, height, scale) : render_brain(parser, outFile, width, height, scale);
}
