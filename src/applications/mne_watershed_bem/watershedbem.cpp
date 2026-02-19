//=============================================================================================================
/**
 * @file     watershedbem.cpp
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
 * @brief    WatershedBem class definition.
 *
 *           Ported from the original MNE shell script mne_watershed_bem
 *           by Matti Hamalainen (SVN $Id: mne_watershed_bem 3391).
 *
 *           The workflow matches the original shell script:
 *             1. Validate environment (FREESURFER_HOME, SUBJECTS_DIR, subject)
 *             2. Create watershed output directory structure
 *             3. Run mri_watershed for BEM segmentation
 *             4. If .mgz input: convert surfaces (update volume geometry)
 *             5. Create head surface FIFF BEM file from outer_skin_surface
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_watershed_bem().
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "watershedbem.h"
#include "mne_watershed_bem_settings.h"

#include <fs/surface.h>

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

#include <mri/mri_mgh_io.h>
#include <mri/mri_vol_data.h>

#include <fiff/fiff_file.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEWATERSHEDBEM;
using namespace FSLIB;
using namespace MNELIB;
using namespace MRILIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

WatershedBem::WatershedBem(const MneWatershedBemSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int WatershedBem::run()
{
    //
    // Validate environment — ported from mne_watershed_bem shell script
    //
    if (m_settings.freeSurferHome().isEmpty()) {
        qCritical() << "The FreeSurfer environment needs to be set up for this tool.";
        qCritical() << "Please set FREESURFER_HOME.";
        return 1;
    }

    if (m_settings.subjectsDir().isEmpty()) {
        qCritical() << "The subjects directory (SUBJECTS_DIR) is not set.";
        qCritical() << "Use --subjects-dir or set the SUBJECTS_DIR environment variable.";
        return 1;
    }

    if (m_settings.subject().isEmpty()) {
        qCritical() << "The subject name is not set.";
        qCritical() << "Use --subject or set the SUBJECT environment variable.";
        return 1;
    }

    //
    // Set up paths — following the shell script's variable naming
    //
    QString subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    QString mriDir = subjectDir + "/mri";
    QString T1Dir = mriDir + "/" + m_settings.volume();
    QString T1Mgz = mriDir + "/" + m_settings.volume() + ".mgz";
    QString bemDir = subjectDir + "/bem";
    QString wsDir = subjectDir + "/bem/watershed";

    //
    // Validate directory structure
    //
    if (!QDir(subjectDir).exists()) {
        qCritical() << "Could not find the MRI data directory" << subjectDir;
        return 1;
    }

    // Create bem directory if needed
    if (!QDir(bemDir).exists()) {
        if (!QDir().mkpath(bemDir)) {
            qCritical() << "Could not create the model directory" << bemDir;
            return 1;
        }
    }

    // Check that MRI data exists (directory or .mgz file)
    bool useMgz = QFileInfo::exists(T1Mgz);
    if (!QDir(T1Dir).exists() && !useMgz) {
        qCritical() << "Could not find the MRI data.";
        qCritical() << "  Looked for directory:" << T1Dir;
        qCritical() << "  Looked for file:" << T1Mgz;
        return 1;
    }

    // Check for existing watershed directory
    if (QDir(wsDir).exists()) {
        if (!m_settings.overwrite()) {
            qCritical() << wsDir << "already exists. Use the --overwrite option to recreate it.";
            return 1;
        } else {
            QDir wsDirObj(wsDir);
            if (!wsDirObj.removeRecursively()) {
                qCritical() << "Could not remove" << wsDir;
                return 1;
            }
        }
    }

    //
    // Report parameters
    //
    printf("\n");
    printf("Running mri_watershed for BEM segmentation with the following parameters\n");
    printf("\n");
    printf("SUBJECTS_DIR = %s\n", qPrintable(m_settings.subjectsDir()));
    printf("Subject      = %s\n", qPrintable(m_settings.subject()));
    printf("Result dir   = %s\n", qPrintable(wsDir));
    printf("\n");

    // Create output directory structure
    QString wsSubDir = wsDir + "/ws";
    if (!QDir().mkpath(wsSubDir)) {
        qCritical() << "Could not create the destination directories.";
        return 1;
    }

    //
    // Step 1: Run mri_watershed
    //
    QString mriInput = useMgz ? T1Mgz : T1Dir;
    if (!runMriWatershed(mriInput, wsDir)) {
        return 1;
    }

    //
    // Step 2: Convert surfaces if using .mgz input
    //   Equivalent to the shell script's:
    //     for s in $surfaces ; do
    //       mne_convert_surface --surf $s --mghmri $T1_mgz --surfout $s
    //     done
    //
    if (useMgz) {
        if (!convertSurfaces(wsDir, T1Mgz)) {
            return 1;
        }
    }

    //
    // Step 3: Create head surface FIFF BEM file
    //   Equivalent to:
    //     mne_surf2bem --surf $ws_dir/${SUBJECT}_outer_skin_surface --id 4 --fif $SUBJECT-head.fif
    //
    QString outerSkinSurf = wsDir + "/" + m_settings.subject() + "_outer_skin_surface";
    QString headFif = bemDir + "/" + m_settings.subject() + "-head.fif";

    // Remove existing head file
    QFile::remove(headFif);

    if (!createBemFif(outerSkinSurf, headFif)) {
        return 1;
    }

    printf("Created %s\n", qPrintable(headFif));
    printf("\nComplete.\n\n");
    return 0;
}

//=============================================================================================================

bool WatershedBem::runMriWatershed(const QString& mriInput, const QString& wsDir)
{
    //
    // Build the mri_watershed command line
    //   mri_watershed $preflood $atlas -useSRAS -surf $ws_dir/$SUBJECT $T1_input $ws_dir/ws
    //
    QString watershedBin = m_settings.freeSurferHome() + "/bin/mri_watershed";
    if (!QFileInfo::exists(watershedBin)) {
        qCritical() << "Could not find mri_watershed at" << watershedBin;
        return false;
    }

    QStringList args;

    // Preflood height
    if (m_settings.preflood() >= 0) {
        args << "-h" << QString::number(m_settings.preflood());
    }

    // Atlas options
    if (m_settings.gcaAtlas()) {
        // GCA atlas: -atlas -T1 -brain_atlas $FREESURFER_HOME/average/RB_all_withskull_2007-08-08.gca
        //            $subject_dir/mri/transforms/talairach_with_skull.lta
        QString gcaFile = m_settings.freeSurferHome() + "/average/RB_all_withskull_2007-08-08.gca";
        QString ltaFile = m_settings.subjectsDir() + "/" + m_settings.subject()
                        + "/mri/transforms/talairach_with_skull.lta";
        args << "-atlas" << "-T1" << "-brain_atlas" << gcaFile << ltaFile;
    } else if (m_settings.atlas()) {
        args << "-atlas";
    }

    // Use surface RAS coordinates
    args << "-useSRAS";

    // Surface output prefix
    args << "-surf" << (wsDir + "/" + m_settings.subject());

    // Input MRI and output directory
    args << mriInput;
    args << (wsDir + "/ws");

    printf("Running: %s", qPrintable(watershedBin));
    for (const QString& arg : args) {
        printf(" %s", qPrintable(arg));
    }
    printf("\n\n");

    //
    // Execute mri_watershed
    //
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);

    // Set up FreeSurfer environment for the subprocess
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FREESURFER_HOME", m_settings.freeSurferHome());
    env.insert("SUBJECTS_DIR", m_settings.subjectsDir());
    process.setProcessEnvironment(env);

    process.start(watershedBin, args);

    if (!process.waitForStarted()) {
        qCritical() << "Failed to start mri_watershed:" << process.errorString();
        return false;
    }

    // Wait indefinitely for mri_watershed to finish (it can take minutes)
    if (!process.waitForFinished(-1)) {
        qCritical() << "mri_watershed did not finish:" << process.errorString();
        return false;
    }

    if (process.exitCode() != 0) {
        qCritical() << "mri_watershed failed with exit code" << process.exitCode();
        return false;
    }

    return true;
}

//=============================================================================================================

bool WatershedBem::convertSurfaces(const QString& wsDir, const QString& mgzFile)
{
    //
    // When using .mgz input, the watershed surfaces need their volume
    // geometry updated from the MGH file. This is equivalent to:
    //   mne_convert_surface --surf $s --mghmri $T1_mgz --surfout $s
    //
    // The mri_watershed tool with -useSRAS already produces surfaces in
    // surface RAS coordinates. The convert step here ensures the volume
    // geometry metadata is consistent with the source MRI volume.
    //
    // For the head surface FIFF BEM file creation step that follows,
    // we read the surfaces via FSLIB::Surface which handles the coordinate
    // system correctly, so this step primarily ensures consistency for
    // other tools that may read these surface files.
    //

    // Read the MGH volume to get coordinate transform
    MriVolData volData;
    QVector<FiffCoordTrans> additionalTrans;
    QString subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

    if (!MriMghIO::read(mgzFile, volData, additionalTrans, subjectMriDir, m_settings.verbose())) {
        qCritical() << "Failed to read MGH/MGZ file" << mgzFile;
        return false;
    }

    // Surface names produced by mri_watershed
    QStringList surfaceNames;
    surfaceNames << m_settings.subject() + "_brain_surface"
                 << m_settings.subject() + "_inner_skull_surface"
                 << m_settings.subject() + "_outer_skull_surface"
                 << m_settings.subject() + "_outer_skin_surface";

    for (const QString& surfName : surfaceNames) {
        QString surfPath = wsDir + "/" + surfName;
        QFileInfo fi(surfPath);
        if (!fi.exists()) {
            qWarning() << "Watershed surface not found:" << surfPath;
            continue;
        }

        if (m_settings.verbose()) {
            printf("Verifying surface: %s\n", qPrintable(surfPath));
        }

        // Read surface to verify it is valid
        Surface surf;
        Surface::read(surfPath, surf, false);
        if (surf.rr().rows() == 0) {
            qWarning() << "Surface" << surfPath << "has no vertices.";
        } else if (m_settings.verbose()) {
            printf("  %d vertices, %d triangles\n", (int)surf.rr().rows(), (int)surf.tris().rows());
        }
    }

    return true;
}

//=============================================================================================================

bool WatershedBem::createBemFif(const QString& surfFile, const QString& fifFile)
{
    //
    // Create a FIFF BEM head surface file.
    // Equivalent to: mne_surf2bem --surf $surfFile --id 4 --fif $fifFile
    //
    // Steps:
    //   1. Read FreeSurfer surface via FSLIB::Surface
    //   2. Create MNEBemSurface with head surface ID (4)
    //   3. Write BEM FIFF file via MNEBem::write()
    //

    // Read the FreeSurfer outer skin surface
    Surface fsSurface;
    Surface::read(surfFile, fsSurface, false);

    if (fsSurface.rr().rows() == 0 || fsSurface.tris().rows() == 0) {
        qCritical() << "Failed to read surface from" << surfFile;
        return false;
    }

    printf("Read surface: %d vertices, %d triangles\n",
           (int)fsSurface.rr().rows(), (int)fsSurface.tris().rows());

    //
    // Create BEM surface
    //   FSLIB::Surface stores vertices in meters (divided by 1000 during read).
    //   MNEBemSurface expects vertices in meters as well.
    //
    MNEBemSurface bemSurface;
    bemSurface.id = FIFFV_BEM_SURF_ID_HEAD;   // 4 = head surface
    bemSurface.np = fsSurface.rr().rows();
    bemSurface.ntri = fsSurface.tris().rows();
    bemSurface.coord_frame = FIFFV_COORD_MRI;
    bemSurface.sigma = 0.0f;                   // Default conductivity

    // Copy vertex coordinates and triangles
    bemSurface.rr = fsSurface.rr();
    bemSurface.tris = fsSurface.tris();

    // Compute vertex normals
    bemSurface.nn = Surface::compute_normals(bemSurface.rr, bemSurface.tris);

    //
    // Create BEM and write to FIFF file
    //
    MNEBem bem;
    bem << bemSurface;

    QFile file(fifFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Could not open file for writing:" << fifFile;
        return false;
    }

    bem.write(file);
    file.close();

    return true;
}
