//=============================================================================================================
/**
 * @file     mne_proj_data.h
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
 * @brief    MneProjData class declaration.
 *
 */

#ifndef MNEPROJDATA_H
#define MNEPROJDATA_H

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

class MneSurfaceOld;

//=============================================================================================================
/**
 * Replaces *projData,projDataRec struct (mne_project_to_surface.c).
 *
 * @brief The MneProjData class.
 */
class MNESHARED_EXPORT MneProjData
{
public:
    typedef QSharedPointer<MneProjData> SPtr;              /**< Shared pointer type for MneProjData. */
    typedef QSharedPointer<const MneProjData> ConstSPtr;   /**< Const shared pointer type for MneProjData. */

    //=========================================================================================================
    /**
     * Constructs the MneProjData.
     */
    MneProjData(MNELIB::MneSurfaceOld* s);

    //=========================================================================================================
    /**
     * Destroys the MneProjData.
     */
    ~MneProjData();

public:
    float *a;
    float *b;
    float *c;
    int   *act;
    int   nactive;

// ### OLD STRUCT ###
//    typedef struct {
//        float *a;
//        float *b;
//        float *c;
//        int   *act;
//        int   nactive;
//    } *projData,projDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEPROJDATA_H
