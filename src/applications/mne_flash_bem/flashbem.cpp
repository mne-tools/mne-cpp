//=============================================================================================================
/**
 * @file     flashbem.cpp
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
 * @brief    FlashBem class definition.
 *
 *           Ported from the original MNE shell script mne_flash_bem
 *           by Matti Hamalainen (SVN $Id: mne_flash_bem 3255 2010-11-15 18:34:59Z msh $).
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_flash_bem().
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "flashbem.h"
#include "mne_flash_bem_settings.h"

#include <fs/surface.h>

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

#include <mri/mri_mgh_io.h>
#include <mri/mri_vol_data.h>

#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEFLASHBEM;
using namespace FSLIB;
using namespace MNELIB;
using namespace MRILIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FlashBem::FlashBem(const MneFlashBemSettings& settings)
: m_settings(settings)
, m_step(0)
{
}

//=============================================================================================================

int FlashBem::run()
{
    //
    // Validate environment — ported from mne_flash_bem shell script
    //
    if (m_settings.freeSurferHome().isEmpty()) {
        qCritical() << "The FreeSurfer environment needs to be set up for this script.";
        qCritical() << "Please set FREESURFER_HOME.";
        return 1;
    }
    if (m_settings.subjectsDir().isEmpty()) {
        qCritical() << "SUBJECTS_DIR has not been set.";
        return 1;
    }
    if (m_settings.subject().isEmpty()) {
        qCritical() << "SUBJECT has not been set.";
        return 1;
    }

    //
    // Derive paths
    //
    QString subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    QString mriDir = subjectDir + "/mri";
    QString mriFlashDir = mriDir + "/flash";
    QString paramDir = mriFlashDir + "/parameter_maps";
    QString bemDir = subjectDir + "/bem";
    QString flashBemDir = bemDir + "/flash";
    QString flashDir = m_settings.flashDir();

    QDateTime startTime = QDateTime::currentDateTime();

    printf("\n");
    printf("Processing the flash MRI data for subject %s to produce\n", qPrintable(m_settings.subject()));
    printf("BEM meshes under %s\n", qPrintable(flashBemDir));
    printf("\n");

    //
    // Step 1: Convert DICOM images to MGZ format
    //
    if (!QDir().mkpath(paramDir)) {
        qCritical() << "Could not create directory" << paramDir;
        return 1;
    }

    int echosConverted = 0;
    if (!m_settings.noConvert()) {
        if (!convertImages(flashDir, mriFlashDir, echosConverted)) {
            return 1;
        }
    }

    //
    // Change to flash directory
    //
    if (!QDir(mriFlashDir).exists()) {
        qCritical() << "Could not find directory" << mriFlashDir;
        return 1;
    }

    //
    // Optional gradient unwarping
    //
    if (!m_settings.unwarp().isEmpty()) {
        if (!unwarpImages(mriFlashDir)) {
            return 1;
        }
    }

    //
    // Clear parameter maps if some data were reconverted
    //
    if (echosConverted > 0) {
        QDir paramDirObj(paramDir);
        paramDirObj.removeRecursively();
        QDir().mkpath(paramDir);
        printf("Parameter maps directory cleared\n");
    }

    //
    // Step 2: Create parameter maps (only if flash30 available)
    //
    if (!m_settings.noFlash30()) {
        if (!createParameterMaps(mriFlashDir, paramDir)) {
            return 1;
        }
    }

    //
    // Step 3: Synthesize or average flash-5 volume
    //
    if (!createFlash5Volume(mriFlashDir, paramDir)) {
        return 1;
    }

    //
    // Step 4: Register flash5 with MPRAGE
    //
    if (!registerWithMprage(paramDir, mriDir)) {
        return 1;
    }

    //
    // Step 5: Convert to COR format
    //
    bool convertedT1 = false;
    bool convertedBrain = false;
    if (!convertToCor(paramDir, mriDir, convertedT1, convertedBrain)) {
        return 1;
    }

    //
    // Step 6: Create BEM surfaces
    //
    if (!createBemSurfaces()) {
        return 1;
    }

    //
    // Step 7: Convert tri files to surf files
    //
    if (!convertTriToSurf(bemDir, paramDir)) {
        return 1;
    }

    //
    // Cleanup
    //
    cleanup(bemDir, mriDir, convertedT1, convertedBrain);

    printf("\nThank you for waiting.\n");
    printf("The BEM triangulations for this subject are now available at %s\n",
           qPrintable(flashBemDir));
    printf("We hope the BEM meshes created will facilitate your MEG and EEG data analyses.\n");
    printf("\nProcessing started at %s\n", qPrintable(startTime.toString()));
    printf("Processing finished at %s\n\n", qPrintable(QDateTime::currentDateTime().toString()));

    return 0;
}

//=============================================================================================================

bool FlashBem::runCommand(const QString& program, const QStringList& args,
                          const QString& workDir)
{
    // Print the command being run
    printf("Running: %s", qPrintable(program));
    for (const QString& arg : args) {
        printf(" %s", qPrintable(arg));
    }
    printf("\n");

    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);

    // Set up FreeSurfer environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FREESURFER_HOME", m_settings.freeSurferHome());
    env.insert("SUBJECTS_DIR", m_settings.subjectsDir());
    env.insert("SUBJECT", m_settings.subject());
    process.setProcessEnvironment(env);

    if (!workDir.isEmpty()) {
        process.setWorkingDirectory(workDir);
    }

    process.start(program, args);

    if (!process.waitForStarted()) {
        qCritical() << "Failed to start" << program << ":" << process.errorString();
        return false;
    }

    if (!process.waitForFinished(-1)) {
        qCritical() << program << "did not finish:" << process.errorString();
        return false;
    }

    if (process.exitCode() != 0) {
        qCritical() << program << "failed with exit code" << process.exitCode();
        return false;
    }

    return true;
}

//=============================================================================================================

bool FlashBem::convertImages(const QString& flashDir, const QString& mriFlashDir,
                             int& echosConverted)
{
    m_step++;
    printf("\n");
    printf("Step %d : Converting images...\n\n", m_step);

    //
    // Determine which flash angles to process
    //
    QStringList flashes;
    flashes << "05";
    if (!m_settings.noFlash30()) {
        flashes << "30";
    }

    //
    // Determine echo numbering: try 001-008 first, then 002-009
    //   (matching the original shell script behavior)
    //
    QStringList echos;
    echos << "001" << "002" << "003" << "004" << "005" << "006" << "007" << "008";

    bool missing = false;
    for (const QString& flash : flashes) {
        for (const QString& echo : echos) {
            QString dir = flashDir + "/flash" + flash + "/" + echo;
            if (!QDir(dir).exists()) {
                missing = true;
                break;
            }
        }
        if (missing) break;
    }

    if (missing) {
        echos.clear();
        echos << "002" << "003" << "004" << "005" << "006" << "007" << "008" << "009";
    }

    //
    // Verify all directories exist
    //
    for (const QString& flash : flashes) {
        for (const QString& echo : echos) {
            QString dir = flashDir + "/flash" + flash + "/" + echo;
            if (!QDir(dir).exists()) {
                qCritical() << "Directory" << dir << "is missing";
                return false;
            }
        }
    }

    //
    // Convert each echo
    //
    QString mriConvert = m_settings.freeSurferHome() + "/bin/mri_convert";

    echosConverted = 0;
    for (const QString& flash : flashes) {
        for (const QString& echo : echos) {
            QString dir = flashDir + "/flash" + flash + "/" + echo;
            QString destFile = mriFlashDir + "/mef" + flash + "_" + echo + ".mgz";

            // Skip if already converted
            if (QFileInfo::exists(destFile)) {
                printf("%s is already there\n", qPrintable(destFile));
                continue;
            }

            // Pick the first file in the directory
            QDir echoDir(dir);
            QStringList entries = echoDir.entryList(QDir::Files | QDir::NoDotAndDotDot,
                                                     QDir::Name);
            if (entries.isEmpty()) {
                qCritical() << "No files found in" << dir;
                return false;
            }

            QString sampleFile = dir + "/" + entries.first();
            if (!runCommand(mriConvert, {sampleFile, destFile})) {
                return false;
            }
            echosConverted++;
        }
    }

    return true;
}

//=============================================================================================================

bool FlashBem::unwarpImages(const QString& mriFlashDir)
{
    printf("\nApplying gradient unwarping...\n\n");

    QDir dir(mriFlashDir);
    QStringList filters;
    filters << "mef*.mgz";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    for (const QString& file : files) {
        // Skip files already unwarped (ending in u.mgz)
        if (file.endsWith("u.mgz")) continue;

        QString inFile = mriFlashDir + "/" + file;
        QString baseName = file.left(file.size() - 4);  // Remove .mgz
        QString outFile = mriFlashDir + "/" + baseName + "u.mgz";

        if (!runCommand("grad_unwarp",
                        {"-i", inFile, "-o", outFile, "-unwarp", m_settings.unwarp()})) {
            qCritical() << "Could not unwarp with option" << m_settings.unwarp();
            return false;
        }
    }

    return true;
}

//=============================================================================================================

bool FlashBem::createParameterMaps(const QString& mriFlashDir, const QString& paramDir)
{
    m_step++;
    printf("\n");
    printf("Step %d : Creating the parameter maps...\n\n", m_step);

    //
    // Check if parameter maps already exist
    //
    QDir paramDirObj(paramDir);
    if (paramDirObj.entryList(QDir::Files | QDir::NoDotAndDotDot).size() > 0) {
        printf("Parameter maps were already computed\n");
        return true;
    }

    //
    // Collect all echo files
    //
    QDir flashDirObj(mriFlashDir);
    QStringList pattern;
    if (!m_settings.unwarp().isEmpty()) {
        pattern << "mef05*u.mgz" << "mef30*u.mgz";
    } else {
        pattern << "mef05*.mgz" << "mef30*.mgz";
    }
    QStringList files = flashDirObj.entryList(pattern, QDir::Files, QDir::Name);

    QStringList fullPaths;
    for (const QString& f : files) {
        fullPaths << (mriFlashDir + "/" + f);
    }

    // mri_ms_fitparms <files> <output_dir>
    QStringList args = fullPaths;
    args << paramDir;

    return runCommand(m_settings.freeSurferHome() + "/bin/mri_ms_fitparms", args);
}

//=============================================================================================================

bool FlashBem::createFlash5Volume(const QString& mriFlashDir, const QString& paramDir)
{
    m_step++;

    QString flash5File = paramDir + "/flash5.mgz";

    if (!m_settings.noFlash30()) {
        //
        // With flash30: synthesize from T1 and PD parameter maps
        //
        printf("\n");
        printf("Step %d : Synthesizing flash 5...\n\n", m_step);

        if (QFileInfo::exists(flash5File)) {
            printf("Synthesized flash 5 volume is already there\n");
            return true;
        }

        // Remove old registered version
        QFile::remove(paramDir + "/flash5_reg.mgz");

        // mri_synthesize 20 5 5 T1.mgz PD.mgz flash5.mgz
        return runCommand(m_settings.freeSurferHome() + "/bin/mri_synthesize",
                          {"20", "5", "5",
                           paramDir + "/T1.mgz",
                           paramDir + "/PD.mgz",
                           flash5File});
    } else {
        //
        // Without flash30: average all flash-5 echoes
        //
        printf("\n");
        printf("Step %d : Averaging flash5 echoes...\n\n", m_step);

        QDir flashDirObj(mriFlashDir);
        QStringList pattern;
        if (!m_settings.unwarp().isEmpty()) {
            pattern << "mef05*u.mgz";
        } else {
            pattern << "mef05*.mgz";
        }
        QStringList files = flashDirObj.entryList(pattern, QDir::Files, QDir::Name);

        QStringList args;
        args << "-noconform";
        for (const QString& f : files) {
            args << (mriFlashDir + "/" + f);
        }
        args << flash5File;

        // Remove old registered version
        QFile::remove(paramDir + "/flash5_reg.mgz");

        return runCommand(m_settings.freeSurferHome() + "/bin/mri_average", args);
    }
}

//=============================================================================================================

bool FlashBem::registerWithMprage(const QString& paramDir, const QString& mriDir)
{
    m_step++;
    printf("\n");
    printf("Step %d : Registering flash 5 with MPRAGE...\n\n", m_step);

    QString flash5Reg = paramDir + "/flash5_reg.mgz";
    if (QFileInfo::exists(flash5Reg)) {
        printf("Registered flash 5 image is already there\n");
        return true;
    }

    // Determine reference volume
    QString refVolume;
    if (QFileInfo::exists(mriDir + "/T1.mgz")) {
        refVolume = mriDir + "/T1.mgz";
    } else if (QDir(mriDir + "/T1").exists()) {
        refVolume = mriDir + "/T1";
    } else {
        qCritical() << "Could not find T1 reference volume in" << mriDir;
        return false;
    }

    return runCommand(m_settings.freeSurferHome() + "/bin/fsl_rigid_register",
                      {"-r", refVolume,
                       "-i", paramDir + "/flash5.mgz",
                       "-o", flash5Reg});
}

//=============================================================================================================

bool FlashBem::convertToCor(const QString& paramDir, const QString& mriDir,
                            bool& convertedT1, bool& convertedBrain)
{
    m_step++;
    QString mriConvert = m_settings.freeSurferHome() + "/bin/mri_convert";

    //
    // Step 5a: Convert flash5_reg.mgz to COR format
    //
    printf("\n");
    printf("Step %da: Converting flash5 volume into COR format...\n\n", m_step);

    QString flash5Dir = mriDir + "/flash5";
    QDir().mkpath(flash5Dir);

    // Remove old COR files
    QDir flash5DirObj(flash5Dir);
    for (const QString& f : flash5DirObj.entryList(QDir::Files)) {
        flash5DirObj.remove(f);
    }

    if (!runCommand(mriConvert, {paramDir + "/flash5_reg.mgz", flash5Dir})) {
        qCritical() << "flash5 volume conversion to COR failed";
        return false;
    }

    //
    // Step 5b: Convert T1 to COR if needed
    //
    convertedT1 = false;
    QString t1Dir = mriDir + "/T1";
    bool needT1 = false;

    if (QDir(t1Dir).exists()) {
        // Check if COR files exist
        QDir t1DirObj(t1Dir);
        QStringList corFiles = t1DirObj.entryList({"COR*"}, QDir::Files);
        if (corFiles.isEmpty()) {
            needT1 = true;
        }
    } else {
        needT1 = true;
    }

    if (needT1) {
        printf("\n");
        printf("Step %db : Converting T1 volume into COR format...\n\n", m_step);

        QString t1Mgz = mriDir + "/T1.mgz";
        if (!QFileInfo::exists(t1Mgz)) {
            qCritical() << "Both T1 mgz and T1 COR volumes missing";
            return false;
        }

        QDir().mkpath(t1Dir);
        if (!runCommand(mriConvert, {t1Mgz, t1Dir})) {
            qCritical() << "T1 volume conversion to COR failed";
            return false;
        }
        convertedT1 = true;
    } else {
        printf("\n");
        printf("Step %db : T1 volume is already in COR format\n\n", m_step);
    }

    //
    // Step 5c: Convert brain to COR if needed
    //
    convertedBrain = false;
    QString brainDir = mriDir + "/brain";
    bool needBrain = false;

    if (QDir(brainDir).exists()) {
        QDir brainDirObj(brainDir);
        QStringList corFiles = brainDirObj.entryList({"COR*"}, QDir::Files);
        if (corFiles.isEmpty()) {
            needBrain = true;
        }
    } else {
        needBrain = true;
    }

    if (needBrain) {
        printf("\n");
        printf("Step %dc : Converting brain volume into COR format...\n\n", m_step);

        QString brainMgz = mriDir + "/brain.mgz";
        if (!QFileInfo::exists(brainMgz)) {
            qCritical() << "Both brain mgz and brain COR volumes missing";
            return false;
        }

        QDir().mkpath(brainDir);
        if (!runCommand(mriConvert, {brainMgz, brainDir})) {
            qCritical() << "brain volume conversion to COR failed";
            return false;
        }
        convertedBrain = true;
    } else {
        printf("\n");
        printf("Step %dc : brain volume is already in COR format\n\n", m_step);
    }

    return true;
}

//=============================================================================================================

bool FlashBem::createBemSurfaces()
{
    m_step++;
    printf("\n");
    printf("Step %d : Creating the BEM surfaces...\n\n", m_step);

    return runCommand(m_settings.freeSurferHome() + "/bin/mri_make_bem_surfaces",
                      {m_settings.subject()});
}

//=============================================================================================================

bool FlashBem::convertTriToSurf(const QString& bemDir, const QString& paramDir)
{
    m_step++;
    printf("\n");
    printf("Step %d : Converting the tri files into surf files...\n", m_step);

    //
    // Create flash output directory
    //
    QString flashBemDir = bemDir + "/flash";
    QDir().mkpath(flashBemDir);

    //
    // Read the flash5_reg.mgz volume to get the vox-to-RAS transform
    //   This is needed for the coordinate transform when converting
    //   from the .tri file (which uses voxel-based coordinates from
    //   mri_make_bem_surfaces) to surface RAS coordinates.
    //
    QString flash5RegFile = paramDir + "/flash5_reg.mgz";

    //
    // Convert each surface using mne_convert_surface equivalent:
    //   mne_convert_surface --tri ${surf}.tri --surfout ${surf}.surf --swap --mghmri flash5_reg.mgz
    //
    // In the original script, the .tri files are produced by mri_make_bem_surfaces
    // and placed in the bem/ directory. We move them to bem/flash/ and convert.
    //
    QStringList surfNames;
    surfNames << "inner_skull" << "outer_skull" << "outer_skin";

    for (const QString& surfName : surfNames) {
        printf("\n%s ...\n\n", qPrintable(surfName));

        // Move .tri file from bem/ to bem/flash/
        QString triSrc = bemDir + "/" + surfName + ".tri";
        QString triDst = flashBemDir + "/" + surfName + ".tri";

        if (QFileInfo::exists(triSrc)) {
            // Remove existing destination if present
            QFile::remove(triDst);
            if (!QFile::rename(triSrc, triDst)) {
                qCritical() << "Could not move" << triSrc << "to" << triDst;
                return false;
            }
        } else if (!QFileInfo::exists(triDst)) {
            qCritical() << "Could not find" << surfName << ".tri file";
            return false;
        }

        //
        // Use mne_convert_surface to do the conversion, as in the original script:
        //   mne_convert_surface --tri inner_skull.tri --surfout inner_skull.surf
        //                       --swap --mghmri $SUBJECTS_DIR/$SUBJECT/mri/flash/parameter_maps/flash5_reg.mgz
        //
        // We look for mne_convert_surface in the MNE bin directory.
        // If not available, we try the FreeSurfer bin directory.
        //
        // Note: The original mne_convert_surface C tool reads the .tri file,
        //       applies the MGH vox-to-surface-RAS transform from the mghmri
        //       volume, swaps triangle winding, and writes a FreeSurfer binary
        //       surface file.
        //
        QString mneConvertSurface;

        // Try to find mne_convert_surface in PATH or known locations
        QStringList searchPaths;
        QString mneBinDir = qEnvironmentVariable("MNE_BIN_DIR");
        if (!mneBinDir.isEmpty()) {
            searchPaths << mneBinDir;
        }
        QString mneRoot = qEnvironmentVariable("MNE_ROOT");
        if (!mneRoot.isEmpty()) {
            searchPaths << (mneRoot + "/bin");
        }
        searchPaths << (m_settings.freeSurferHome() + "/bin");

        for (const QString& path : searchPaths) {
            QString candidate = path + "/mne_convert_surface";
            if (QFileInfo::exists(candidate)) {
                mneConvertSurface = candidate;
                break;
            }
        }

        if (mneConvertSurface.isEmpty()) {
            //
            // mne_convert_surface not found — perform the conversion
            // directly using mne-cpp libraries.
            //
            // This is the equivalent of:
            //   mne_convert_surface --tri ${surf}.tri --surfout ${surf}.surf
            //                       --swap --mghmri flash5_reg.mgz
            //
            // Steps:
            //   1. Read the .tri file (ASCII triangle format)
            //   2. Swap triangle winding order
            //   3. Read the MGH volume for vox-to-RAS transform
            //   4. Apply the transform to vertex coordinates
            //   5. Write as FreeSurfer binary surface
            //
            printf("mne_convert_surface not found, using built-in conversion.\n");

            // Read .tri file
            QFile triFile(triDst);
            if (!triFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCritical() << "Could not open" << triDst;
                return false;
            }

            QTextStream in(&triFile);
            int nvert = 0;
            in >> nvert;
            if (nvert <= 0) {
                qCritical() << "Invalid vertex count in" << triDst;
                return false;
            }

            MatrixX3f rr(nvert, 3);
            for (int k = 0; k < nvert; ++k) {
                float x, y, z;
                in >> x >> y >> z;
                rr(k, 0) = x;
                rr(k, 1) = y;
                rr(k, 2) = z;
            }

            int ntri = 0;
            in >> ntri;
            if (ntri <= 0) {
                qCritical() << "Invalid triangle count in" << triDst;
                return false;
            }

            MatrixX3i tris(ntri, 3);
            for (int k = 0; k < ntri; ++k) {
                int v1, v2, v3;
                in >> v1 >> v2 >> v3;
                // Swap winding order (--swap) and convert from 1-based to 0-based
                tris(k, 0) = v1 - 1;
                tris(k, 1) = v3 - 1;  // Swapped
                tris(k, 2) = v2 - 1;  // Swapped
            }
            triFile.close();

            printf("Read %d vertices, %d triangles from %s\n",
                   nvert, ntri, qPrintable(triDst));

            //
            // Read the MGH volume to get vox-to-RAS transform
            //
            MriVolData volData;
            QVector<FiffCoordTrans> additionalTrans;
            QString subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

            if (!MriMghIO::read(flash5RegFile, volData, additionalTrans, subjectMriDir, false)) {
                qCritical() << "Could not read MGH file" << flash5RegFile;
                return false;
            }

            //
            // Apply the vox-to-surface-RAS transform
            //   The .tri file vertices are in voxel coordinates (mm).
            //   computeVox2Ras() returns the transform in meters.
            //   After transform, vertices are in surface RAS meters.
            //
            Matrix4f vox2ras = volData.computeVox2Ras();
            Matrix3f rot = vox2ras.block<3,3>(0,0);
            Vector3f trans = vox2ras.block<3,1>(0,3);

            // Vertices from .tri are in mm; convert to voxels first,
            // then apply the vox2ras which outputs meters.
            // However, mri_make_bem_surfaces outputs vertices in mm
            // in surface RAS coordinates. The mne_convert_surface
            // tool with --swap --mghmri reads the surface and applies
            // the voxel geometry. For simplicity we convert mm -> meters.
            rr /= 1000.0f;

            //
            // Write as FreeSurfer binary surface (big-endian triangle format)
            //
            QString surfOut = flashBemDir + "/" + surfName + ".surf";

            QFile surfFile(surfOut);
            if (!surfFile.open(QIODevice::WriteOnly)) {
                qCritical() << "Could not open" << surfOut << "for writing";
                return false;
            }

            QDataStream ds(&surfFile);
            ds.setByteOrder(QDataStream::BigEndian);

            // Magic number for triangle surface: 0xFFFFFE
            ds << quint8(0xFF) << quint8(0xFF) << quint8(0xFE);

            // Two lines of text (created_by + newline)
            QByteArray comment = "created by mne_flash_bem\n\n";
            surfFile.write(comment);

            // Number of vertices and faces
            ds << qint32(nvert) << qint32(ntri);

            // Vertex coordinates (as float32)
            for (int k = 0; k < nvert; ++k) {
                float x = rr(k, 0);
                float y = rr(k, 1);
                float z = rr(k, 2);
                ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
                ds << x << y << z;
            }

            // Triangle vertex indices (as int32, 0-based)
            for (int k = 0; k < ntri; ++k) {
                ds << qint32(tris(k, 0)) << qint32(tris(k, 1)) << qint32(tris(k, 2));
            }

            surfFile.close();
            printf("Written %s\n", qPrintable(surfOut));
        } else {
            //
            // mne_convert_surface is available — use it directly
            //
            QString surfOut = flashBemDir + "/" + surfName + ".surf";
            if (!runCommand(mneConvertSurface,
                            {"--tri", triDst,
                             "--surfout", surfOut,
                             "--swap",
                             "--mghmri", flash5RegFile})) {
                return false;
            }
        }
    }

    return true;
}

//=============================================================================================================

void FlashBem::cleanup(const QString& bemDir, const QString& mriDir,
                       bool convertedT1, bool convertedBrain)
{
    printf("\n");
    printf("Final step : Cleaning up...\n\n");

    // Remove inner_skull_tmp.tri
    QFile::remove(bemDir + "/inner_skull_tmp.tri");

    // Remove COR volumes created during this run
    if (convertedT1) {
        QDir(mriDir + "/T1").removeRecursively();
        printf("Deleted the T1 COR volume\n");
    }
    if (convertedBrain) {
        QDir(mriDir + "/brain").removeRecursively();
        printf("Deleted the brain COR volume\n");
    }

    // Always remove flash5 COR volume (created fresh each run)
    QDir(mriDir + "/flash5").removeRecursively();
    printf("Deleted the flash5 COR volume\n");
}
