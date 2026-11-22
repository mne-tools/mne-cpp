//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    List source space information.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_spaces.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/log.h>

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

static const char* coordFrameName(int frame)
{
    switch (frame) {
        case FIFFV_COORD_MRI:
            return "MRI (surface RAS)";
        case FIFFV_COORD_HEAD:
            return "Head";
        case FIFFV_COORD_DEVICE:
            return "Device";
        case FIFFV_COORD_UNKNOWN:
            return "Unknown";
        default:
            return "Other";
    }
}

//=============================================================================================================

static const char* spaceTypeName(int type)
{
    switch (type) {
        case FIFFV_MNE_SPACE_SURFACE:
            return "Surface";
        case FIFFV_MNE_SPACE_VOLUME:
            return "Volume";
        case FIFFV_MNE_SPACE_DISCRETE:
            return "Discrete";
        default:
            return "Unknown";
    }
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_source_space");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("List source space information in various output formats.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption srcOpt("src", "Source space FIFF file.", "file");
    parser.addOption(srcOpt);

    QCommandLineOption pntOpt("pnt", "Output point file (MRIlab format).", "file");
    parser.addOption(pntOpt);

    QCommandLineOption dipOpt("dip", "Output dipole file (MRIlab format).", "file");
    parser.addOption(dipOpt);

    QCommandLineOption vertOpt("vert", "Output vertex file (text).", "file");
    parser.addOption(vertOpt);

    QCommandLineOption allOpt("all", "List all vertices (not just active).");
    parser.addOption(allOpt);

    QCommandLineOption coordOpt("coord", "Coordinate frame: head or mri.", "name", "mri");
    parser.addOption(coordOpt);

    parser.process(app);

    QString srcName = parser.value(srcOpt);
    QString pntName = parser.value(pntOpt);
    QString dipName = parser.value(dipOpt);
    QString vertName = parser.value(vertOpt);
    bool listAll = parser.isSet(allOpt);
    [[maybe_unused]] int coordFrame = FIFFV_COORD_MRI;
    QString coordStr = parser.value(coordOpt);
    if (coordStr == "head")
        coordFrame = FIFFV_COORD_HEAD;
    else if (coordStr == "mri")
        coordFrame = FIFFV_COORD_MRI;
    else {
        Log::critical("Unknown coordinate frame: {} (use head or mri)", coordStr.toStdString());
        return 1;
    }

    if (srcName.isEmpty()) {
        Log::critical("--src is required.");
        return 1;
    }

    // Read source spaces
    auto read = MNESourceSpaces::read(srcName.toStdU16String());
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const MNESourceSpaces& sourceSpaces = *read;

    if (sourceSpaces.size() == 0) {
        Log::critical("No source spaces found in file.");
        return 1;
    }

    // Print summary
    Log::info("Source space file : {}", srcName.toStdString());
    Log::info("Number of spaces  : {}", sourceSpaces.size());
    Log::info("{}", "");

    int totalActive = 0;
    int totalVerts = 0;

    for (int s = 0; s < sourceSpaces.size(); s++) {
        const MNESourceSpace& space = sourceSpaces[s];

        int nvert = space.np;
        int nuse = space.nuse;
        int ntri = space.ntri;
        totalActive += nuse;
        totalVerts += nvert;

        Log::info("--- Space {} ---", s + 1);
        Log::info("  Type            : {}", spaceTypeName(space.type));
        Log::info("  Vertices        : {}", nvert);
        Log::info("  Active vertices : {}", nuse);
        Log::info("  Triangles       : {}", ntri);
        Log::info("  Coord frame     : {}", coordFrameName(space.coord_frame));

        if (space.id == FIFFV_MNE_SURF_LEFT_HEMI)
            Log::info("  Hemisphere      : Left");
        else if (space.id == FIFFV_MNE_SURF_RIGHT_HEMI)
            Log::info("  Hemisphere      : Right");

        if (nuse > 0 && space.rr.rows() > 0) {
            // Compute bounding box of active vertices
            float xmin = 1e10, xmax = -1e10;
            float ymin = 1e10, ymax = -1e10;
            float zmin = 1e10, zmax = -1e10;

            for (int v = 0; v < nvert; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                float x = space.rr(v, 0) * 1000.0f;
                float y = space.rr(v, 1) * 1000.0f;
                float z = space.rr(v, 2) * 1000.0f;
                if (x < xmin)
                    xmin = x;
                if (x > xmax)
                    xmax = x;
                if (y < ymin)
                    ymin = y;
                if (y > ymax)
                    ymax = y;
                if (z < zmin)
                    zmin = z;
                if (z > zmax)
                    zmax = z;
            }
            Log::info("  Extent (mm)     : x [{:.1f}, {:.1f}] y [{:.1f}, {:.1f}] z [{:.1f}, {:.1f}]", xmin, xmax, ymin, ymax, zmin, zmax);
        }
        Log::info("{}", "");
    }

    Log::info("Total active vertices : {} / {}", totalActive, totalVerts);

    // Write point file (MRIlab format)
    if (!pntName.isEmpty()) {
        QFile pntFile(pntName);
        if (!pntFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Log::critical("Cannot open output file: {}", pntName.toStdString());
            return 1;
        }
        QTextStream pnt(&pntFile);

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                pnt << QString::asprintf("%10.4f %10.4f %10.4f\n",
                                         space.rr(v, 0) * 1000.0f,
                                         space.rr(v, 1) * 1000.0f,
                                         space.rr(v, 2) * 1000.0f);
            }
        }
        Log::info("Wrote point file: {}", pntName.toStdString());
    }

    // Write dipole file (MRIlab format: position + orientation)
    if (!dipName.isEmpty()) {
        QFile dipFile(dipName);
        if (!dipFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Log::critical("Cannot open output file: {}", dipName.toStdString());
            return 1;
        }
        QTextStream dip(&dipFile);

        int dipNum = 1;
        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                dip << QString::asprintf("%5d %8.3f %8.3f %8.3f  %7.4f %7.4f %7.4f\n",
                                         dipNum++,
                                         space.rr(v, 0) * 1000.0f, space.rr(v, 1) * 1000.0f, space.rr(v, 2) * 1000.0f,
                                         space.nn(v, 0), space.nn(v, 1), space.nn(v, 2));
            }
        }
        Log::info("Wrote dipole file: {}", dipName.toStdString());
    }

    // Write vertex file (text: vertex_number x y z)
    if (!vertName.isEmpty()) {
        QFile vertFile(vertName);
        if (!vertFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Log::critical("Cannot open output file: {}", vertName.toStdString());
            return 1;
        }
        QTextStream vert(&vertFile);

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace& space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                vert << QString::asprintf("%6d %10.4f %10.4f %10.4f\n",
                                          v,
                                          space.rr(v, 0) * 1000.0f, space.rr(v, 1) * 1000.0f, space.rr(v, 2) * 1000.0f);
            }
        }
        Log::info("Wrote vertex file: {}", vertName.toStdString());
    }

    return 0;
}
