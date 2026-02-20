//=============================================================================================================
/**
 * @file     mne_setup_forward_model_settings.h
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
 * @brief    MneSetupForwardModelSettings class declaration.
 *
 *           Parses command-line settings for mne_setup_forward_model.
 *
 *           Supports the same options as the original MNE shell script:
 *             --subject, --scalpc, --skullc, --brainc, --model, --homog,
 *             --surf, --ico, --nosol, --noswap, --meters, --innershift,
 *             --outershift, --scalpshift, --overwrite
 *
 */

#ifndef MNESETUPFORWARDMODELSETTINGS_H
#define MNESETUPFORWARDMODELSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNESETUPFORWARDMODEL
//=============================================================================================================

namespace MNESETUPFORWARDMODEL {

//=============================================================================================================
/**
 * Parses and stores command-line settings for mne_setup_forward_model.
 *
 * @brief Command-line settings for mne_setup_forward_model.
 */
class MneSetupForwardModelSettings
{
public:
    //=========================================================================================================
    /**
     * Constructs settings from command-line arguments.
     *
     * @param[in] argc  Number of arguments.
     * @param[in] argv  Argument array.
     */
    MneSetupForwardModelSettings(int *argc, char **argv);

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
     * Returns the scalp conductivity in S/m.
     *
     * @return Scalp conductivity (default 0.3 S/m).
     */
    float scalpConductivity() const;

    //=========================================================================================================
    /**
     * Returns the skull conductivity in S/m.
     *
     * @return Skull conductivity (default 0.006 S/m).
     */
    float skullConductivity() const;

    //=========================================================================================================
    /**
     * Returns the brain conductivity in S/m.
     *
     * @return Brain conductivity (default 0.3 S/m).
     */
    float brainConductivity() const;

    //=========================================================================================================
    /**
     * Returns the BEM model name (without path and -bem.fif suffix).
     *
     * @return Model name string, empty for auto-generated name.
     */
    QString modelName() const;

    //=========================================================================================================
    /**
     * Returns whether to create a single-compartment (homogeneous) model.
     *
     * @return True if --homog was specified.
     */
    bool homogeneous() const;

    //=========================================================================================================
    /**
     * Returns whether to use FreeSurfer binary surface files (.surf)
     * instead of ASCII triangle files (.tri).
     *
     * @return True if --surf was specified.
     */
    bool useSurfFormat() const;

    //=========================================================================================================
    /**
     * Returns the icosahedron subdivision level for downsampling.
     *
     * @return ICO level (0-6), or -1 if not specified.
     */
    int icoLevel() const;

    //=========================================================================================================
    /**
     * Returns whether to skip the BEM solution preparation step.
     *
     * @return True if --nosol was specified.
     */
    bool noSolution() const;

    //=========================================================================================================
    /**
     * Returns whether to swap triangle vertex winding order.
     * Only effective for ASCII .tri files.
     *
     * @return True if winding should be swapped (default true).
     */
    bool swap() const;

    //=========================================================================================================
    /**
     * Returns whether triangle coordinates are in meters.
     * Only effective for ASCII .tri files (default: millimeters).
     *
     * @return True if --meters was specified.
     */
    bool meters() const;

    //=========================================================================================================
    /**
     * Returns the inner skull surface shift in mm.
     *
     * @return Shift value in mm (default 0.0).
     */
    float innerShift() const;

    //=========================================================================================================
    /**
     * Returns the outer skull surface shift in mm.
     *
     * @return Shift value in mm (default 0.0).
     */
    float outerShift() const;

    //=========================================================================================================
    /**
     * Returns the scalp surface shift in mm.
     *
     * @return Shift value in mm (default 0.0).
     */
    float scalpShift() const;

    //=========================================================================================================
    /**
     * Returns whether to overwrite existing files.
     *
     * @return True if --overwrite was specified.
     */
    bool overwrite() const;

private:
    QString m_sSubject;             /**< Subject name. */
    QString m_sSubjectsDir;         /**< Subjects directory path. */
    float   m_fScalpConductivity;   /**< Scalp conductivity [S/m]. */
    float   m_fSkullConductivity;   /**< Skull conductivity [S/m]. */
    float   m_fBrainConductivity;   /**< Brain conductivity [S/m]. */
    QString m_sModelName;           /**< BEM model name (empty = auto). */
    bool    m_bHomogeneous;         /**< Single-compartment model. */
    bool    m_bUseSurfFormat;       /**< Use .surf instead of .tri. */
    int     m_iIcoLevel;            /**< ICO subdivision level, -1 = not used. */
    bool    m_bNoSolution;          /**< Skip solution preparation. */
    bool    m_bSwap;                /**< Swap triangle winding order. */
    bool    m_bMeters;              /**< Coordinates in meters. */
    float   m_fInnerShift;          /**< Inner skull shift [mm]. */
    float   m_fOuterShift;          /**< Outer skull shift [mm]. */
    float   m_fScalpShift;          /**< Scalp shift [mm]. */
    bool    m_bOverwrite;           /**< Overwrite existing files. */
};

} // namespace MNESETUPFORWARDMODEL

#endif // MNESETUPFORWARDMODELSETTINGS_H
