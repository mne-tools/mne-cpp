//=============================================================================================================
/**
 * @file     mne_watershed_bem_settings.h
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
 * @brief    MneWatershedBemSettings class declaration.
 *
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
class MneWatershedBemSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MneWatershedBemSettings(int *argc, char **argv);

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
