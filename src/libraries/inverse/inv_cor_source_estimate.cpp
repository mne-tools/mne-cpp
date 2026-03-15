//=============================================================================================================
/**
 * @file     inv_cor_source_estimate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the InvCorSourceEstimate Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_cor_source_estimate.h"

#include <QFile>
#include <QDataStream>
#include <QSharedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvCorSourceEstimate::InvCorSourceEstimate()
: InvSourceEstimate()
{
}

//=============================================================================================================

InvCorSourceEstimate::InvCorSourceEstimate(const MatrixXd &p_sol, const VectorXi &p_vertices, float p_tmin, float p_tstep)
: InvSourceEstimate(p_sol, p_vertices, p_tmin, p_tstep)
{
}

//=============================================================================================================

InvCorSourceEstimate::InvCorSourceEstimate(const InvCorSourceEstimate& p_SourceEstimate)
: InvSourceEstimate(p_SourceEstimate)
{
}

//=============================================================================================================

InvCorSourceEstimate::InvCorSourceEstimate(QIODevice &p_IODevice)
: InvSourceEstimate(p_IODevice)
{
}

//=============================================================================================================

void InvCorSourceEstimate::clear()
{
    InvSourceEstimate::clear();
}

//=============================================================================================================

bool InvCorSourceEstimate::read(QIODevice &p_IODevice, InvCorSourceEstimate& p_stc)
{
    return InvSourceEstimate::read(p_IODevice, p_stc);
}

//=============================================================================================================

bool InvCorSourceEstimate::write(QIODevice &p_IODevice)
{
    return InvSourceEstimate::write(p_IODevice);
}

//=============================================================================================================

InvCorSourceEstimate& InvCorSourceEstimate::operator= (const InvCorSourceEstimate &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        InvSourceEstimate::operator= (rhs);
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}
