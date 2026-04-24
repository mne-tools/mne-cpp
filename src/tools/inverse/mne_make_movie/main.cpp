//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Export source estimate frames as image sequences from STC data on cortical surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_evoked_set.h>
#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>
#include <fs/fs_surface.h>
#include <fs/fs_label.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace FSLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QColor valueToColor(double value, double fthresh, double fmid, double fmax, bool isSigned)
{
    double absVal = std::abs(value);
    if (absVal < fthresh) return QColor(128, 128, 128);

    // Interpolate between hot colors
    double t = 0.0;
    if (fmax > fthresh) {
        t = qBound(0.0, (absVal - fthresh) / (fmax - fthresh), 1.0);
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

    return QColor(r, g, b);
}

//=============================================================================================================

static QImage renderFrame(const FsSurface& surf,
                          const VectorXd& values,
                          int width, int height,
                          double fthresh, double fmid, double fmax,
                          bool isSigned,
                          const QString& viewName)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(QColor(0, 0, 0));

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MatrixX3f& rr = surf.rr();
    const MatrixX3i& tris = surf.tris();

    if (rr.rows() == 0 || tris.rows() == 0) return image;

    // Simple orthographic projection
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    for (int i = 0; i < rr.rows(); ++i) {
        cx += rr(i, 0); cy += rr(i, 1); cz += rr(i, 2);
    }
    cx /= rr.rows(); cy /= rr.rows(); cz /= rr.rows();

    // View rotation
    int xIdx = 0, yIdx = 1;
    float flipX = 1.0f, flipY = -1.0f;
    if (viewName == "lat" || viewName == "lateral") {
        xIdx = 1; yIdx = 2;
    } else if (viewName == "med" || viewName == "medial") {
        xIdx = 1; yIdx = 2; flipX = -1.0f;
    } else if (viewName == "dor" || viewName == "dorsal") {
        xIdx = 0; yIdx = 1;
    } else if (viewName == "ven" || viewName == "ventral") {
        xIdx = 0; yIdx = 1; flipY = 1.0f;
    } else if (viewName == "ros" || viewName == "rostral") {
        xIdx = 0; yIdx = 2;
    } else if (viewName == "cau" || viewName == "caudal") {
        xIdx = 0; yIdx = 2; flipX = -1.0f;
    }

    // Find bounds
    float minX = 1e10f, maxX = -1e10f, minY = 1e10f, maxY = -1e10f;
    for (int i = 0; i < rr.rows(); ++i) {
        float px = flipX * (rr(i, xIdx) - (xIdx == 0 ? cx : (xIdx == 1 ? cy : cz)));
        float py = flipY * (rr(i, yIdx) - (yIdx == 0 ? cx : (yIdx == 1 ? cy : cz)));
        if (px < minX) minX = px;
        if (px > maxX) maxX = px;
        if (py < minY) minY = py;
        if (py > maxY) maxY = py;
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
        if (i0 < values.size()) { triVal += values(i0); count++; }
        if (i1 < values.size()) { triVal += values(i1); count++; }
        if (i2 < values.size()) { triVal += values(i2); count++; }
        if (count > 0) triVal /= count;

        QColor color = valueToColor(triVal, fthresh, fmid, fmax, isSigned);

        auto project = [&](int idx) -> QPointF {
            float px = flipX * (rr(idx, xIdx) - (xIdx == 0 ? cx : (xIdx == 1 ? cy : cz)));
            float py = flipY * (rr(idx, yIdx) - (yIdx == 0 ? cx : (yIdx == 1 ? cy : cz)));
            return QPointF((px - minX) * scale + 10, (py - minY) * scale + 10);
        };

        QPolygonF tri;
        tri << project(i0) << project(i1) << project(i2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPolygon(tri);
    }

    painter.end();
    return image;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_movie");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Export source estimate frames as image sequences from STC data on cortical surfaces.\n\n"
        "This tool renders source activity overlaid on a cortical surface and exports\n"
        "individual frames as PNG images. The frame sequence can be assembled into a\n"
        "movie using external tools (e.g. ffmpeg).\n\n"
        "Port of the MNE-C mne_make_movie utility.");
    parser.addHelpOption();
    parser.addVersionOption();

    // Input options
    QCommandLineOption stcinOpt("stcin", "Input STC file prefix (left hemisphere assumed; -rh.stc auto-loaded).", "prefix");
    QCommandLineOption invOpt("inv", "Inverse operator file (alternative to --stcin: compute from data).", "file");
    QCommandLineOption measOpt("meas", "Measurement file (used with --inv).", "file");
    QCommandLineOption setOpt("set", "Data set number (default: 0).", "number", "0");
    QCommandLineOption snrOpt("snr", "SNR for regularization (default: 3).", "value", "3.0");
    QCommandLineOption methodOpt("method", "Inverse method: MNE, dSPM, sLORETA (default: dSPM).", "name", "dSPM");

    // Time windowing
    QCommandLineOption tminOpt("tmin", "Start time in seconds.", "seconds");
    QCommandLineOption tmaxOpt("tmax", "End time in seconds.", "seconds");
    QCommandLineOption tstepOpt("tstep", "Time step between frames in seconds.", "seconds");

    // Surface options
    QCommandLineOption subjectOpt("subject", "FreeSurfer subject name.", "name");
    QCommandLineOption subjectsDirOpt("subjects-dir", "FreeSurfer SUBJECTS_DIR (default: $SUBJECTS_DIR).", "dir");
    QCommandLineOption surfaceOpt("surface", "Surface to use (default: inflated).", "name", "inflated");
    QCommandLineOption lhOpt("lh", "Left hemisphere only.");
    QCommandLineOption rhOpt("rh", "Right hemisphere only.");
    QCommandLineOption viewOpt("view", "View: lat, med, dor, ven, ros, cau (default: lat).", "name", "lat");

    // Color scaling
    QCommandLineOption fthreshOpt("fthresh", "Activation threshold (default: 3.0).", "value", "3.0");
    QCommandLineOption fmidOpt("fmid", "Mid-scale value (default: 6.0).", "value", "6.0");
    QCommandLineOption fmaxOpt("fmax", "Maximum scale value (default: 10.0).", "value", "10.0");
    QCommandLineOption signedOpt("signed", "Show signed values (positive + negative).");

    // Output options
    QCommandLineOption pngOpt("png", "Output PNG prefix.", "prefix");
    QCommandLineOption widthOpt("width", "Image width in pixels (default: 800).", "pixels", "800");
    QCommandLineOption heightOpt("height", "Image height in pixels (default: 600).", "pixels", "600");

    // Label options
    QCommandLineOption labelOpt("label", "Label file(s) for ROI extraction.", "file");
    QCommandLineOption labelOutOpt("labelout", "Output text file for label time courses.", "file");

    parser.addOption(stcinOpt);
    parser.addOption(invOpt);
    parser.addOption(measOpt);
    parser.addOption(setOpt);
    parser.addOption(snrOpt);
    parser.addOption(methodOpt);
    parser.addOption(tminOpt);
    parser.addOption(tmaxOpt);
    parser.addOption(tstepOpt);
    parser.addOption(subjectOpt);
    parser.addOption(subjectsDirOpt);
    parser.addOption(surfaceOpt);
    parser.addOption(lhOpt);
    parser.addOption(rhOpt);
    parser.addOption(viewOpt);
    parser.addOption(fthreshOpt);
    parser.addOption(fmidOpt);
    parser.addOption(fmaxOpt);
    parser.addOption(signedOpt);
    parser.addOption(pngOpt);
    parser.addOption(widthOpt);
    parser.addOption(heightOpt);
    parser.addOption(labelOpt);
    parser.addOption(labelOutOpt);

    parser.process(app);

    //=========================================================================
    // Resolve subjects directory
    //=========================================================================
    QString subjectsDir = parser.value(subjectsDirOpt);
    if (subjectsDir.isEmpty()) {
        subjectsDir = qEnvironmentVariable("SUBJECTS_DIR");
    }

    QString subject = parser.value(subjectOpt);
    QString surfName = parser.value(surfaceOpt);
    QString viewName = parser.value(viewOpt);

    int width = parser.value(widthOpt).toInt();
    int height = parser.value(heightOpt).toInt();
    double fthresh = parser.value(fthreshOpt).toDouble();
    double fmid = parser.value(fmidOpt).toDouble();
    double fmax = parser.value(fmaxOpt).toDouble();
    bool isSigned = parser.isSet(signedOpt);

    //=========================================================================
    // Load source estimate
    //=========================================================================
    InvSourceEstimate stcLH, stcRH;

    if (parser.isSet(stcinOpt)) {
        // Load from STC files (separate lh/rh)
        QString stcPrefix = parser.value(stcinOpt);

        QString lhFile = stcPrefix + "-lh.stc";
        QString rhFile = stcPrefix + "-rh.stc";

        QFile fLH(lhFile);
        if (fLH.exists()) {
            if (!InvSourceEstimate::read(fLH, stcLH)) {
                qCritical("Cannot read LH STC from: %s", qPrintable(lhFile));
                return 1;
            }
            printf("Read LH STC: %d sources x %d time points (tmin=%.3f, tstep=%.6f)\n",
                   static_cast<int>(stcLH.data.rows()), static_cast<int>(stcLH.data.cols()),
                   stcLH.tmin, stcLH.tstep);
        }

        QFile fRH(rhFile);
        if (fRH.exists()) {
            if (!InvSourceEstimate::read(fRH, stcRH)) {
                qCritical("Cannot read RH STC from: %s", qPrintable(rhFile));
                return 1;
            }
            printf("Read RH STC: %d sources x %d time points\n",
                   static_cast<int>(stcRH.data.rows()), static_cast<int>(stcRH.data.cols()));
        }

        if (stcLH.isEmpty() && stcRH.isEmpty()) {
            qCritical("No STC data loaded from prefix: %s", qPrintable(stcPrefix));
            return 1;
        }
    } else if (parser.isSet(invOpt) && parser.isSet(measOpt)) {
        // Compute from inverse operator + measurement
        QString invFile = parser.value(invOpt);
        QString measFile = parser.value(measOpt);
        int setNo = parser.value(setOpt).toInt();
        double snr = parser.value(snrOpt).toDouble();
        double lambda2 = 1.0 / (snr * snr);
        QString method = parser.value(methodOpt);

        QFile fInv(invFile);
        MNEInverseOperator invOp(fInv);
        if (invOp.nsource <= 0) {
            qCritical("Cannot read inverse operator: %s", qPrintable(invFile));
            return 1;
        }

        QFile fMeas(measFile);
        FiffEvokedSet evokedSet;
        if (!FiffEvokedSet::read(fMeas, evokedSet)) {
            qCritical("Cannot read evoked data: %s", qPrintable(measFile));
            return 1;
        }
        if (setNo >= evokedSet.evoked.size()) {
            qCritical("Set number %d out of range (have %lld sets).",
                      setNo, static_cast<long long>(evokedSet.evoked.size()));
            return 1;
        }

        FiffEvoked evoked = evokedSet.evoked[setNo];
        printf("Computing inverse: %s, SNR=%.1f, lambda2=%.4f\n",
               qPrintable(method), snr, lambda2);

        InvMinimumNorm minimumNorm(invOp, lambda2, method);
        stcLH = minimumNorm.calculateInverse(evoked);
        if (stcLH.isEmpty()) {
            qCritical("Inverse computation failed.");
            return 1;
        }

        // Resolve subject from inverse operator if not specified
        if (subject.isEmpty()) {
            subject = "sample";
        }

        printf("Computed STC: %d sources x %d time points\n",
               static_cast<int>(stcLH.data.rows()), static_cast<int>(stcLH.data.cols()));
    } else {
        qCritical("Provide either --stcin or both --inv and --meas.");
        return 1;
    }

    // Use the non-empty STC for time parameters
    const InvSourceEstimate& refStc = stcLH.isEmpty() ? stcRH : stcLH;

    //=========================================================================
    // Time windowing
    //=========================================================================
    int tStartIdx = 0;
    int tEndIdx = static_cast<int>(refStc.data.cols()) - 1;
    int tStepIdx = 1;

    if (parser.isSet(tminOpt)) {
        double tmin = parser.value(tminOpt).toDouble();
        tStartIdx = qMax(0, static_cast<int>((tmin - refStc.tmin) / refStc.tstep));
    }
    if (parser.isSet(tmaxOpt)) {
        double tmax = parser.value(tmaxOpt).toDouble();
        tEndIdx = qMin(static_cast<int>(refStc.data.cols()) - 1,
                       static_cast<int>((tmax - refStc.tmin) / refStc.tstep));
    }
    if (parser.isSet(tstepOpt)) {
        double tstep = parser.value(tstepOpt).toDouble();
        tStepIdx = qMax(1, static_cast<int>(tstep / refStc.tstep));
    }

    //=========================================================================
    // Label extraction (if requested)
    //=========================================================================
    if (parser.isSet(labelOpt) && parser.isSet(labelOutOpt)) {
        QString labelFile = parser.value(labelOpt);
        QString labelOutFile = parser.value(labelOutOpt);

        FsLabel label;
        if (!FsLabel::read(labelFile, label)) {
            qCritical("Cannot read label: %s", qPrintable(labelFile));
            return 1;
        }

        printf("Extracting label time course for %lld vertices\n",
               static_cast<long long>(label.vertices.size()));

        QFile outFile(labelOutFile);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open output file: %s", qPrintable(labelOutFile));
            return 1;
        }
        QTextStream out(&outFile);

        for (int t = tStartIdx; t <= tEndIdx; t += tStepIdx) {
            double time = refStc.tmin + t * refStc.tstep;
            double meanVal = 0.0;
            int count = 0;
            for (int v = 0; v < label.vertices.size(); ++v) {
                int vi = label.vertices(v);
                if (vi < refStc.data.rows()) {
                    meanVal += refStc.data(vi, t);
                    count++;
                }
            }
            if (count > 0) meanVal /= count;
            out << QString::number(time, 'f', 6) << "\t" << QString::number(meanVal, 'e', 6) << "\n";
        }

        outFile.close();
        printf("Written label time course to: %s\n", qPrintable(labelOutFile));
    }

    //=========================================================================
    // Load surface and render frames
    //=========================================================================
    QString pngPrefix = parser.value(pngOpt);

    if (pngPrefix.isEmpty()) {
        printf("No --png specified; skipping frame rendering.\n");
        printf("Use --png <prefix> to export image frames.\n");
        return 0;
    }

    if (subject.isEmpty() || subjectsDir.isEmpty()) {
        qCritical("--subject and --subjects-dir (or $SUBJECTS_DIR) are required for rendering.");
        return 1;
    }

    bool doLH = !parser.isSet(rhOpt);
    bool doRH = !parser.isSet(lhOpt);

    int frameCount = 0;

    auto renderHemi = [&](const QString& hemi) {
        QString surfPath = subjectsDir + "/" + subject + "/surf/" + hemi + "." + surfName;
        FsSurface surf;
        if (!FsSurface::read(surfPath, surf)) {
            qCritical("Cannot read surface: %s", qPrintable(surfPath));
            return;
        }
        printf("Loaded surface %s: %d vertices, %d triangles\n",
               qPrintable(surfPath), static_cast<int>(surf.rr().rows()),
               static_cast<int>(surf.tris().rows()));

        // Select the appropriate STC for this hemisphere
        const InvSourceEstimate& hemiStc = (hemi == "lh") ? stcLH : stcRH;
        if (hemiStc.isEmpty()) {
            printf("No STC data for %s hemisphere, skipping.\n", qPrintable(hemi));
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

            QImage frame = renderFrame(surf, surfValues, width, height,
                                       fthresh, fmid, fmax, isSigned, viewName);

            // Add time annotation
            double time = refStc.tmin + t * refStc.tstep;
            QPainter p(&frame);
            p.setPen(Qt::white);
            p.setFont(QFont("Helvetica", 14));
            p.drawText(10, height - 10, QString("t = %1 ms").arg(time * 1000.0, 0, 'f', 1));
            p.end();

            QString framePath = QString("%1-%2-%3.png").arg(pngPrefix).arg(hemi).arg(frameCount, 5, 10, QChar('0'));
            if (!frame.save(framePath)) {
                qCritical("Cannot save frame: %s", qPrintable(framePath));
                return;
            }
            frameCount++;
        }
    };

    if (doLH) renderHemi("lh");
    if (doRH) renderHemi("rh");

    printf("Exported %d frame(s) to %s-*.png\n", frameCount, qPrintable(pngPrefix));
    printf("\nTo create a movie, use:\n");
    printf("  ffmpeg -framerate 24 -i %s-lh-%%05d.png -c:v libx264 -pix_fmt yuv420p movie.mp4\n",
           qPrintable(pngPrefix));

    return 0;
}
