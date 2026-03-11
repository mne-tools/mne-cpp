//=============================================================================================================
/**
 * @file     ecd.cpp
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
 * @brief    Definition of the Electric Current Dipole (ECD) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecd.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECD::ECD()
: valid(false)
, time(-1)
, rd(Vector3f::Zero(3))
, Q(Vector3f::Zero(3))
, good(0)
, khi2(0)
, nfree(0)
, neval(-1)
{
}

//=============================================================================================================

ECD::ECD(const ECD& p_ECD)
: valid(p_ECD.valid)
, time(p_ECD.time)
, rd(p_ECD.rd)
, Q(p_ECD.Q)
, good(p_ECD.good)
, khi2(p_ECD.khi2)
, nfree(p_ECD.nfree)
, neval(p_ECD.neval)
{
}

//=============================================================================================================

ECD::~ECD()
{
}

//=============================================================================================================

void ECD::print(FILE *f) const
{
    if (!f || !this->valid)
        return;

    fprintf(f,"%6.1f %7.2f %7.2f %7.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %d\n",
            1000*this->time,                              /* Time */
            1000*this->rd[0],                             /* Dipole location */
            1000*this->rd[1],
            1000*this->rd[2],
            1e9*this->Q.norm(),                           /* Dipole moment */
            1e9*this->Q[0],1e9*this->Q[1],1e9*this->Q[2],
            this->khi2/this->nfree,                       /* This is the reduced khi^2 value */
            100*this->good,                               /* Goodness of fit */
            this->neval);                                 /* Number of function evaluations required */
}
