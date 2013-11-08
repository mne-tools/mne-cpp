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
#include <QFile>
#include <QDataStream>

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

void FilterTools::createFilter(QString type, qint32 numberOfCoefficients, double normalizedCutOffFreq, QVector<double> &impulseResponse)
{
    //Create kaiser window
    QVector<double> window(numberOfCoefficients);
    KBDWindow(window, numberOfCoefficients, 8);

    impulseResponse = window;

    //Calculate approximated sinc function (ideal TP in frequency domain)
    QVector<double> sincApprox;
    int t = 0;
    int nd = (numberOfCoefficients-1)/2;

    for(int i=0; i<numberOfCoefficients; i++)
    {
        double sinc = sin(normalizedCutOffFreq*M_PI*(t-nd)) / (M_PI*(t-nd));
        sincApprox.push_back(sinc*window[i]);
        t++;
    }

    //Create final filter specified by the type parameter
    if(type == QString('HP'))
    {

    }

    if(type == QString('LP'))
    {

    }

    if(type == QString('BP'))
    {

    }

}


//*************************************************************************************************************

void FilterTools::KBDWindow(QVector<double> &window, int size, double alpha)
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

double FilterTools::BesselI0(double x)
{
    double denominator;
    double numerator;
    double z;

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

