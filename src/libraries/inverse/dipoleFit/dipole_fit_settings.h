//=============================================================================================================
/**
 * @file     dipole_fit_settings.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Dipole Fit Setting class declaration.
 *
 */

#ifndef DIPOLEFITSETTINGS_H
#define DIPOLEFITSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "ecd_set.h"

#include <mne/c/mne_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>

#define BIG_TIME 1e6

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements the dipole fit setting parser
 *
 * @brief Dipole Fit setting implementation
 */
class INVERSESHARED_EXPORT DipoleFitSettings
{
public:
    typedef QSharedPointer<DipoleFitSettings> SPtr;             /**< Shared pointer type for DipoleFitSettings. */
    typedef QSharedPointer<const DipoleFitSettings> ConstSPtr;  /**< Const shared pointer type for DipoleFitSettings. */

    //=========================================================================================================
    /**
     * Default Constructor
     */
    explicit DipoleFitSettings();

    //=========================================================================================================
    /**
     * Constructs Dipole Fit Settings
     *
     * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
     * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
     */
    explicit DipoleFitSettings(int *argc,char **argv);

    //=========================================================================================================
    /**
     * Destructs the Dipole Fit Settings
     */
    virtual ~DipoleFitSettings();

    //=========================================================================================================
    /**
     * Check whether Dipole Fit Settings are correctly set.
     */
    void checkIntegrity();

public:
    QString bemname;                    /**< Boundary-element model. */
    Eigen::Vector3f r0;                 /**< Sphere model origin . */
    bool   accurate;         		/**< Use accurate coil definitions?. */
    QString mriname;                    /**< Gives the MRI <-> head transform. */

    QString guessname;                  /**< Initial guess grid (if not present, the values below will be employed to generate the grid). */
    QString guess_surfname;             /**< Load the inner skull surface from this BEM file. */
float guess_rad;       			/**< Radius of spherical guess surface. */
    float guess_mindist;       		/**< Minimum allowed distance to the surface. */
    float guess_exclude;       		/**< Exclude points closer than this to the origin. */
    float guess_grid;       		/**< Grid spacing. */

    QString noisename;                  /**< Noise-covariance matrix. */
    float grad_std;        		/**< Standard deviations to be used if noise covariance is not specified. */
    float mag_std;
    float eeg_std;
    bool  diagnoise;         		/**< Use only the diagonals of the noise-covariance matrix. */

    QString measname;                   /**< Data file. */
    bool  is_raw;         		/**< Is this a raw data file. */
    char  *badname;          		/**< Bad channels. */
    bool  include_meg;         		/**< Use MEG?. */
    bool  include_eeg;         		/**< Use EEG?. */
    float tmin;   			/**< Possibility to set these from the command line. */
    float tmax;
    float tstep;          		/**< Step between fits. */
    float integ;
    float bmin;      			/**< Possibility to set these from the command line. */
    float bmax;
    bool  do_baseline;         		/**< Are both baseline limits set?. */
    int   setno;             		/**< Which data set. */
    bool  verbose;
    MNELIB::mneFilterDefRec filter;
    QStringList projnames;              /**< Projection file names. */
    bool omit_data_proj;

    QString eeg_model_file;             /**< File of EEG sphere model specifications. */
    QString eeg_model_name;             /**< Name of the EEG model to use. */
    float  eeg_sphere_rad;      		/**< Scalp radius to use in EEG sphere model. */
    bool    scale_eeg_pos;     		/**< Scale the electrode locations to scalp in the sphere model. */
    float  mag_reg;         		/**< Noise-covariance matrix regularization for MEG (magnetometers and axial gradiometers) . */
    bool   fit_mag_dipoles;

    float  grad_reg;         		/**< Noise-covariance matrix regularization for EEG (planar gradiometers). */
    float  eeg_reg;         		/**< Noise-covariance matrix regularization for EEG . */
    QString dipname;                    /**< Output file in dip format. */
    QString bdipname;                   /**< Output file in bdip format. */

    bool gui;                		/**< Should the gui been shown?. */

private:
    void initMembers();
    void usage(char *name);
    bool check_unrecognized_args(int argc, char **argv);
    bool check_args (int *argc,char **argv);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // DIPOLEFITSETTINGS_H
