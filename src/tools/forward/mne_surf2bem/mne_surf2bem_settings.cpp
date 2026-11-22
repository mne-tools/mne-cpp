//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_surf2bem_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNESurf2BemSettings class definition.
 *
 *           Command-line parsing replicates the original MNE C tool's behavior
 *           where options like --id, --swap, --shift apply to the most recently
 *           specified input surface (--surf or --tri).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "mne_surf2bem_settings.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESurf2BemSettings::MNESurf2BemSettings(int* argc, char** argv)
: m_iCoordFrame(FIFFV_COORD_MRI)
, m_bCheck(false)
, m_bCheckMore(false)
, m_bForce(false)
{
    //
    // The original C tool used a custom positional argument parser where
    // --id, --swap, --shift, etc. apply to the most recently specified
    // surface. We replicate that behavior here rather than using
    // QCommandLineParser, which doesn't support positional association.
    //
    QStringList args;
    for (int i = 0; i < *argc; ++i) {
        args << QString(argv[i]);
    }

    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args[i];

        if (arg == "--help" || arg == "-h") {
            mne::Log::info("Usage: {} [options]", args[0].toStdString());
            mne::Log::info("Convert FreeSurfer surfaces into BEM FIFF files.\n");
            mne::Log::info("Ported from the original MNE C tool mne_surf2bem by Matti Hamalainen");
            mne::Log::info("(SVN $Id: mne_surf2bem.c 3351 2012-03-05 12:03:50Z msh $).\n");
            mne::Log::info("Options:");
            mne::Log::info("  --surf name       Input FreeSurfer binary surface file.");
            mne::Log::info("  --tri name        Input ASCII triangle file.");
            mne::Log::info("  --fif name        Output FIFF BEM surface file.");
            mne::Log::info("  --id id           BEM surface id to assign:");
            mne::Log::info("                      {} = head (outer skin)", FIFFV_BEM_SURF_ID_HEAD);
            mne::Log::info("                      {} = outer skull", FIFFV_BEM_SURF_ID_SKULL);
            mne::Log::info("                      {} = inner skull (brain)", FIFFV_BEM_SURF_ID_BRAIN);
            mne::Log::info("  --swap            Swap vertex winding order (ASCII tri files).");
            mne::Log::info("  --meters          Coordinates in meters (ASCII files only, default: mm).");
            mne::Log::info("  --coordf no       Coordinate frame for ASCII file vertices.");
            mne::Log::info("  --shift val       Shift vertices by val [mm] along normals.");
            mne::Log::info("  --ico number      Downsample to icosahedron subdivision (0-6).");
            mne::Log::info("  --sigma val       Compartment conductivity [S/m].");
            mne::Log::info("  --force           Load surface despite topological defects.");
            mne::Log::info("  --check           Perform topology and solid angle checks.");
            mne::Log::info("  --checkmore       Also check skull/skin thicknesses.");
            mne::Log::info("  --help            Print this help.");
            mne::Log::info("  --version         Print version info.\n");
            mne::Log::info("Note: --id, --swap, --meters, --shift, --ico, --sigma apply to the");
            mne::Log::info("most recently specified --surf or --tri.\n");
            m_bShouldExit = true;
            m_iExitCode = 0;
            return;
        } else if (arg == "--version") {
            mne::Log::info("mne_surf2bem version 2.0 (mne-cpp port)");
            mne::Log::info("Based on MNE C version 1.8 by Matti Hamalainen");
            m_bShouldExit = true;
            m_iExitCode = 0;
            return;
        } else if (arg == "--surf" || arg == "--tri") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("{} : argument required.", arg.toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            SurfaceInput si;
            si.fileName = args[++i];
            si.isAsciiTri = (arg == "--tri");
            m_surfaces.append(si);
        } else if (arg == "--fif") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--fif: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_sOutputFile = args[++i];
        } else if (arg == "--id") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--id: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                mne::Log::critical("Illegal number: {}", args[i].toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify surface before its id.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_surfaces.last().id = val;
        } else if (arg == "--swap") {
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify surface before --swap.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_surfaces.last().swap = true;
        } else if (arg == "--meters") {
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify surface before --meters.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_surfaces.last().mm = false;
        } else if (arg == "--coordf") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--coordf: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                mne::Log::critical("Illegal number: {}", args[i].toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_iCoordFrame = val;
        } else if (arg == "--ico") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--ico: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                mne::Log::critical("Illegal number: {}", args[i].toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify surface before --ico.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            if (val < 0 || val > 6) {
                mne::Log::critical("--ico value should be between 0 and 6.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_surfaces.last().ico = val;
        } else if (arg == "--sigma") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--sigma: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            bool ok;
            float val = args[++i].toFloat(&ok);
            if (!ok) {
                mne::Log::critical("Illegal number: {}", args[i].toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify surface before its conductivity.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            m_surfaces.last().sigma = val;
        } else if (arg == "--shift") {
            if (i + 1 >= args.size()) {
                mne::Log::critical("--shift: argument required.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            bool ok;
            float val = args[++i].toFloat(&ok);
            if (!ok) {
                mne::Log::critical("Incomprehensible value: {}", args[i].toStdString());
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            if (m_surfaces.isEmpty()) {
                mne::Log::critical("Specify the surface before the vertex shift.");
                m_bShouldExit = true;
                m_iExitCode = 1;
                return;
            }
            // Convert mm to meters (matching original C code)
            m_surfaces.last().shift = val / 1000.0f;
        } else if (arg == "--force") {
            m_bForce = true;
        } else if (arg == "--check") {
            m_bCheck = true;
        } else if (arg == "--checkmore") {
            m_bCheck = true;
            m_bCheckMore = true;
        } else {
            mne::Log::critical("Unrecognized argument: {}", arg.toStdString());
            m_bShouldExit = true;
            m_iExitCode = 1;
            return;
        }
    }
}

//=============================================================================================================

const QVector<SurfaceInput>& MNESurf2BemSettings::surfaces() const
{
    return m_surfaces;
}

//=============================================================================================================

QString MNESurf2BemSettings::outputFile() const
{
    return m_sOutputFile;
}

//=============================================================================================================

int MNESurf2BemSettings::coordFrame() const
{
    return m_iCoordFrame;
}

//=============================================================================================================

bool MNESurf2BemSettings::check() const
{
    return m_bCheck;
}

//=============================================================================================================

bool MNESurf2BemSettings::checkMore() const
{
    return m_bCheckMore;
}

//=============================================================================================================

bool MNESurf2BemSettings::force() const
{
    return m_bForce;
}

//=============================================================================================================

bool MNESurf2BemSettings::shouldExit() const
{
    return m_bShouldExit;
}

//=============================================================================================================

int MNESurf2BemSettings::exitCode() const
{
    return m_iExitCode;
}
