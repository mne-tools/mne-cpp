//=============================================================================================================
/**
 * @file     mne_surf2bem_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 *
 * @brief    MneSurf2BemSettings class definition.
 *
 *           Command-line parsing replicates the original MNE C tool's behavior
 *           where options like --id, --swap, --shift apply to the most recently
 *           specified input surface (--surf or --tri).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surf2bem_settings.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSurf2BemSettings::MneSurf2BemSettings(int *argc, char **argv)
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
            printf("Usage: %s [options]\n", qPrintable(args[0]));
            printf("Convert FreeSurfer surfaces into BEM FIFF files.\n\n");
            printf("Ported from the original MNE C tool mne_surf2bem by Matti Hamalainen\n");
            printf("(SVN $Id: mne_surf2bem.c 3351 2012-03-05 12:03:50Z msh $).\n\n");
            printf("Options:\n");
            printf("  --surf name       Input FreeSurfer binary surface file.\n");
            printf("  --tri name        Input ASCII triangle file.\n");
            printf("  --fif name        Output FIFF BEM surface file.\n");
            printf("  --id id           BEM surface id to assign:\n");
            printf("                      %d = head (outer skin)\n", FIFFV_BEM_SURF_ID_HEAD);
            printf("                      %d = outer skull\n", FIFFV_BEM_SURF_ID_SKULL);
            printf("                      %d = inner skull (brain)\n", FIFFV_BEM_SURF_ID_BRAIN);
            printf("  --swap            Swap vertex winding order (ASCII tri files).\n");
            printf("  --meters          Coordinates in meters (ASCII files only, default: mm).\n");
            printf("  --coordf no       Coordinate frame for ASCII file vertices.\n");
            printf("  --shift val       Shift vertices by val [mm] along normals.\n");
            printf("  --ico number      Downsample to icosahedron subdivision (0-6).\n");
            printf("  --sigma val       Compartment conductivity [S/m].\n");
            printf("  --force           Load surface despite topological defects.\n");
            printf("  --check           Perform topology and solid angle checks.\n");
            printf("  --checkmore       Also check skull/skin thicknesses.\n");
            printf("  --help            Print this help.\n");
            printf("  --version         Print version info.\n\n");
            printf("Note: --id, --swap, --meters, --shift, --ico, --sigma apply to the\n");
            printf("most recently specified --surf or --tri.\n\n");
            exit(0);
        }
        else if (arg == "--version") {
            printf("mne_surf2bem version 2.0 (mne-cpp port)\n");
            printf("Based on MNE C version 1.8 by Matti Hamalainen\n");
            exit(0);
        }
        else if (arg == "--surf" || arg == "--tri") {
            if (i + 1 >= args.size()) {
                qCritical() << arg << ": argument required.";
                exit(1);
            }
            SurfaceInput si;
            si.fileName = args[++i];
            si.isAsciiTri = (arg == "--tri");
            m_surfaces.append(si);
        }
        else if (arg == "--fif") {
            if (i + 1 >= args.size()) {
                qCritical() << "--fif: argument required.";
                exit(1);
            }
            m_sOutputFile = args[++i];
        }
        else if (arg == "--id") {
            if (i + 1 >= args.size()) {
                qCritical() << "--id: argument required.";
                exit(1);
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                qCritical() << "Illegal number:" << args[i];
                exit(1);
            }
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify surface before its id.";
                exit(1);
            }
            m_surfaces.last().id = val;
        }
        else if (arg == "--swap") {
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify surface before --swap.";
                exit(1);
            }
            m_surfaces.last().swap = true;
        }
        else if (arg == "--meters") {
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify surface before --meters.";
                exit(1);
            }
            m_surfaces.last().mm = false;
        }
        else if (arg == "--coordf") {
            if (i + 1 >= args.size()) {
                qCritical() << "--coordf: argument required.";
                exit(1);
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                qCritical() << "Illegal number:" << args[i];
                exit(1);
            }
            m_iCoordFrame = val;
        }
        else if (arg == "--ico") {
            if (i + 1 >= args.size()) {
                qCritical() << "--ico: argument required.";
                exit(1);
            }
            bool ok;
            int val = args[++i].toInt(&ok);
            if (!ok) {
                qCritical() << "Illegal number:" << args[i];
                exit(1);
            }
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify surface before --ico.";
                exit(1);
            }
            if (val < 0 || val > 6) {
                qCritical() << "--ico value should be between 0 and 6.";
                exit(1);
            }
            m_surfaces.last().ico = val;
        }
        else if (arg == "--sigma") {
            if (i + 1 >= args.size()) {
                qCritical() << "--sigma: argument required.";
                exit(1);
            }
            bool ok;
            float val = args[++i].toFloat(&ok);
            if (!ok) {
                qCritical() << "Illegal number:" << args[i];
                exit(1);
            }
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify surface before its conductivity.";
                exit(1);
            }
            m_surfaces.last().sigma = val;
        }
        else if (arg == "--shift") {
            if (i + 1 >= args.size()) {
                qCritical() << "--shift: argument required.";
                exit(1);
            }
            bool ok;
            float val = args[++i].toFloat(&ok);
            if (!ok) {
                qCritical() << "Incomprehensible value:" << args[i];
                exit(1);
            }
            if (m_surfaces.isEmpty()) {
                qCritical() << "Specify the surface before the vertex shift.";
                exit(1);
            }
            // Convert mm to meters (matching original C code)
            m_surfaces.last().shift = val / 1000.0f;
        }
        else if (arg == "--force") {
            m_bForce = true;
        }
        else if (arg == "--check") {
            m_bCheck = true;
        }
        else if (arg == "--checkmore") {
            m_bCheck = true;
            m_bCheckMore = true;
        }
        else {
            qCritical() << "Unrecognized argument:" << arg;
            exit(1);
        }
    }
}

//=============================================================================================================

const QVector<SurfaceInput>& MneSurf2BemSettings::surfaces() const
{
    return m_surfaces;
}

//=============================================================================================================

QString MneSurf2BemSettings::outputFile() const
{
    return m_sOutputFile;
}

//=============================================================================================================

int MneSurf2BemSettings::coordFrame() const
{
    return m_iCoordFrame;
}

//=============================================================================================================

bool MneSurf2BemSettings::check() const
{
    return m_bCheck;
}

//=============================================================================================================

bool MneSurf2BemSettings::checkMore() const
{
    return m_bCheckMore;
}

//=============================================================================================================

bool MneSurf2BemSettings::force() const
{
    return m_bForce;
}
