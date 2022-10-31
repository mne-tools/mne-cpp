//=============================================================================================================
/**
 * @file     fwd_bem_solution.h
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
 * @brief    Forward BEM Solution (FwdBemSolution) class declaration.
 *
 */

#ifndef FWDBEMSOLUTION_H
#define FWDBEMSOLUTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
/**
 * Implements a Forward BEM Solution (Replaces *fwdBemSolution,fwdBemSolutionRec; struct of MNE-C fwd_types.h).
 *
 * @brief Mapping from infinite medium potentials to a particular set of coils or electrodes
 */
class FWDSHARED_EXPORT FwdBemSolution
{
public:
    typedef QSharedPointer<FwdBemSolution> SPtr;              /**< Shared pointer type for FwdBemSolution. */
    typedef QSharedPointer<const FwdBemSolution> ConstSPtr;   /**< Const shared pointer type for FwdBemSolution. */

    //=========================================================================================================
    /**
     * Constructs the Forward BEM Solution
     * Refactored: fwd_bem_new_coil_solution (fwd_bem_model.c)
     *
     */
    FwdBemSolution();

    //=========================================================================================================
    /**
     * Destroys the Forward BEM Solution
     * Refactored: fwd_bem_free_coil_solution (fwd_bem_model.c)
     */
    ~FwdBemSolution();

    //============================= fwd_bem_model.c =============================
    //TODO Remove later on use delete instead
    static void fwd_bem_free_coil_solution(void *user);

public:
    float **solution;                   /* The solution matrix */
    int   ncoil;                        /* Number of sensors */
    int   np;                           /* Number of potential solution points */

// ### OLD STRUCT ###
//typedef struct {                        /* Space to store a solution matrix */
//    float **solution;                   /* The solution matrix */
//    int   ncoil;                        /* Number of sensors */
//    int   np;                           /* Number of potential solution points */
//} *fwdBemSolution,fwdBemSolutionRec;    /* Mapping from infinite medium potentials to a particular set of coils or electrodes */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWDBEMSOLUTION_H
