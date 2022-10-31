//=============================================================================================================
/**
 * @file     mne_corsourceestimate.cpp
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
 * @brief    Definition of the MNECorSourceEstimate Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_corsourceestimate.h"

#include <QFile>
#include <QDataStream>
#include <QSharedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNECorSourceEstimate::MNECorSourceEstimate()
: MNESourceEstimate()
{
}

//=============================================================================================================

MNECorSourceEstimate::MNECorSourceEstimate(const MatrixXd &p_sol, const VectorXi &p_vertices, float p_tmin, float p_tstep)
: MNESourceEstimate(p_sol, p_vertices, p_tmin, p_tstep)
{
}

//=============================================================================================================

MNECorSourceEstimate::MNECorSourceEstimate(const MNECorSourceEstimate& p_SourceEstimate)
: MNESourceEstimate(p_SourceEstimate)
{
}

//=============================================================================================================

MNECorSourceEstimate::MNECorSourceEstimate(QIODevice &p_IODevice)
: MNESourceEstimate(p_IODevice)
{
}

//=============================================================================================================

void MNECorSourceEstimate::clear()
{
    MNESourceEstimate::clear();
}

//=============================================================================================================

bool MNECorSourceEstimate::read(QIODevice &p_IODevice, MNECorSourceEstimate& p_stc)
{
    return MNESourceEstimate::read(p_IODevice, p_stc);
}

//=============================================================================================================

bool MNECorSourceEstimate::write(QIODevice &p_IODevice)
{
    return MNESourceEstimate::write(p_IODevice);
}

//=============================================================================================================

MNECorSourceEstimate& MNECorSourceEstimate::operator= (const MNECorSourceEstimate &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        MNESourceEstimate::operator= (rhs);
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}
