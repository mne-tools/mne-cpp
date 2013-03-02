//=============================================================================================================
/**
* @file     minimumnorm.cpp
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
* @brief    Implementation of the MinimumNorm Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "minimumnorm.h"
#include "../sourceestimate.h"

#include <fiff/fiff_evoked.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimumNorm::MinimumNorm(const MNEInverseOperator &p_inverseOperator, float lambda, const QString method)
: m_inverseOperator(p_inverseOperator)
{
    this->setRegularization(lambda);
    this->setMethod(method);
}

//*************************************************************************************************************

MinimumNorm::MinimumNorm(const MNEInverseOperator &p_inverseOperator, float lambda, bool dSPM, bool sLORETA)
: m_inverseOperator(p_inverseOperator)
{
    this->setRegularization(lambda);
    this->setMethod(dSPM, sLORETA);
}


//*************************************************************************************************************

bool MinimumNorm::calculateInverse(const FiffEvoked &p_fiffEvoked, SourceEstimate &p_SourceEstimate) const
{
    //ToDo put this in parameters
    VectorXi label;
    bool pick_normal = false;

    //
    //   Set up the inverse according to the parameters
    //
    qint32 nave = p_fiffEvoked.nave;

    if(!m_inverseOperator.check_ch_names(p_fiffEvoked.info))
    {
        qWarning("Channel name check failed.");
        return false;
    }

    MNEInverseOperator inv = m_inverseOperator.prepare_inverse_operator(nave, m_fLambda, m_bdSPM, m_bsLORETA);
    //
    //   Pick the correct channels from the data
    //
    FiffEvoked t_fiffEvoked = p_fiffEvoked.pick_channels(inv.noise_cov->names);

    printf("Picked %d channels from the data\n",t_fiffEvoked.info.nchan);
    printf("Computing inverse...");

    MatrixXd K;
    SparseMatrix<double> noise_norm;
    QList<VectorXi> vertno;

    inv.assemble_kernel(label, m_sMethod, pick_normal, K, noise_norm, vertno);
    MatrixXd sol = K * t_fiffEvoked.data; //apply imaging kernel

    if (inv.source_ori == FIFFV_MNE_FREE_ORI)
    {
        printf("combining the current components...");
        MatrixXd sol1(sol.rows()/3,sol.cols());
        for(qint32 i = 0; i < sol.cols(); ++i)
        {
            VectorXd* tmp = MNEMath::combine_xyz(sol.block(0,i,sol.rows(),1));
            sol1.block(0,i,sol.rows()/3,1) = tmp->cwiseSqrt();
            delete tmp;
        }
        sol.resize(sol1.rows(),sol1.cols());
        sol = sol1;
    }

    if (m_bdSPM)
    {
        printf("(dSPM)...");
        sol = inv.noisenorm*sol;
    }
    else if (m_bsLORETA)
    {
        printf("(sLORETA)...");
        sol = inv.noisenorm*sol;
    }
    printf("[done]\n");

    //Results
    float tmin = ((float)t_fiffEvoked.first) / t_fiffEvoked.info.sfreq;
    float tstep = 1/t_fiffEvoked.info.sfreq;

    QList<VectorXi> t_qListVertices;
    for(qint32 h = 0; h < inv.src.hemispheres.size(); ++h)
        t_qListVertices.push_back(inv.src.hemispheres[h].vertno);

    p_SourceEstimate = SourceEstimate(sol, t_qListVertices, tmin, tstep);

    return true;
}


//*************************************************************************************************************

void MinimumNorm::setMethod(QString method)
{
    if(method.compare("MNE") == 0)
        setMethod(false, false);
    else if(method.compare("dSPM") == 0)
        setMethod(true, false);
    else if(method.compare("sLORETA") == 0)
        setMethod(false, true);
    else
    {
        qWarning("Method not recognized!");
        method = "dSPM";
        setMethod(true, false);
    }

    printf("\tSet minimum norm method to %s.\n", method.toLatin1().constData());
}


//*************************************************************************************************************

void MinimumNorm::setMethod(bool dSPM, bool sLORETA)
{
    if(dSPM && sLORETA)
    {
        qWarning("Cant activate dSPM and sLORETA at the same time! - Activating dSPM");
        m_bdSPM = true;
        m_bsLORETA = false;
    }
    else
    {
        m_bdSPM = dSPM;
        m_bsLORETA = sLORETA;
        if(dSPM)
            m_sMethod = QString("dSPM");
        else if(sLORETA)
            m_sMethod = QString("sLORETA");
        else
            m_sMethod = QString("MNE");

    }
}


//*************************************************************************************************************

void MinimumNorm::setRegularization(float lambda)
{
    m_fLambda = lambda;
}
