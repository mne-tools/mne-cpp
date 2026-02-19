//=============================================================================================================
/**
 * @file     mne_flash_bem_settings.h
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
 * @brief    MneFlashBemSettings class declaration.
 *
 */

#ifndef MNEFLASHBEMSETTINGS_H
#define MNEFLASHBEMSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNEFLASHBEM
//=============================================================================================================

namespace MNEFLASHBEM {

//=============================================================================================================
/**
 * Parses and stores command-line settings for mne_flash_bem.
 *
 * Supports the same options as the original MNE shell script:
 *   --noflash30, --noconvert, --unwarp
 *
 * Additional environment variables:
 *   FREESURFER_HOME, SUBJECTS_DIR, SUBJECT
 *
 * @brief Command-line settings for mne_flash_bem.
 */
class MneFlashBemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MneFlashBemSettings(int *argc, char **argv);

    //=========================================================================================================
    /**
     * Returns the subject name (from SUBJECT env var or --subject).
     *
     * @return Subject name string.
     */
    QString subject() const;

    //=========================================================================================================
    /**
     * Returns the subjects directory path (from SUBJECTS_DIR env var or --subjects-dir).
     *
     * @return Subjects directory path.
     */
    QString subjectsDir() const;

    //=========================================================================================================
    /**
     * Returns the FreeSurfer home directory path.
     *
     * @return FreeSurfer home path.
     */
    QString freeSurferHome() const;

    //=========================================================================================================
    /**
     * Returns whether DICOM-to-MGZ conversion should be skipped.
     *
     * @return True if --noconvert was specified.
     */
    bool noConvert() const;

    //=========================================================================================================
    /**
     * Returns whether 30-degree flash data is unavailable.
     *
     * @return True if --noflash30 was specified.
     */
    bool noFlash30() const;

    //=========================================================================================================
    /**
     * Returns the gradient unwarping option string.
     *
     * @return Unwarp option string, empty if not set.
     */
    QString unwarp() const;

    //=========================================================================================================
    /**
     * Returns the flash data directory (current working directory at invocation,
     * or set via --flash-dir).
     *
     * @return Flash data directory path.
     */
    QString flashDir() const;

private:
    QString     m_sSubject;         /**< Subject name. */
    QString     m_sSubjectsDir;     /**< Subjects directory path. */
    QString     m_sFreeSurferHome;  /**< FreeSurfer home path. */
    bool        m_bNoConvert;       /**< Skip DICOM conversion. */
    bool        m_bNoFlash30;       /**< No 30-degree flash data. */
    QString     m_sUnwarp;          /**< Gradient unwarp option. */
    QString     m_sFlashDir;        /**< Flash data directory. */
};

} // namespace MNEFLASHBEM

#endif // MNEFLASHBEMSETTINGS_H
