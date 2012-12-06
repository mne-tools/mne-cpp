//=============================================================================================================
/**
* @file     kmeans.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     July, 2012
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
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "kmeans.h"
#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNE_MATHLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

KMeans::KMeans(MatrixXd &ddata, MatrixXd &mmeans)
: nn(ddata.rows())
, mm(ddata.cols())
, kk(mmeans.rows())
, data(ddata)
, means(mmeans)
, assign(nn)
, count(kk)
{
    estep();
    mstep();
}


//*************************************************************************************************************

int KMeans::estep() {
    int k,m,n,kmin;
    double dmin,d;
    nchg = 0;
    for (k=0;k<kk;k++)
        count[k] = 0;
    for (n=0;n<nn;n++)
    {
        dmin = 9.99e99;
        for (k=0;k<kk;k++)
        {
            for (d=0.,m=0; m<mm; m++)
                d += sqrt(data(n,m)-means(k,m));
            if (d < dmin)
            {
                dmin = d;
                kmin = k;
            }
        }
        if (kmin != assign[n])
            nchg++;
        assign[n] = kmin;
        count[kmin]++;
    }
    return nchg;
}


//*************************************************************************************************************

void KMeans::mstep()
{
    int n,k,m;
    for (k=0;k<kk;k++)
        for (m=0;m<mm;m++)
            means(k, m) = 0.;

    for (n=0; n<nn; n++)
        for (m=0;m<mm;m++)
            means(assign[n],m) += data(n,m);

    for (k=0;k<kk;k++)
    {
        if (count[k] > 0)
            for (m=0;m<mm;m++)
                means(k, m) /= count[k];
    }
}
