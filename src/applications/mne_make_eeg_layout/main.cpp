//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Create topographical EEG layout file from electrode positions in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Create a topographical EEG layout file from electrode positions.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --fif <name>    FIFF file with EEG channel info\n");
    fprintf(stderr, "  --out <name>    Output layout file (.lout)\n");
    fprintf(stderr, "  --width <val>   Channel box width (default: 5.0)\n");
    fprintf(stderr, "  --height <val>  Channel box height (default: 4.0)\n");
    fprintf(stderr, "  --help          Print this help\n");
    fprintf(stderr, "  --version       Print version\n");
}

//=============================================================================================================

/**
 * Azimuthal equidistant projection from 3D electrode positions on a sphere.
 * Projects to a 2D plane preserving angles from the center.
 */
static void azimuthalProjection(const Vector3f &pos, const Vector3f &center,
                                 float radius, float &x, float &y)
{
    Vector3f d = pos - center;
    d.normalize();

    // Convert to spherical
    float theta = acos(std::max(-1.0f, std::min(1.0f, d(2))));  // polar angle from z-axis
    float phi = atan2(d(1), d(0));     // azimuthal angle

    // Azimuthal equidistant projection
    float r = theta;
    x = r * cos(phi);
    y = r * sin(phi);
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString fifName;
    QString outName;
    float width = 5.0f;
    float height = 4.0f;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        } else if (strcmp(argv[k], "--fif") == 0) {
            if (k + 1 >= argc) { qCritical("--fif: argument required."); return 1; }
            fifName = QString(argv[++k]);
        } else if (strcmp(argv[k], "--out") == 0) {
            if (k + 1 >= argc) { qCritical("--out: argument required."); return 1; }
            outName = QString(argv[++k]);
        } else if (strcmp(argv[k], "--width") == 0) {
            if (k + 1 >= argc) { qCritical("--width: argument required."); return 1; }
            width = atof(argv[++k]);
        } else if (strcmp(argv[k], "--height") == 0) {
            if (k + 1 >= argc) { qCritical("--height: argument required."); return 1; }
            height = atof(argv[++k]);
        } else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (fifName.isEmpty() || outName.isEmpty()) {
        qCritical("Both --fif and --out are required.");
        usage(argv[0]);
        return 1;
    }

    // Read the FIFF file to get channel info
    QFile file(fifName);
    FiffRawData raw(file);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read FIFF file: %s", qPrintable(fifName));
        return 1;
    }

    // Collect EEG channel positions
    struct EegCh {
        QString name;
        Vector3f pos;
    };
    QList<EegCh> eegChannels;

    for (int i = 0; i < raw.info.nchan; i++) {
        if (raw.info.chs[i].kind == FIFFV_EEG_CH) {
            EegCh ch;
            ch.name = raw.info.chs[i].ch_name;
            ch.pos = Vector3f(raw.info.chs[i].chpos.r0(0),
                              raw.info.chs[i].chpos.r0(1),
                              raw.info.chs[i].chpos.r0(2));
            eegChannels.append(ch);
        }
    }

    if (eegChannels.isEmpty()) {
        qCritical("No EEG channels found in %s", qPrintable(fifName));
        return 1;
    }

    fprintf(stderr, "%d EEG channels found.\n", eegChannels.size());

    // Compute center of mass for a simple sphere fit
    Vector3f center = Vector3f::Zero();
    for (const EegCh &ch : eegChannels) {
        center += ch.pos;
    }
    center /= eegChannels.size();

    // Compute average radius
    float radius = 0.0f;
    for (const EegCh &ch : eegChannels) {
        radius += (ch.pos - center).norm();
    }
    radius /= eegChannels.size();

    fprintf(stderr, "Sphere center: %.3f %.3f %.3f m, radius: %.3f m\n",
            center(0), center(1), center(2), radius);

    // Project to 2D
    QList<float> xs, ys;
    for (const EegCh &ch : eegChannels) {
        float x, y;
        azimuthalProjection(ch.pos, center, radius, x, y);
        xs.append(x);
        ys.append(y);
    }

    // Normalize to layout coordinates
    float xmin = *std::min_element(xs.begin(), xs.end());
    float xmax = *std::max_element(xs.begin(), xs.end());
    float ymin = *std::min_element(ys.begin(), ys.end());
    float ymax = *std::max_element(ys.begin(), ys.end());

    float xrange = xmax - xmin;
    float yrange = ymax - ymin;
    if (xrange < 1e-10f) xrange = 1.0f;
    if (yrange < 1e-10f) yrange = 1.0f;

    // Write layout file
    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot open output file: %s", qPrintable(outName));
        return 1;
    }

    QTextStream out(&outFile);

    for (int i = 0; i < eegChannels.size(); i++) {
        // Normalize to 0..100 range and center
        float nx = 5.0f + 90.0f * (xs[i] - xmin) / xrange;
        float ny = 5.0f + 90.0f * (ys[i] - ymin) / yrange;

        // Layout format: channel_number x y width height name
        out << i + 1 << " "
            << QString::number(nx, 'f', 2) << " "
            << QString::number(ny, 'f', 2) << " "
            << QString::number(width, 'f', 2) << " "
            << QString::number(height, 'f', 2) << " "
            << eegChannels[i].name << "\n";
    }

    outFile.close();
    fprintf(stderr, "Layout written to %s (%d channels)\n", qPrintable(outName), eegChannels.size());

    return 0;
}
