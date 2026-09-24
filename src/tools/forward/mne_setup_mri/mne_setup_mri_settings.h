//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_setup_mri_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNESetupMriSettings class declaration.
 */

#ifndef MNESETUPMRISETTINGS_H
#define MNESETUPMRISETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNESETUPMRI
//=============================================================================================================

namespace MNESETUPMRI {

//=============================================================================================================
/**
 * Parses and stores command-line settings for mne_setup_mri.
 *
 * @brief Command-line settings for mne_setup_mri.
 */
class MNESetupMriSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MNESetupMriSettings(int *argc, char **argv);

    //=========================================================================================================
    /**
     * Returns the subject name.
     *
     * @return Subject name string.
     */
    QString subject() const;

    //=========================================================================================================
    /**
     * Returns the subjects directory path.
     *
     * @return Subjects directory path.
     */
    QString subjectsDir() const;

    //=========================================================================================================
    /**
     * Returns the list of MRI set names to process.
     *
     * @return List of MRI set names.
     */
    QStringList mriSets() const;

    //=========================================================================================================
    /**
     * Returns whether to overwrite existing data.
     *
     * @return True if overwrite mode is enabled.
     */
    bool overwrite() const;

    //=========================================================================================================
    /**
     * Returns whether verbose output is enabled.
     *
     * @return True if verbose mode is enabled.
     */
    bool verbose() const;

private:
    QString     m_sSubject;         /**< Subject name. */
    QString     m_sSubjectsDir;     /**< Subjects directory path. */
    QStringList m_slMriSets;        /**< MRI set names to process. */
    bool        m_bOverwrite;       /**< Whether to overwrite existing data. */
    bool        m_bVerbose;         /**< Verbose output. */
};

} // namespace MNESETUPMRI

#endif // MNESETUPMRISETTINGS_H
