//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     setupmri.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    SetupMri class definition.
 *           Ported from the original MNE C tools by Matti Hamalainen:
 *             - mne_setup_mri (shell script): directory orchestration
 *             - mne_make_cor_set (C program): COR/MGZ to COR.fif conversion
 *
 *           Cross-referenced with MNE-Python, which never ported this tool.
 *           In modern MNE-Python, .mgz files are read directly via nibabel,
 *           making the COR.fif conversion unnecessary. However, this tool
 *           maintains compatibility with the original MNE C workflow.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "setupmri.h"
#include "mne_setup_mri_settings.h"

#include <mne/mri/mri_mgh_io.h>
#include <mne/mri/mri_cor_io.h>
#include <mne/mri/mri_cor_fif_io.h>
#include <mne/mri/mri_vol_data.h>
#include <mne/mri/mri_types.h>

#include <mne/fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SetupMri::SetupMri(const MNESetupMriSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int SetupMri::run()
{
    // Validate settings
    if (m_settings.subjectsDir().isEmpty()) {
        Log::critical("The subjects directory (SUBJECTS_DIR) is not set.\nUse --subjects-dir or set the SUBJECTS_DIR environment variable.");
        return 1;
    }
    if (m_settings.subject().isEmpty()) {
        Log::critical("The subject name is not set.\nUse --subject or set the SUBJECT environment variable.");
        return 1;
    }

    QString subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    QString mriDir = subjectDir + "/mri";

    QDir dir(subjectDir);
    if (!dir.exists()) {
        Log::critical("Could not find the MRI data directory {}", subjectDir.toStdString());
        return 1;
    }
    QDir mDir(mriDir);
    if (!mDir.exists()) {
        Log::critical("Could not find the MRI directory {}", mriDir.toStdString());
        return 1;
    }

    // Process each MRI set
    QStringList mriSets = m_settings.mriSets();
    for (const QString& mriName : mriSets) {
        Log::info("-----------------------------------------------------------------------------------------------");
        Log::info("Setting up {}...", mriName.toStdString());

        if (!processMriSet(mriName)) {
            return 1;
        }
    }

    Log::info("-----------------------------------------------------------------------------------------------");
    Log::info("\nComplete.\n");
    return 0;
}

//=============================================================================================================

bool SetupMri::processMriSet(const QString& mriName)
{
    QString mriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

    // Determine paths, handling absolute paths with '/'
    // Ported from the shell script logic: if mri name contains '/', use as-is
    QString thisDir;
    QString thisMgz;
    QString neuromagDir;

    if (mriName.contains('/')) {
        QFileInfo fi(mriName);
        thisDir = fi.absoluteFilePath();
        thisMgz = fi.absoluteFilePath() + ".mgz";
        neuromagDir = fi.absolutePath() + "/" + fi.fileName() + "-neuromag";
    } else {
        thisDir = mriDir + "/" + mriName;
        thisMgz = mriDir + "/" + mriName + ".mgz";
        neuromagDir = mriDir + "/" + mriName + "-neuromag";
    }

    // Check that MRI data exists (either directory or .mgz file)
    QDir mriDataDir(thisDir);
    QFileInfo mgzInfo(thisMgz);
    if (!mriDataDir.exists() && !mgzInfo.exists()) {
        // Also try .mgh extension
        QString thisMgh = thisMgz;
        thisMgh.replace(".mgz", ".mgh");
        QFileInfo mghInfo(thisMgh);
        if (mghInfo.exists()) {
            thisMgz = thisMgh;
            mgzInfo = QFileInfo(thisMgz);
        } else {
            Log::critical("Could not find the MRI data for {}", mriName.toStdString());
            Log::critical("  Looked for directory: {}", thisDir.toStdString());
            Log::critical("  Looked for file: {}", thisMgz.toStdString());
            return false;
        }
    }

    // Check for existing output
    QDir neuromagDirObj(neuromagDir);
    if (neuromagDirObj.exists() && !m_settings.overwrite()) {
        Log::critical("Destination directory {} already exists.", neuromagDir.toStdString());
        Log::critical("Use --overwrite option to overwrite existing data.");
        return false;
    }

    // Create directory structure
    // Ported from shell script: mkdir -p $neuromag_dir/sets, $neuromag_dir/slices
    QString setsDir = neuromagDir + "/sets";
    QString slicesDir = neuromagDir + "/slices";

    if (!QDir().mkpath(setsDir)) {
        Log::critical("Could not create sets directory {}", setsDir.toStdString());
        return false;
    }
    if (!QDir().mkpath(slicesDir)) {
        Log::critical("Could not create slices directory {}", slicesDir.toStdString());
        return false;
    }

    // Read MRI data and create COR.fif
    std::vector<MriSlice> slices;
    std::vector<FiffCoordTrans> additionalTrans;

    if (mgzInfo.exists()) {
        // MGZ/MGH path — use mne::MriMghIO
        Log::info("Reading {}...", thisMgz.toStdString());

        MriVolData volData;
        QString subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

        if (!MriMghIO::read(thisMgz.toStdString(), volData, additionalTrans, subjectMriDir.toStdString(), m_settings.verbose())) {
            Log::critical("Failed to read MGZ/MGH file {}", thisMgz.toStdString());
            return false;
        }
        slices = volData.slices;
    } else {
        // COR slices path — use mne::MriCorIO
        Log::info("Reading COR files from {}...", thisDir.toStdString());

        // Create symbolic links (like the shell script does)
        QDir corDir(thisDir);
        QStringList corFiles = corDir.entryList(QStringList() << "COR-*", QDir::Files, QDir::Name);
        for (const QString& corFile : corFiles) {
            QString src = thisDir + "/" + corFile;
            QString dst = slicesDir + "/" + corFile;
            QFile::remove(dst);
            if (!QFile::link(src, dst)) {
                Log::warning("Could not create symbolic link: {} -> {}", dst.toStdString(), src.toStdString());
            }
        }

        if (!MriCorIO::read(thisDir.toStdString(), slices, m_settings.verbose())) {
            Log::critical("Failed to read COR slices from {}", thisDir.toStdString());
            return false;
        }
    }

    // Write COR.fif — use mne::MriCorFifIO
    QString corFifPath = setsDir + "/COR.fif";
    QFile::remove(corFifPath);

    Log::info("Creating {}...", corFifPath.toStdString());
    if (!MriCorFifIO::write(corFifPath.toStdString(), slices, additionalTrans)) {
        Log::critical("Failed to write {}", corFifPath.toStdString());
        return false;
    }
    Log::info("Created {}", corFifPath.toStdString());

    return true;
}
