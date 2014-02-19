//=============================================================================================================
/**
* @file     filtertools.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FilterTools class
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtertools.h"
#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterTools::FilterTools()
{
}


//*************************************************************************************************************

void FilterTools::getStaticFilter(QString type, qint32 numberOfCoefficients, qint32 samplingRate, qint32 cutOffFreq, QVector<float> &impulseResponse)
{
    if(type == QString('HP'))
    {
        //HP from matlab
    }

    if(type == QString('LP'))
    {
        //LP from matlab
    }

    if(type == QString('BP'))
    {
        //BP from matlab
    }

    Q_UNUSED(numberOfCoefficients);
    Q_UNUSED(samplingRate);
    Q_UNUSED(cutOffFreq);
    Q_UNUSED(impulseResponse);
}

//*************************************************************************************************************

void FilterTools::createDynamicFilter(QString type, qint32 numberOfCoefficients, float normalizedCutOffFreq, QVector<float> &impulseResponse)
{
    //Create kaiser window
    QVector<float> window(numberOfCoefficients);
    KBDWindow(window, numberOfCoefficients, 8);

    //Calculate approximated sinc function (ideal TP in frequency domain)
    int t = 0;
    int nd = (numberOfCoefficients-1)/2;

    for(int i=0; i<numberOfCoefficients; i++)
    {
        double sinc = sin(normalizedCutOffFreq*M_PI*(t-nd)) / (M_PI*(t-nd));
        impulseResponse[i] = sinc*window[i];
        t++;
    }

    //Create final filter specified by the type parameter
    if(type == QString('HP'))
    {
        for(int i=1; i<impulseResponse.size(); i=i+2)
            impulseResponse[i] = impulseResponse[i] * (-1);
    }

    if(type == QString('LP'))
    {
        impulseResponse = impulseResponse;
    }

    if(type == QString('BP'))
    {
        t = 0;
        for(int i=0; i<numberOfCoefficients; i++)
        {
            double tempCos = cos(normalizedCutOffFreq*M_PI*(t-nd));
            impulseResponse[i] = impulseResponse[i]*tempCos;
            t++;
        }
    }
}


//*************************************************************************************************************

QVector<float> FilterTools::convolve(QVector<float> &in, QVector<float> &kernel)
{
    int i, j, k;
    QVector<float> out(in.size());

    // start convolution from out[kernelSize.size()-1] to out[in.size()-1] (last)
    for(i = kernel.size()-1; i < in.size(); ++i)
    {
        out[i] = 0;                             // init to 0 before accumulate

        for(j = i, k = 0; k < kernel.size(); --j, ++k)
            out[i] += in[j] * kernel[k];
    }

    // convolution from out[0] to out[kernelSize-2]
    for(i = 0; i < kernel.size() - 1; ++i)
    {
        out[i] = 0;                             // init to 0 before sum

        for(j = i, k = 0; j >= 0; --j, ++k)
            out[i] += in[j] * kernel[k];
    }

    return out;
}


//*************************************************************************************************************

void FilterTools::KBDWindow(QVector<float> &window, int size, float alpha)
{
    double sumvalue = 0.0;
    int i;

    for (i=0; i<size/2; i++)
    {
        sumvalue += BesselI0(M_PI * alpha * sqrt(1.0 - pow(4.0*i/size - 1.0, 2)));
        window[i] = sumvalue;
    }

    /* need to add one more value to the nomalization factor at size/2: */
    sumvalue += BesselI0(M_PI * alpha * sqrt(1.0 - pow(4.0*(size/2)/size-1.0, 2)));

    /* normalize the window and fill in the righthand side of the window: */
    for (i=0; i<size/2; i++)
    {
        window[i] = sqrt(window[i]/sumvalue);
        window[size-1-i] = window[i];
    }
}


//*************************************************************************************************************

float FilterTools::BesselI0(float x)
{
    float denominator;
    float numerator;
    float z;

    if (x == 0.0)
    {
        return 1.0;
    }
    else
    {
        z = x * x;
        numerator = (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z*
            (z* 0.210580722890567e-22  + 0.380715242345326e-19 ) +
            0.479440257548300e-16) + 0.435125971262668e-13 ) +
            0.300931127112960e-10) + 0.160224679395361e-7  ) +
            0.654858370096785e-5)  + 0.202591084143397e-2  ) +
            0.463076284721000e0)   + 0.754337328948189e2   ) +
            0.830792541809429e4)   + 0.571661130563785e6   ) +
            0.216415572361227e8)   + 0.356644482244025e9   ) +
            0.144048298227235e10);

        denominator = (z*(z*(z-0.307646912682801e4)+
            0.347626332405882e7)-0.144048298227235e10);
    }

    return -numerator/denominator;
}

