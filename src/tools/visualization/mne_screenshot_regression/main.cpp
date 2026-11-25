// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Headless screenshot regression tool for disp3D BrainView.
//
// Loads FreeSurfer surfaces from a subjects directory, renders them
// offscreen via BrainView::savePng() and optionally compares the
// result against a golden reference image (pixel RMSE with tolerance).
//
// Usage:
//   mne_screenshot_regression --subjects-dir <path> --subject <name>
//       --out <output.png> [--ref <golden.png>] [--tolerance <0.0-1.0>]
//       [--width 1200] [--height 800]

#include <QApplication>
#include <QCommandLineParser>
#include <QImage>
#include <QDir>

#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_annotation.h>
#include <mne/disp3D_widgets/view/brainview.h>
#include <mne/disp3D/model/braintreemodel.h>
#include <mne/base/log.h>

#include <cmath>

using namespace mne;
using namespace mne::disp;

//=============================================================================================================

static double imageRmse(const QImage& a, const QImage& b)
{
    if (a.size() != b.size())
        return 1.0;

    const QImage imgA = a.convertToFormat(QImage::Format_RGBA8888);
    const QImage imgB = b.convertToFormat(QImage::Format_RGBA8888);

    double sumSq = 0.0;
    const int n = imgA.width() * imgA.height() * 4; // RGBA channels
    const uchar* pA = imgA.constBits();
    const uchar* pB = imgB.constBits();

    for (int i = 0; i < n; ++i) {
        double diff = static_cast<double>(pA[i]) - static_cast<double>(pB[i]);
        sumSq += diff * diff;
    }

    return std::sqrt(sumSq / n) / 255.0; // normalised to [0, 1]
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("mne_screenshot_regression");

    QCommandLineParser parser;
    parser.setApplicationDescription("Headless screenshot regression for disp3D");
    parser.addHelpOption();

    parser.addOption({{"s", "subjects-dir"}, "FreeSurfer subjects directory.", "path"});
    parser.addOption({"subject", "Subject name (e.g. sample).", "name", "sample"});
    parser.addOption({{"o", "out"}, "Output PNG path.", "path", "screenshot.png"});
    parser.addOption({{"r", "ref"}, "Golden reference PNG for comparison.", "path"});
    parser.addOption({{"t", "tolerance"}, "Max RMSE tolerance (0.0–1.0).", "value", "0.01"});
    parser.addOption({{"W", "width"}, "Render width in pixels.", "px", "1200"});
    parser.addOption({{"H", "height"}, "Render height in pixels.", "px", "800"});
    parser.process(app);

    const QString subjectsDir = parser.value("subjects-dir");
    const QString subject = parser.value("subject");
    const QString outPath = parser.value("out");
    const QString refPath = parser.value("ref");
    const double tolerance = parser.value("tolerance").toDouble();
    const int width = parser.value("width").toInt();
    const int height = parser.value("height").toInt();

    if (subjectsDir.isEmpty()) {
        Log::critical("Error: --subjects-dir is required.");
        return 1;
    }

    // ── Set up BrainView and model ─────────────────────────────────────
    BrainTreeModel model;
    BrainView view;
    view.setModel(&model);

    // ── Load FreeSurfer surfaces (after setModel so rowsInserted fires) ─
    bool anyLoaded = false;

    const QStringList hemis{QStringLiteral("lh"), QStringLiteral("rh")};
    for (const QString& hemi : hemis) {
        const int hemiIdx = (hemi == "lh") ? 0 : 1;
        FsSurface surf;
        if (FsSurface::read(subject.toStdString(), hemiIdx, "white", subjectsDir.toStdU16String(), surf)) {
            model.addSurface(subject, hemi, "white", surf);
            anyLoaded = true;
        }

        FsAnnotation annot;
        const QString annotPath = QDir(subjectsDir).filePath(subject + "/label/" + hemi + ".aparc.annot");
        if (QFile::exists(annotPath) && FsAnnotation::read(annotPath.toStdU16String(), annot)) {
            model.addAnnotation(subject, hemi, annot);
        }
    }

    if (!anyLoaded) {
        Log::critical("Error: No surfaces found in {}/{}/surf/", subjectsDir.toStdString(), subject.toStdString());
        return 1;
    }

    // ── Render offscreen ────────────────────────────────────────────────

    Log::info("Surfaces loaded, rendering offscreen ({}x{})...", width, height);

    if (!view.savePng(outPath.toStdU16String(), width, height, "white")) {
        Log::critical("Error: offscreen render failed.");
        return 1;
    }

    Log::info("Saved screenshot: {}", outPath.toStdString());

    // ── Optional golden comparison ──────────────────────────────────────
    if (!refPath.isEmpty()) {
        QImage ref(refPath);
        if (ref.isNull()) {
            Log::critical("Error: cannot load reference image: {}", refPath.toStdString());
            return 1;
        }

        QImage out(outPath);
        if (out.isNull()) {
            Log::critical("Error: cannot load rendered image.");
            return 1;
        }

        const double rmse = imageRmse(ref, out);
        Log::info("RMSE: {} (tolerance: {})", rmse, tolerance);

        if (rmse > tolerance) {
            Log::critical("FAIL: RMSE {} exceeds tolerance {}", rmse, tolerance);
            return 1;
        }

        Log::info("PASS: screenshot matches reference within tolerance.");
    }

    return 0;
}
