//=============================================================================================================
/**
 * @file     mnxproject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 *
 * @brief    MnxProject class declaration — ZIP-based project container for mne_browse.
 *
 */

#ifndef MNXPROJECT_H
#define MNXPROJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QString>
#include <QJsonObject>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTemporaryDir;

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace MNEBROWSE {

//=============================================================================================================
/**
 * MnxProject manages a ZIP-based project container (.mnx) for mne_browse.
 *
 * On open:  The archive is extracted to a temporary directory.  All subsequent
 *           file reads and writes operate on the unpacked tree so that existing
 *           FIF I/O code (FiffBlockReader, FiffStream, etc.) works unchanged.
 *
 * On save:  The temporary directory is packed back into a ZIP archive.
 *
 * Directory layout inside the archive (BIDS-inspired):
 *
 *     manifest.json          – version, metadata, view/processing settings
 *     meg/
 *       sub-01_task-raw.fif  – raw recording
 *       sub-01_task-eve.fif  – events        (optional)
 *       sub-01_task-annot.json – annotations  (optional)
 *       sub-01_task-virtchan.json – virtual channels (optional)
 *       sub-01_task-ave.fif  – evoked         (optional)
 *       sub-01_task-cov.fif  – covariance     (optional)
 *       sub-01_task-inv.fif  – inverse op     (optional)
 */
class MnxProject
{
public:
    MnxProject();
    ~MnxProject();

    // Non-copyable, movable
    MnxProject(const MnxProject&) = delete;
    MnxProject& operator=(const MnxProject&) = delete;
    MnxProject(MnxProject&&) noexcept;
    MnxProject& operator=(MnxProject&&) noexcept;

    /**
     * Open an existing .mnx archive.
     * Extracts all contents to a temporary directory.
     * @param[in] mnxPath   Path to the .mnx file.
     * @return true on success.
     */
    bool open(const QString& mnxPath);

    /**
     * Create a new project from existing files.
     * Copies the specified files into a fresh temporary directory.
     * @param[in] rawPath         Path to the raw FIF file (required).
     * @param[in] eventPath       Path to the event file (optional, empty to skip).
     * @param[in] annotationPath  Path to the annotation file (optional).
     * @param[in] virtualChanPath Path to the virtual-channel file (optional).
     * @param[in] evokedPath      Path to the evoked file (optional).
     * @param[in] covPath         Path to the covariance file (optional).
     * @param[in] inversePath     Path to the inverse-operator file (optional).
     * @return true on success.
     */
    bool create(const QString& rawPath,
                const QString& eventPath = {},
                const QString& annotationPath = {},
                const QString& virtualChanPath = {},
                const QString& evokedPath = {},
                const QString& covPath = {},
                const QString& inversePath = {});

    /**
     * Save (pack) the temporary directory into a .mnx ZIP archive.
     * @param[in] mnxPath   Destination path for the .mnx file.
     * @param[in] settings  Optional JSON object with view/processing settings to embed.
     * @return true on success.
     */
    bool save(const QString& mnxPath, const QJsonObject& settings = {});

    /** @return true if a project is currently open (temp dir exists). */
    bool isOpen() const;

    /** Close the project and remove the temporary directory. */
    void close();

    /** @return Path to the .mnx file that was opened (empty for unsaved new projects). */
    QString mnxFilePath() const { return m_mnxFilePath; }

    // ── Paths to extracted files (empty if not present) ──────────────

    /** @return Path to the raw FIF file inside the temp directory. */
    QString rawPath() const;

    /** @return Path to the event file inside the temp directory. */
    QString eventPath() const;

    /** @return Path to the annotation file inside the temp directory. */
    QString annotationPath() const;

    /** @return Path to the virtual-channel file inside the temp directory. */
    QString virtualChannelPath() const;

    /** @return Path to the evoked file inside the temp directory. */
    QString evokedPath() const;

    /** @return Path to the covariance file inside the temp directory. */
    QString covariancePath() const;

    /** @return Path to the inverse-operator file inside the temp directory. */
    QString inversePath() const;

    /** @return The parsed manifest settings (view, filter, averaging, etc.). */
    QJsonObject settings() const { return m_settings; }

    /** @return The base temp directory path. */
    QString tempDir() const;

private:
    /** Scan the temp directory and populate file path members. */
    void discoverFiles();

    /** Copy a file into the meg/ subdirectory with the given target name. */
    bool copyIntoProject(const QString& srcPath, const QString& targetName);

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_mnxFilePath;
    QJsonObject m_settings;

    // Discovered paths inside the temp directory
    QString m_rawPath;
    QString m_eventPath;
    QString m_annotationPath;
    QString m_virtualChanPath;
    QString m_evokedPath;
    QString m_covPath;
    QString m_inversePath;
};

} // namespace MNEBROWSE

#endif // MNXPROJECT_H
