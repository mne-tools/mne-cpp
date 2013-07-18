//=============================================================================================================
/**
* @file     sourceestimate.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the SourceEstimate Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimate.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimate::SourceEstimate()
: tmin(0)
, tstep(-1)
{
}


//*************************************************************************************************************

SourceEstimate::SourceEstimate(const MatrixXd &p_sol, const QList<VectorXi> &p_vertices, float p_tmin, float p_tstep)
: data(p_sol)
, vertno(p_vertices)
, tmin(p_tmin)
, tstep(p_tstep)
{
    this->update_times();
}


//*************************************************************************************************************

SourceEstimate::SourceEstimate(const SourceEstimate& p_SourceEstimate)
: data(p_SourceEstimate.data)
, vertno(p_SourceEstimate.vertno)
, times(p_SourceEstimate.times)
, tmin(p_SourceEstimate.tmin)
, tstep(p_SourceEstimate.tstep)
{

}


//*************************************************************************************************************

void SourceEstimate::clear()
{
    data = MatrixXd();
    vertno.clear();
    times = RowVectorXf();
    tmin = 0;
    tstep = 0;
}


//*************************************************************************************************************

SourceEstimate SourceEstimate::reduce(qint32 start, qint32 n)
{
    SourceEstimate p_sourceEstimateReduced;

    qint32 rows = this->data.rows();

    p_sourceEstimateReduced.data = MatrixXd::Zero(rows,n);
    p_sourceEstimateReduced.data = this->data.block(0, start, rows, n);
    p_sourceEstimateReduced.vertno = this->vertno;
    p_sourceEstimateReduced.times = RowVectorXf::Zero(n);
    p_sourceEstimateReduced.times = this->times.block(start,0,1,n);
    p_sourceEstimateReduced.tmin = p_sourceEstimateReduced.times(0);
    p_sourceEstimateReduced.tstep = this->tstep;

    return p_sourceEstimateReduced;
}


//*************************************************************************************************************

void SourceEstimate::update_times()
{
    if(data.cols() > 0)
    {
        this->times = RowVectorXf(data.cols());
        this->times[0] = this->tmin;
        for(float i = 1; i < this->times.size(); ++i)
            this->times[i] = this->times[i-1] + this->tstep;
    }
    else
        this->times = RowVectorXf();
}
