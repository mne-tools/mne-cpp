//=============================================================================================================
/**
 * @file     mne_msh_color_scale_def.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MneMshColorScaleDef class declaration.
 *
 */

#ifndef MNEMSHCOLORSCALEDEF_H
#define MNEMSHCOLORSCALEDEF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshPicked, mshPickedRec struct (analyze_types.c).
 *
 * @brief The MneMshColorScaleDef class.
 */
class MNESHARED_EXPORT MneMshColorScaleDef
{
public:
    typedef QSharedPointer<MneMshColorScaleDef> SPtr;              /**< Shared pointer type for MneMshColorScaleDef. */
    typedef QSharedPointer<const MneMshColorScaleDef> ConstSPtr;   /**< Const shared pointer type for MneMshColorScaleDef. */

    //=========================================================================================================
    /**
     * Constructs the MneMshColorScaleDef.
     */
    MneMshColorScaleDef();

    //=========================================================================================================
    /**
     * Destroys the MneMshColorScaleDef.
     */
    ~MneMshColorScaleDef();

public:
    int   type;                     /* What is this scale setting good for? */
    float mult;                     /* Convenience multiplier from internal units to displayed numbers */
    float fthresh;                  /* Threshold */
    float fmid;                     /* This is in the middle */
    float fslope;                   /* We still use the slope internally (sigh) */
    float tc_mult;                  /* Multiply the scales by this value for timecourses */
    int   relative;                 /* Are fthresh and fmid relative to the maximum value over the surface? */

// ### OLD STRUCT ###
//typedef struct {		                     /* The celebrated tksurfer-style values */
//  int   type;		                             /* What is this scale setting good for? */
//  float mult;			                     /* Convenience multiplier from internal units to displayed numbers */
//  float fthresh;				     /* Threshold */
//  float fmid;					     /* This is in the middle */
//  float fslope;					     /* We still use the slope internally (sigh) */
//  float tc_mult;			             /* Multiply the scales by this value for timecourses */
//  int   relative;		                     /* Are fthresh and fmid relative to the maximum value over the surface? */
//} *mshColorScaleDef,mshColorScaleDefRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHCOLORSCALEDEF_H
