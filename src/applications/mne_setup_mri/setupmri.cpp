//=============================================================================================================
/**
 * @file     setupmri.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    SetupMri class definition.
 *           Ported from the original MNE C tools by Matti Hamalainen:
 *             - mne_setup_mri (shell script): directory orchestration
 *             - mne_make_cor_set (C program): COR/MGZ to COR.fif conversion
 *
 *           Cross-referenced with MNE-Python, which never ported this tool.
 *           In modern MNE-Python, .mgz files are read directly via nibabel,
 *           making the COR.fif conversion unnecessary. However, this tool
 *           maintains compatibility with the original MNE C workflow.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "setupmri.h"
#include "mne_setup_mri_settings.h"

#include <mri/mri_mgh_io.h>
#include <mri/mri_cor_io.h>
#include <mri/mri_cor_fif_io.h>
#include <mri/mri_vol_data.h>
#include <mri/mri_types.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;
using namespace MRILIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SetupMri::SetupMri(const MneSetupMriSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int SetupMri::run()
{
    // Validate settings
    if (m_settings.subjectsDir().isEmpty()) {
        qCritical() << "The subjects directory (SUBJECTS_DIR) is not set.\n"
                     << "Use --subjects-dir or set the SUBJECTS_DIR environment variable.";
        return 1;
    }
    if (m_settings.subject().isEmpty()) {
        qCritical() << "The subject name is not set.\n"
                     << "Use --subject or set the SUBJECT environment variable.";
        return 1;
    }

    QString subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    QString mriDir = subjectDir + "/mri";

    QDir dir(subjectDir);
    if (!dir.exists()) {
        qCritical() << "Could not find the MRI data directory" << subjectDir;
        return 1;
    }
    QDir mDir(mriDir);
    if (!mDir.exists()) {
        qCritical() << "Could not find the MRI directory" << mriDir;
        return 1;
    }

    // Process each MRI set
    QStringList mriSets = m_settings.mriSets();
    for (const QString& mriName : mriSets) {
        printf("-----------------------------------------------------------------------------------------------\n");
        printf("Setting up %s...\n", qPrintable(mriName));

        if (!processMriSet(mriName)) {
            return 1;
        }
    }

    printf("-----------------------------------------------------------------------------------------------\n");
    printf("\nComplete.\n\n");
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
            qCritical() << "Could not find the MRI data for" << mriName;
            qCritical() << "  Looked for directory:" << thisDir;
            qCritical() << "  Looked for file:" << thisMgz;
            return false;
        }
    }

    // Check for existing output
    QDir neuromagDirObj(neuromagDir);
    if (neuromagDirObj.exists() && !m_settings.overwrite()) {
        qCritical() << "Destination directory" << neuromagDir << "already exists.";
        qCritical() << "Use --overwrite option to overwrite existing data.";
        return false;
    }

    // Create directory structure
    // Ported from shell script: mkdir -p $neuromag_dir/sets, $neuromag_dir/slices
    QString setsDir = neuromagDir + "/sets";
    QString slicesDir = neuromagDir + "/slices";

    if (!QDir().mkpath(setsDir)) {
        qCritical() << "Could not create sets directory" << setsDir;
        return false;
    }
    if (!QDir().mkpath(slicesDir)) {
        qCritical() << "Could not create slices directory" << slicesDir;
        return false;
    }

    // Read MRI data and create COR.fif
    QVector<MriSlice> slices;
    QVector<FiffCoordTrans> additionalTrans;

    if (mgzInfo.exists()) {
        // MGZ/MGH path — use MRILIB::MriMghIO
        printf("Reading %s...\n", qPrintable(thisMgz));

        MriVolData volData;
        QString subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

        if (!MriMghIO::read(thisMgz, volData, additionalTrans, subjectMriDir, m_settings.verbose())) {
            qCritical() << "Failed to read MGZ/MGH file" << thisMgz;
            return false;
        }
        slices = volData.slices;
    } else {
        // COR slices path — use MRILIB::MriCorIO
        printf("Reading COR files from %s...\n", qPrintable(thisDir));

        // Create symbolic links (like the shell script does)
        QDir corDir(thisDir);
        QStringList corFiles = corDir.entryList(QStringList() << "COR-*", QDir::Files, QDir::Name);
        for (const QString& corFile : corFiles) {
            QString src = thisDir + "/" + corFile;
            QString dst = slicesDir + "/" + corFile;
            QFile::remove(dst);
            if (!QFile::link(src, dst)) {
                qWarning() << "Could not create symbolic link:" << dst << "->" << src;
            }
        }

        if (!MriCorIO::read(thisDir, slices, m_settings.verbose())) {
            qCritical() << "Failed to read COR slices from" << thisDir;
            return false;
        }
    }

    // Write COR.fif — use MRILIB::MriCorFifIO
    QString corFifPath = setsDir + "/COR.fif";
    QFile::remove(corFifPath);

    printf("Creating %s...\n", qPrintable(corFifPath));
    if (!MriCorFifIO::write(corFifPath, slices, additionalTrans)) {
        qCritical() << "Failed to write" << corFifPath;
        return false;
    }
    printf("Created %s\n", qPrintable(corFifPath));

    return true;
}
