//=============================================================================================================
/**
* @file     colormap.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the ColorMap class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "colormap.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ColorMap::ColorMap()
{
}


//*************************************************************************************************************

ColorMap::~ColorMap()
{
}


//*************************************************************************************************************
//HSV Colormap
int ColorMap::hsvR(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return 0;
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(hsvSlopeMRaising(x, -1.5)*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(1.0*255);
    else if(x >= 0.875)
        return (int)floor(hsvSlopeMFalling(x, 4.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

int ColorMap::hsvG(double x)
{
    //Describe the green fuzzy set
    if(x < 0.125)
        return 0;
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(hsvSlopeMRaising(x, -0.5)*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(1.0*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(hsvSlopeMFalling(x, 2.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

int ColorMap::hsvB(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.125)
        return (int)floor(hsvSlopeMRaising(x, 0.5)*255);
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(1.0*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(hsvSlopeMFalling(x, 2.5)*255);
    else
        return 0;
}


//*************************************************************************************************************

double ColorMap::hsvSlopeMRaising(double x, double n)
{
    //f = m*x + n
    return 4*x + n;
}


//*************************************************************************************************************

double ColorMap::hsvSlopeMFalling(double x,double n)
{
    //f = m*x + n
    return -4*x + n;
}
