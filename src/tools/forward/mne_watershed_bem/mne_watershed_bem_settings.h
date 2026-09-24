//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_watershed_bem_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNEWatershedBemSettings class declaration.
 */

#ifndef MNEWATERSHEDBEMSETTINGS_H
#define MNEWATERSHEDBEMSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNEWATERSHEDBEM
//=============================================================================================================

namespace MNEWATERSHEDBEM {

//=============================================================================================================
/**
 * Parses and stores command-line settings for mne_watershed_bem.
 *
 * Supports the same options as the original MNE shell script:
 *   --subject, --volume, --overwrite, --atlas, --gcaatlas, --preflood
 *
 * @brief Command-line settings for mne_watershed_bem.
 */
class MNEWatershedBemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MNEWatershedBemSettings(int *argc, char **argv);

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
     * Returns the FreeSurfer home directory path.
     *
     * @return FreeSurfer home path.
     */
    QString freeSurferHome() const;

    //=========================================================================================================
    /**
     * Returns the MRI volume name to use.
     *
     * @return Volume name (default: "T1").
     */
    QString volume() const;

    //=========================================================================================================
    /**
     * Returns whether to overwrite existing data.
     *
     * @return True if overwrite mode is enabled.
     */
    bool overwrite() const;

    //=========================================================================================================
    /**
     * Returns whether the --atlas option should be passed to mri_watershed.
     *
     * @return True if atlas mode is enabled.
     */
    bool atlas() const;

    //=========================================================================================================
    /**
     * Returns whether to use the subcortical atlas (--gcaatlas).
     *
     * @return True if GCA atlas mode is enabled.
     */
    bool gcaAtlas() const;

    //=========================================================================================================
    /**
     * Returns the preflood height parameter for mri_watershed.
     *
     * @return Preflood height, or -1 if not set.
     */
    int preflood() const;

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
    QString     m_sFreeSurferHome;  /**< FreeSurfer home path. */
    QString     m_sVolume;          /**< MRI volume name. */
    bool        m_bOverwrite;       /**< Whether to overwrite existing data. */
    bool        m_bAtlas;           /**< Use --atlas for mri_watershed. */
    bool        m_bGcaAtlas;        /**< Use subcortical atlas. */
    int         m_iPreflood;        /**< Preflood height (-1 = not set). */
    bool        m_bVerbose;         /**< Verbose output. */
};

} // namespace MNEWATERSHEDBEM

#endif // MNEWATERSHEDBEMSETTINGS_H
