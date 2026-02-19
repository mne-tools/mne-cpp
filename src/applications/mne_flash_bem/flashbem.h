//=============================================================================================================
/**
 * @file     flashbem.h
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
 * @brief    FlashBem class declaration.
 *
 *           Ported from the original MNE shell script mne_flash_bem
 *           by Matti Hamalainen (SVN $Id: mne_flash_bem 3255).
 *
 *           Creates BEM meshes using multi-echo FLASH MRI sequences.
 *           Orchestrates FreeSurfer tools (mri_convert, mri_ms_fitparms,
 *           mri_synthesize, mri_average, fsl_rigid_register,
 *           mri_make_bem_surfaces) and then uses mne-cpp libraries to
 *           convert the resulting triangle files into FreeSurfer surface
 *           format with proper coordinate transforms.
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_flash_bem().
 *
 */

#ifndef FLASHBEM_H
#define FLASHBEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNEFLASHBEM
//=============================================================================================================

namespace MNEFLASHBEM {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneFlashBemSettings;

//=============================================================================================================
/**
 * Creates BEM meshes from multi-echo FLASH MRI data.
 *
 * This class implements the full workflow of the original mne_flash_bem
 * shell script:
 *   1. Convert DICOM images to MGZ format (mri_convert)
 *   2. Optional gradient unwarping (grad_unwarp)
 *   3. Create T1/PD parameter maps (mri_ms_fitparms) or average echoes
 *   4. Synthesize flash-5 volume (mri_synthesize) or average flash-5 echoes
 *   5. Register flash-5 with MPRAGE T1 (fsl_rigid_register)
 *   6. Convert flash5 to COR format (mri_convert)
 *   7. Create BEM surfaces (mri_make_bem_surfaces)
 *   8. Convert tri files to surf files with coordinate transform
 *
 * @brief Creates BEM meshes from FLASH MRI data.
 */
class FlashBem
{
public:
    //=========================================================================================================
    /**
     * Constructs the FlashBem processor.
     *
     * @param[in] settings  The parsed command-line settings.
     */
    FlashBem(const MneFlashBemSettings& settings);

    //=========================================================================================================
    /**
     * Runs the entire flash BEM creation pipeline.
     *
     * @return 0 on success, 1 on failure.
     */
    int run();

private:
    //=========================================================================================================
    /**
     * Runs a FreeSurfer command as a subprocess with forwarded output.
     *
     * @param[in] program    The program to execute.
     * @param[in] args       Command-line arguments.
     * @param[in] workDir    Working directory for the process (empty = inherit).
     *
     * @return true on success (exit code 0), false on failure.
     */
    bool runCommand(const QString& program, const QStringList& args,
                    const QString& workDir = QString());

    //=========================================================================================================
    /**
     * Step 1: Converts DICOM images to MGZ format.
     *
     * Scans flash05/ (and flash30/ if available) for echo directories,
     * picks the first file from each, and converts via mri_convert.
     *
     * @param[in]  flashDir   Flash data directory containing flash05/, flash30/.
     * @param[in]  mriFlashDir Target directory for MGZ output.
     * @param[out] echosConverted Number of echoes actually converted.
     *
     * @return true on success.
     */
    bool convertImages(const QString& flashDir, const QString& mriFlashDir,
                       int& echosConverted);

    //=========================================================================================================
    /**
     * Optional: Applies gradient unwarping to all MGZ echo files.
     *
     * @param[in] mriFlashDir Directory containing the mef*.mgz files.
     *
     * @return true on success.
     */
    bool unwarpImages(const QString& mriFlashDir);

    //=========================================================================================================
    /**
     * Step 2: Creates T1/PD parameter maps from flash data.
     *
     * @param[in] mriFlashDir  Directory containing the echo MGZ files.
     * @param[in] paramDir     Output directory for parameter maps.
     *
     * @return true on success.
     */
    bool createParameterMaps(const QString& mriFlashDir, const QString& paramDir);

    //=========================================================================================================
    /**
     * Step 3: Synthesizes or averages the flash-5 volume.
     *
     * With flash30: synthesizes from T1.mgz and PD.mgz.
     * Without flash30: averages all flash-5 echo volumes.
     *
     * @param[in] mriFlashDir  Directory containing mef05 echo files.
     * @param[in] paramDir     Parameter maps directory.
     *
     * @return true on success.
     */
    bool createFlash5Volume(const QString& mriFlashDir, const QString& paramDir);

    //=========================================================================================================
    /**
     * Step 4: Registers flash5.mgz with the MPRAGE T1 volume.
     *
     * @param[in] paramDir  Parameter maps directory (contains flash5.mgz).
     * @param[in] mriDir    Subject's mri directory (contains T1.mgz or T1/).
     *
     * @return true on success.
     */
    bool registerWithMprage(const QString& paramDir, const QString& mriDir);

    //=========================================================================================================
    /**
     * Step 5: Converts volumes to COR format.
     *
     * 5a: flash5_reg.mgz → COR in flash5/
     * 5b: T1.mgz → COR in T1/ (if needed)
     * 5c: brain.mgz → COR in brain/ (if needed)
     *
     * @param[in] paramDir  Parameter maps directory.
     * @param[in] mriDir    Subject's mri directory.
     * @param[out] convertedT1    Whether T1 COR was created (for cleanup).
     * @param[out] convertedBrain Whether brain COR was created (for cleanup).
     *
     * @return true on success.
     */
    bool convertToCor(const QString& paramDir, const QString& mriDir,
                      bool& convertedT1, bool& convertedBrain);

    //=========================================================================================================
    /**
     * Step 6: Creates BEM triangulation surfaces.
     *
     * @return true on success.
     */
    bool createBemSurfaces();

    //=========================================================================================================
    /**
     * Step 7: Converts .tri files to .surf files with coordinate transform.
     *
     * Reads each .tri file as an ASCII triangle surface, applies the
     * coordinate transform from the flash5_reg.mgz volume, and writes
     * a FreeSurfer binary surface file.
     *
     * @param[in] bemDir    Subject's bem directory.
     * @param[in] paramDir  Parameter maps directory (for flash5_reg.mgz).
     *
     * @return true on success.
     */
    bool convertTriToSurf(const QString& bemDir, const QString& paramDir);

    //=========================================================================================================
    /**
     * Final step: Cleanup temporary files.
     *
     * @param[in] bemDir         Subject's bem directory.
     * @param[in] mriDir         Subject's mri directory.
     * @param[in] convertedT1    Whether T1 COR was created by us.
     * @param[in] convertedBrain Whether brain COR was created by us.
     */
    void cleanup(const QString& bemDir, const QString& mriDir,
                 bool convertedT1, bool convertedBrain);

    //=========================================================================================================

    const MneFlashBemSettings& m_settings;   /**< Command-line settings. */
    int m_step;                              /**< Current processing step number. */
};

} // namespace MNEFLASHBEM

#endif // FLASHBEM_H
