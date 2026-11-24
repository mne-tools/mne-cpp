//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     January 2002
 * @brief    Export source estimate frames as image sequences from STC data on cortical surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/mne/mne.h>
#include <mne/mne/mne_inverse_operator.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_label.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/base/command_line.h>
#include <mne/base/raster_image.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static Rgb valueToColor(double value, double fthresh, [[maybe_unused]] double fmid, double fmax, bool isSigned)
{
    double absVal = std::abs(value);
    if (absVal < fthresh)
        return {128, 128, 128};

    // Interpolate between hot colors
    double t = 0.0;
    if (fmax > fthresh) {
        t = std::clamp((absVal - fthresh) / (fmax - fthresh), 0.0, 1.0);
    }

    int r, g, b;
    if (t < 0.5) {
        double s = t / 0.5;
        r = static_cast<int>(255 * s);
        g = static_cast<int>(255 * s * 0.5);
        b = 0;
    } else {
        double s = (t - 0.5) / 0.5;
        r = 255;
        g = static_cast<int>(128 + 127 * s);
        b = static_cast<int>(255 * s);
    }

    if (isSigned && value < 0) {
        // Cool colors for negative values
        std::swap(r, b);
    }

    return {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
}

//=============================================================================================================

static RasterImage renderFrame(const FsSurface& surf,
                               const VectorXd& values,
                               int width, int height,
                               double fthresh, double fmid, double fmax,
                               bool isSigned,
                               const std::string& viewName)
{
    RasterImage image(width, height);

    const MatrixX3f& rr = surf.rr();
    const MatrixX3i& tris = surf.tris();

    if (rr.rows() == 0 || tris.rows() == 0)
        return image;

    // Simple orthographic projection
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    for (int i = 0; i < rr.rows(); ++i) {
        cx += rr(i, 0);
        cy += rr(i, 1);
        cz += rr(i, 2);
    }
    cx /= rr.rows();
    cy /= rr.rows();
    cz /= rr.rows();

    // View rotation
    int xIdx = 0, yIdx = 1;
    float flipX = 1.0f, flipY = -1.0f;
    if (viewName == "lat" || viewName == "lateral") {
        xIdx = 1;
        yIdx = 2;
    } else if (viewName == "med" || viewName == "medial") {
        xIdx = 1;
        yIdx = 2;
        flipX = -1.0f;
    } else if (viewName == "dor" || viewName == "dorsal") {
        xIdx = 0;
        yIdx = 1;
    } else if (viewName == "ven" || viewName == "ventral") {
        xIdx = 0;
        yIdx = 1;
        flipY = 1.0f;
    } else if (viewName == "ros" || viewName == "rostral") {
        xIdx = 0;
        yIdx = 2;
    } else if (viewName == "cau" || viewName == "caudal") {
        xIdx = 0;
        yIdx = 2;
        flipX = -1.0f;
    }

    // Find bounds
    float minX = 1e10f, maxX = -1e10f, minY = 1e10f, maxY = -1e10f;
    for (int i = 0; i < rr.rows(); ++i) {
        float px = flipX * (rr(i, xIdx) - (xIdx == 0 ? cx : (xIdx == 1 ? cy : cz)));
        float py = flipY * (rr(i, yIdx) - (yIdx == 0 ? cx : (yIdx == 1 ? cy : cz)));
        if (px < minX)
            minX = px;
        if (px > maxX)
            maxX = px;
        if (py < minY)
            minY = py;
        if (py > maxY)
            maxY = py;
    }

    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float scale = std::min((width - 20) / rangeX, (height - 20) / rangeY);

    // Draw triangles
    for (int t = 0; t < tris.rows(); ++t) {
        int i0 = tris(t, 0), i1 = tris(t, 1), i2 = tris(t, 2);

        // Average value for this triangle
        double triVal = 0.0;
        int count = 0;
        if (i0 < values.size()) {
            triVal += values(i0);
            count++;
        }
        if (i1 < values.size()) {
            triVal += values(i1);
            count++;
        }
        if (i2 < values.size()) {
            triVal += values(i2);
            count++;
        }
        if (count > 0)
            triVal /= count;

        const Rgb color = valueToColor(triVal, fthresh, fmid, fmax, isSigned);

        auto project = [&](int idx) -> PointF {
            float px = flipX * (rr(idx, xIdx) - (xIdx == 0 ? cx : (xIdx == 1 ? cy : cz)));
            float py = flipY * (rr(idx, yIdx) - (yIdx == 0 ? cx : (yIdx == 1 ? cy : cz)));
            return {(px - minX) * scale + 10, (py - minY) * scale + 10};
        };

        const std::array<PointF, 3> tri = {project(i0), project(i1), project(i2)};
        image.fill_polygon(tri, color);
    }

    return image;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "Export source estimate frames as image sequences from STC data on cortical surfaces.\n\n"
        "This tool renders source activity overlaid on a cortical surface and exports\n"
        "individual frames as PNG images. The frame sequence can be assembled into a\n"
        "movie using external tools (e.g. ffmpeg).\n\n"
        "Port of the MNE-C mne_make_movie utility.");
    parser.add_help_option();
    parser.add_version_option();

    // Input options
    parser.add_option({{"stcin"}, "Input STC file prefix (left hemisphere assumed; -rh.stc auto-loaded).", "prefix", {}});
    parser.add_option({{"inv"}, "Inverse operator file (alternative to --stcin: compute from data).", "file", {}});
    parser.add_option({{"meas"}, "Measurement file (used with --inv).", "file", {}});
    parser.add_option({{"set"}, "Data set number (default: 0).", "number", "0"});
    parser.add_option({{"snr"}, "SNR for regularization (default: 3).", "value", "3.0"});
    parser.add_option({{"method"}, "Inverse method: MNE, dSPM, sLORETA (default: dSPM).", "name", "dSPM"});

    // Time windowing
    parser.add_option({{"tmin"}, "Start time in seconds.", "seconds", {}});
    parser.add_option({{"tmax"}, "End time in seconds.", "seconds", {}});
    parser.add_option({{"tstep"}, "Time step between frames in seconds.", "seconds", {}});

    // Surface options
    parser.add_option({{"subject"}, "FreeSurfer subject name.", "name", {}});
    parser.add_option({{"subjects-dir"}, "FreeSurfer SUBJECTS_DIR (default: $SUBJECTS_DIR).", "dir", {}});
    parser.add_option({{"surface"}, "Surface to use (default: inflated).", "name", "inflated"});
    parser.add_option({{"lh"}, "Left hemisphere only.", {}, {}});
    parser.add_option({{"rh"}, "Right hemisphere only.", {}, {}});
    parser.add_option({{"view"}, "View: lat, med, dor, ven, ros, cau (default: lat).", "name", "lat"});

    // Color scaling
    parser.add_option({{"fthresh"}, "Activation threshold (default: 3.0).", "value", "3.0"});
    parser.add_option({{"fmid"}, "Mid-scale value (default: 6.0).", "value", "6.0"});
    parser.add_option({{"fmax"}, "Maximum scale value (default: 10.0).", "value", "10.0"});
    parser.add_option({{"signed"}, "Show signed values (positive + negative).", {}, {}});

    // Output options
    parser.add_option({{"png"}, "Output PNG prefix.", "prefix", {}});
    parser.add_option({{"width"}, "Image width in pixels (default: 800).", "pixels", "800"});
    parser.add_option({{"height"}, "Image height in pixels (default: 600).", "pixels", "600"});

    // Label options
    parser.add_option({{"label"}, "Label file(s) for ROI extraction.", "file", {}});
    parser.add_option({{"labelout"}, "Output text file for label time courses.", "file", {}});

    parser.process(argc, argv, "mne_make_movie", PROGRAM_VERSION);

    // Numbers as QString::toInt / toDouble read them: 0 when the text is not a number
    const auto intValue = [&parser](const char* name) {
        return to_int(parser.value(name)).value_or(0);
    };
    const auto doubleValue = [&parser](const char* name) {
        return to_double(parser.value(name)).value_or(0.0);
    };

    //=========================================================================
    // Resolve subjects directory
    //=========================================================================
    std::string subjectsDir = parser.value("subjects-dir");
    if (subjectsDir.empty()) {
        const char* env = std::getenv("SUBJECTS_DIR");
        subjectsDir = env ? env : "";
    }

    std::string subject = parser.value("subject");
    const std::string surfName = parser.value("surface");
    const std::string viewName = parser.value("view");

    const int width = intValue("width");
    const int height = intValue("height");
    const double fthresh = doubleValue("fthresh");
    const double fmid = doubleValue("fmid");
    const double fmax = doubleValue("fmax");
    const bool isSigned = parser.is_set("signed");

    //=========================================================================
    // Load source estimate
    //=========================================================================
    InvSourceEstimate stcLH, stcRH;

    if (parser.is_set("stcin")) {
        // Load from STC files (separate lh/rh)
        const std::string stcPrefix = parser.value("stcin");

        const std::filesystem::path lhFile = path_from_utf8(stcPrefix + "-lh.stc");
        const std::filesystem::path rhFile = path_from_utf8(stcPrefix + "-rh.stc");

        if (std::filesystem::exists(lhFile)) {
            auto read = InvSourceEstimate::read(lhFile);
            if (!read) {
                Log::critical("{}", read.error().what());
                return 1;
            }
            stcLH = std::move(*read);
            Log::info("Read LH STC: {} sources x {} time points (tmin={:.3f}, tstep={:.6f})", static_cast<int>(stcLH.data.rows()), static_cast<int>(stcLH.data.cols()), stcLH.tmin, stcLH.tstep);
        }

        if (std::filesystem::exists(rhFile)) {
            auto read = InvSourceEstimate::read(rhFile);
            if (!read) {
                Log::critical("{}", read.error().what());
                return 1;
            }
            stcRH = std::move(*read);
            Log::info("Read RH STC: {} sources x {} time points", static_cast<int>(stcRH.data.rows()), static_cast<int>(stcRH.data.cols()));
        }

        if (stcLH.empty() && stcRH.empty()) {
            Log::critical("No STC data loaded from prefix: {}", stcPrefix);
            return 1;
        }
    } else if (parser.is_set("inv") && parser.is_set("meas")) {
        // Compute from inverse operator + measurement
        const std::string invFile = parser.value("inv");
        const std::string measFile = parser.value("meas");
        const int setNo = intValue("set");
        const double snr = doubleValue("snr");
        const double lambda2 = 1.0 / (snr * snr);
        const std::string method = parser.value("method");

        auto invOpRead = MNEInverseOperator::read(path_from_utf8(invFile));
        if (!invOpRead) {
            Log::critical("{}", invOpRead.error().what());
            return 1;
        }
        MNEInverseOperator& invOp = *invOpRead;

        auto read = FiffEvokedSet::read(path_from_utf8(measFile));
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        FiffEvokedSet& evokedSet = *read;
        if (setNo < 0 || setNo >= std::ssize(evokedSet.evoked)) {
            Log::critical("Set number {} out of range (have {} sets).", setNo, static_cast<long long>(evokedSet.evoked.size()));
            return 1;
        }

        FiffEvoked evoked = evokedSet.evoked[static_cast<std::size_t>(setNo)];
        Log::info("Computing inverse: {}, SNR={:.1f}, lambda2={:.4f}", method, snr, lambda2);

        InvMinimumNorm minimumNorm(invOp, lambda2, method);
        stcLH = minimumNorm.calculateInverse(evoked);
        if (stcLH.empty()) {
            Log::critical("Inverse computation failed.");
            return 1;
        }

        // Resolve subject from inverse operator if not specified
        if (subject.empty()) {
            subject = "sample";
        }

        Log::info("Computed STC: {} sources x {} time points", static_cast<int>(stcLH.data.rows()), static_cast<int>(stcLH.data.cols()));
    } else {
        Log::critical("Provide either --stcin or both --inv and --meas.");
        return 1;
    }

    // Use the non-empty STC for time parameters
    const InvSourceEstimate& refStc = stcLH.empty() ? stcRH : stcLH;

    //=========================================================================
    // Time windowing
    //=========================================================================
    int tStartIdx = 0;
    int tEndIdx = static_cast<int>(refStc.data.cols()) - 1;
    int tStepIdx = 1;

    if (parser.is_set("tmin")) {
        const double tmin = doubleValue("tmin");
        tStartIdx = std::max(0, static_cast<int>((tmin - refStc.tmin) / refStc.tstep));
    }
    if (parser.is_set("tmax")) {
        const double tmax = doubleValue("tmax");
        tEndIdx = std::min(static_cast<int>(refStc.data.cols()) - 1,
                           static_cast<int>((tmax - refStc.tmin) / refStc.tstep));
    }
    if (parser.is_set("tstep")) {
        const double tstep = doubleValue("tstep");
        tStepIdx = std::max(1, static_cast<int>(tstep / refStc.tstep));
    }

    //=========================================================================
    // Label extraction (if requested)
    //=========================================================================
    if (parser.is_set("label") && parser.is_set("labelout")) {
        const std::string labelFile = parser.value("label");
        const std::string labelOutFile = parser.value("labelout");

        FsLabel label;
        if (!FsLabel::read(path_from_utf8(labelFile), label)) {
            Log::critical("Cannot read label: {}", labelFile);
            return 1;
        }

        Log::info("Extracting label time course for {} vertices", static_cast<long long>(label.vertices.size()));

        std::ofstream out(path_from_utf8(labelOutFile));
        if (!out) {
            Log::critical("Cannot open output file: {}", labelOutFile);
            return 1;
        }

        for (int t = tStartIdx; t <= tEndIdx; t += tStepIdx) {
            const double time = refStc.tmin + t * refStc.tstep;
            double meanVal = 0.0;
            int count = 0;
            for (int v = 0; v < label.vertices.size(); ++v) {
                const int vi = label.vertices(v);
                if (vi < refStc.data.rows()) {
                    meanVal += refStc.data(vi, t);
                    count++;
                }
            }
            if (count > 0)
                meanVal /= count;
            out << format_number(time, 'f', 6) << "\t" << format_number(meanVal, 'e', 6) << "\n";
        }

        Log::info("Written label time course to: {}", labelOutFile);
    }

    //=========================================================================
    // Load surface and render frames
    //=========================================================================
    const std::string pngPrefix = parser.value("png");

    if (pngPrefix.empty()) {
        Log::info("No --png specified; skipping frame rendering.");
        Log::info("Use --png <prefix> to export image frames.");
        return 0;
    }

    if (subject.empty() || subjectsDir.empty()) {
        Log::critical("--subject and --subjects-dir (or $SUBJECTS_DIR) are required for rendering.");
        return 1;
    }

    const bool doLH = !parser.is_set("rh");
    const bool doRH = !parser.is_set("lh");

    int frameCount = 0;

    auto renderHemi = [&](const std::string& hemi) {
        const std::string surfPath = subjectsDir + "/" + subject + "/surf/" + hemi + "." + surfName;
        FsSurface surf;
        if (!FsSurface::read(path_from_utf8(surfPath), surf)) {
            Log::critical("Cannot read surface: {}", surfPath);
            return;
        }
        Log::info("Loaded surface {}: {} vertices, {} triangles", surfPath, static_cast<int>(surf.rr().rows()), static_cast<int>(surf.tris().rows()));

        // Select the appropriate STC for this hemisphere
        const InvSourceEstimate& hemiStc = (hemi == "lh") ? stcLH : stcRH;
        if (hemiStc.empty()) {
            Log::info("No STC data for {} hemisphere, skipping.", hemi);
            return;
        }

        for (int t = tStartIdx; t <= tEndIdx; t += tStepIdx) {
            // Map STC data to surface vertices
            VectorXd surfValues = VectorXd::Zero(surf.rr().rows());

            const VectorXi& verts = hemiStc.vertices;
            for (int i = 0; i < verts.size(); ++i) {
                int surfVert = verts(i);
                if (surfVert < surfValues.size() && i < hemiStc.data.rows()) {
                    surfValues(surfVert) = hemiStc.data(i, t);
                }
            }

            RasterImage frame = renderFrame(surf, surfValues, width, height,
                                            fthresh, fmid, fmax, isSigned, viewName);

            // Add time annotation
            const double time = refStc.tmin + t * refStc.tstep;
            frame.draw_text(10, height - 10, std::format("t = {} ms", format_number(time * 1000.0, 'f', 1)), {255, 255, 255}, 2);

            const std::string framePath = std::format("{}-{}-{:05}.png", pngPrefix, hemi, frameCount);
            if (!frame.save_png(path_from_utf8(framePath))) {
                Log::critical("Cannot save frame: {}", framePath);
                return;
            }
            frameCount++;
        }
    };

    if (doLH)
        renderHemi("lh");
    if (doRH)
        renderHemi("rh");

    Log::info("Exported {} frame(s) to {}-*.png", frameCount, pngPrefix);
    Log::info("\nTo create a movie, use:");
    Log::info("  ffmpeg -framerate 24 -i {}-lh-%05d.png -c:v libx264 -pix_fmt yuv420p movie.mp4", pngPrefix);

    return 0;
}
