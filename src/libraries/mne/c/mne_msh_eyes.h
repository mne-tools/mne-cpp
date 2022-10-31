//=============================================================================================================
/**
 * @file     mne_msh_eyes.h
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
 * @brief    MneMshEyes class declaration.
 *
 */

#ifndef MNEMSHEYES_H
#define MNEMSHEYES_H

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
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Replaces *mshEyes,mshEyesRec struct (analyze_types.c).
 *
 * @brief The MneMshEyes class.
 */
class MNESHARED_EXPORT MneMshEyes
{
public:
    typedef QSharedPointer<MneMshEyes> SPtr;              /**< Shared pointer type for MneMshEyes. */
    typedef QSharedPointer<const MneMshEyes> ConstSPtr;   /**< Const shared pointer type for MneMshEyes. */

    //=========================================================================================================
    /**
     * Constructs the MneMshEyes.
     */
    MneMshEyes();

    //=========================================================================================================
    /**
     * Destroys the MneMshEyes.
     */
    ~MneMshEyes();

public:
    char  *name;			/* Name of this definition */
    float left[3];		/* Left hemisphere viewpoint */
    float right[3];		/* Right hemisphere viewpoint */
    float left_up[3];		/* The up vectors */
    float right_up[3];		/* The up vectors */

// ### OLD STRUCT ###
//    typedef struct {		/* Where to look at the surfaces from */
//      char  *name;			/* Name of this definition */
//      float left[3];		/* Left hemisphere viewpoint */
//      float right[3];		/* Right hemisphere viewpoint */
//      float left_up[3];		/* The up vectors */
//      float right_up[3];		/* The up vectors */
//    } *mshEyes,mshEyesRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHEYES_H
