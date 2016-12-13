//=============================================================================================================
/**
* @file     dipolefitsettings.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "ecd_set.h"

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>



#define BIG_TIME 1e6


#ifndef MNEFILTERDEF
#define MNEFILTERDEF
typedef struct {
  bool  filter_on;          /* Is it on? */
  int   size;               /* Length in samples (must be a power of 2) */
  int   taper_size;         /* How long a taper in the beginning and end */
  float highpass;           /* Highpass in Hz */
  float highpass_width;     /* Highpass transition width in Hz */
  float lowpass;            /* Lowpass in Hz */
  float lowpass_width;      /* Lowpass transition width in Hz */
  float eog_highpass;       /* EOG highpass in Hz */
  float eog_highpass_width; /* EOG highpass transition width in Hz */
  float eog_lowpass;        /* EOG lowpass in Hz */
  float eog_lowpass_width;  /* EOG lowpass transition width in Hz */
} *mneFilterDef,mneFilterDefRec;
#endif


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//*************************************************************************************************************
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
    * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
    * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
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
    char  *bemname     = NULL;              /**< Boundary-element model */
    Eigen::Vector3f r0;                     /**< Sphere model origin  */
    bool   accurate    = false;             /**< Use accurate coil definitions? */
    char  *mriname     = NULL;              /**< Gives the MRI <-> head transform */

    char  *guessname   = NULL;               /* Initial guess grid (if not present, the values below
                                                     * will be employed to generate the grid) */
    char  *guess_surfname = NULL;            /* Load the inner skull surface from this BEM file */
    float guess_rad     = 0.080f;            /* Radius of spherical guess surface */
    float guess_mindist = 0.010f;            /* Minimum allowed distance to the surface */
    float guess_exclude = 0.020f;            /* Exclude points closer than this to the origin */
    float guess_grid    = 0.010f;            /* Grid spacing */

    char  *noisename   = NULL;               /* Noise-covariance matrix */
    float grad_std     = 5e-13f;             /* Standard deviations to be used if noise covariance is not specified */
    float mag_std      = 20e-15f;
    float eeg_std      = 0.2e-6f;
    bool  diagnoise    = false;              /* Use only the diagonals of the noise-covariance matrix */

    QString measname;                        /* Data file */
    bool  is_raw       = false;              /* Is this a raw data file */
    char  *badname     = NULL;               /* Bad channels */
    bool  include_meg  = false;              /* Use MEG? */
    bool  include_eeg  = false;              /* Use EEG? */
    float tmin         = -2*BIG_TIME;        /* Possibility to set these from the command line */
    float tmax         = 2*BIG_TIME;
    float tstep        = -1.0;               /* Step between fits */
    float integ        = 0.0;
    float bmin         = BIG_TIME;	        /* Possibility to set these from the command line */
    float bmax         = BIG_TIME;
    bool  do_baseline  = false;              /* Are both baseline limits set? */
    int   setno        = 1;                  /* Which data set */
    bool  verbose      = false;
    mneFilterDefRec     filter;
    char **projnames   = NULL;               /* Projection file names */
    int  nproj         = 0;
    bool omit_data_proj = false;

    char   *eeg_model_file = NULL;          /* File of EEG sphere model specifications */
    QString eeg_model_name;                 /* Name of the EEG model to use */
    float  eeg_sphere_rad = 0.09f;          /* Scalp radius to use in EEG sphere model */
    bool    scale_eeg_pos  = false;	        /* Scale the electrode locations to scalp in the sphere model */
    float  mag_reg      = 0.1f;             /* Noise-covariance matrix regularization for MEG (magnetometers and axial gradiometers)  */
    bool   fit_mag_dipoles = false;

    float  grad_reg     = 0.1f;              /* Noise-covariance matrix regularization for EEG (planar gradiometers) */
    float  eeg_reg      = 0.1f;              /* Noise-covariance matrix regularization for EEG  */
    QString dipname;              /* Output file in dip format */
    QString bdipname;              /* Output file in bdip format */

private:
    void initMembers();
    void usage(char *name);
    bool check_unrecognized_args(int argc, char **argv);
    bool check_args (int *argc,char **argv);

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // DIPOLEFITSETTINGS_H
