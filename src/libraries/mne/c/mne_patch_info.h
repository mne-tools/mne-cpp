//=============================================================================================================
/**
 * @file     mne_patch_info.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Patch Information (MnePatchInfo) class declaration.
 *
 */

#ifndef MNEPATCHINFO_H
#define MNEPATCHINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

class MneSourceSpaceOld;

//=============================================================================================================
/**
 * Implements an MNE Patch Information (Replaces *mnePatchInfo,mnePatchInfoRec; struct of MNE-C mne_types.h).
 *
 * @brief One item in a derivation data set
 */
class MNESHARED_EXPORT MnePatchInfo
{
public:
    typedef QSharedPointer<MnePatchInfo> SPtr;              /**< Shared pointer type for MnePatchInfo. */
    typedef QSharedPointer<const MnePatchInfo> ConstSPtr;   /**< Const shared pointer type for MnePatchInfo. */

    //=========================================================================================================
    /**
     * Constructs the MNE Patch Information
     * Refactored: mne_new_patch (mne_source_space.c)
     */
    MnePatchInfo();

    //=========================================================================================================
    /**
     * Destroys the MNE Patch Information
     * Refactored: mne_free_patch (mne_source_space.c)
     */
    ~MnePatchInfo();

    //=========================================================================================================
    /**
     * Refactored: calculate_patch_area (mne_patches.c)
     */
    static void calculate_patch_area(MneSourceSpaceOld* s, MnePatchInfo* p);

    //=========================================================================================================
    /**
     * Refactored: calculate_normal_stats (mne_patches.c)
     */
    static void calculate_normal_stats(MneSourceSpaceOld* s, MnePatchInfo* p);

public:
    int   vert;         /* Which vertex does this apply to */
    int   *memb_vert;   /* Which vertices constitute the patch? */
    int   nmemb;        /* How many? */
    float area;         /* Area of the patch */
    float ave_nn[3];    /* Average normal */
    float dev_nn;       /* Average deviation of the patch normals from the average normal */

// ### OLD STRUCT ###
//typedef struct {
//    int   vert;             /* Which vertex does this apply to */
//    int   *memb_vert;       /* Which vertices constitute the patch? */
//    int   nmemb;            /* How many? */
//    float area;             /* Area of the patch */
//    float ave_nn[3];        /* Average normal */
//    float dev_nn;           /* Average deviation of the patch normals from the average normal */
//} *mnePatchInfo,mnePatchInfoRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEPATCHINFO_H
