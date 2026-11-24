//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_flash_bem_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     May 2006
 * @brief    MNEFlashBemSettings class declaration.
 */

#ifndef MNEFLASHBEMSETTINGS_H
#define MNEFLASHBEMSETTINGS_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE MNEFLASHBEM
//=============================================================================================================

namespace MNEFLASHBEM
{

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
class MNEFlashBemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MNEFlashBemSettings(int* argc, char** argv);

    //=========================================================================================================
    /**
     * Returns the subject name (from SUBJECT env var or --subject).
     *
     * @return Subject name string.
     */
    std::string subject() const;

    //=========================================================================================================
    /**
     * Returns the subjects directory path (from SUBJECTS_DIR env var or --subjects-dir).
     *
     * @return Subjects directory path.
     */
    std::string subjectsDir() const;

    //=========================================================================================================
    /**
     * Returns the FreeSurfer home directory path.
     *
     * @return FreeSurfer home path.
     */
    std::string freeSurferHome() const;

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
    std::string unwarp() const;

    //=========================================================================================================
    /**
     * Returns the flash data directory (current working directory at invocation,
     * or set via --flash-dir).
     *
     * @return Flash data directory path.
     */
    std::string flashDir() const;

private:
    std::string m_sSubject;        /**< Subject name. */
    std::string m_sSubjectsDir;    /**< Subjects directory path. */
    std::string m_sFreeSurferHome; /**< FreeSurfer home path. */
    bool m_bNoConvert;             /**< Skip DICOM conversion. */
    bool m_bNoFlash30;             /**< No 30-degree flash data. */
    std::string m_sUnwarp;         /**< Gradient unwarp option. */
    std::string m_sFlashDir;       /**< Flash data directory. */
};

} // namespace MNEFLASHBEM

#endif // MNEFLASHBEMSETTINGS_H
