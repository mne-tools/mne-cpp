//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Create topographical EEG layout file from electrode positions in a FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>

#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

/**
 * Azimuthal equidistant projection from 3D electrode positions on a sphere.
 * Projects to a 2D plane preserving angles from the center.
 */
static void azimuthalProjection(const Vector3f& pos, const Vector3f& center,
                                [[maybe_unused]] float radius, float& x, float& y)
{
    Vector3f d = pos - center;
    d.normalize();

    // Convert to spherical
    float theta = acos(std::max(-1.0f, std::min(1.0f, d(2)))); // polar angle from z-axis
    float phi = atan2(d(1), d(0));                             // azimuthal angle

    // Azimuthal equidistant projection
    float r = theta;
    x = r * std::cos(static_cast<double>(phi));
    y = r * std::sin(static_cast<double>(phi));
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_make_eeg_layout");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Create a topographical EEG layout file from electrode positions in a FIFF file.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fifOpt("fif", "FIFF file with EEG channel info.", "name");
    parser.addOption(fifOpt);

    QCommandLineOption outOpt("out", "Output layout file (.lout).", "name");
    parser.addOption(outOpt);

    QCommandLineOption widthOpt("width", "Channel box width.", "val", "5.0");
    parser.addOption(widthOpt);

    QCommandLineOption heightOpt("height", "Channel box height.", "val", "4.0");
    parser.addOption(heightOpt);

    parser.process(app);

    QString fifName = parser.value(fifOpt);
    QString outName = parser.value(outOpt);
    float width = parser.value(widthOpt).toDouble();
    float height = parser.value(heightOpt).toDouble();

    if (fifName.isEmpty() || outName.isEmpty()) {
        Log::critical("Both --fif and --out are required.");
        parser.showHelp(1);
    }

    // Read the FIFF file to get channel info
    FiffRawData raw(fifName.toStdU16String());
    if (raw.info.empty()) {
        Log::critical("Cannot read FIFF file: {}", fifName.toStdString());
        return 1;
    }

    // Collect EEG channel positions
    struct EegCh
    {
        QString name;
        Vector3f pos;
    };
    QList<EegCh> eegChannels;

    for (int i = 0; i < raw.info.nchan; i++) {
        if (raw.info.chs[i].kind == FIFFV_EEG_CH) {
            EegCh ch;
            ch.name = QString::fromStdString(raw.info.chs[i].ch_name);
            ch.pos = Vector3f(raw.info.chs[i].chpos.r0(0),
                              raw.info.chs[i].chpos.r0(1),
                              raw.info.chs[i].chpos.r0(2));
            eegChannels.append(ch);
        }
    }

    if (eegChannels.isEmpty()) {
        Log::critical("No EEG channels found in {}", fifName.toStdString());
        return 1;
    }
    const auto eegChannelCount = static_cast<long long>(eegChannels.size());

    Log::info("{} EEG channels found.", eegChannelCount);

    // Compute center of mass for a simple sphere fit
    Vector3f center = Vector3f::Zero();
    for (const EegCh& ch : eegChannels) {
        center += ch.pos;
    }
    center /= eegChannels.size();

    // Compute average radius
    float radius = 0.0f;
    for (const EegCh& ch : eegChannels) {
        radius += (ch.pos - center).norm();
    }
    radius /= eegChannels.size();

    Log::info("Sphere center: {:.3f} {:.3f} {:.3f} m, radius: {:.3f} m", center(0), center(1), center(2), radius);

    // Project to 2D
    QList<float> xs, ys;
    for (const EegCh& ch : eegChannels) {
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
    if (xrange < 1e-10f)
        xrange = 1.0f;
    if (yrange < 1e-10f)
        yrange = 1.0f;

    // Write layout file
    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::critical("Cannot open output file: {}", outName.toStdString());
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
    Log::info("Layout written to {} ({} channels)", outName.toStdString(), eegChannelCount);

    return 0;
}
