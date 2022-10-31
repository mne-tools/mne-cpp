//=============================================================================================================
/**
 * @file     colormap.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the ColorMap class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "colormap.h"
#include <math.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ColorMap::ColorMap()
{    
}

//=============================================================================================================

ColorMap::~ColorMap()
{
}

//=============================================================================================================

double ColorMap::linearSlope(double x, double m, double n)
{
    //f = m*x + n
    return m*x + n;
}

//=============================================================================================================

int ColorMap::jetR(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return 0;
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(linearSlope(x, 4, -1.5)*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(1.0*255);
    else if(x >= 0.875)
        return (int)floor(linearSlope(x, -4, 4.5)*255);
    else
        return 0;
}

//=============================================================================================================

int ColorMap::jetG(double x)
{
    //Describe the green fuzzy set
    if(x < 0.125)
        return 0;
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(linearSlope(x, 4, -0.5)*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(1.0*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(linearSlope(x, -4, 3.5)*255);
    else
        return 0;
}

//=============================================================================================================

int ColorMap::jetB(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.125)
        return (int)floor(linearSlope(x, 4, 0.5)*255);
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(1.0*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(linearSlope(x, -4, 2.5)*255);
    else
        return 0;
}

//=============================================================================================================

int ColorMap::hotR(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return (int)floor(linearSlope(x, 2.5621, 0.0392)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotG(double x)
{
    //Describe the green fuzzy set
    if(x < 0.375)
        return 0;
    else if(x >= 0.375 && x < 0.75)
        return (int)floor(linearSlope(x, 2.6667, -1.0)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotB(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.75)
        return 0;
    else
        return (int)floor(linearSlope(x,4,-3)*255);
}

//=============================================================================================================

int ColorMap::hotRNeg1(double x)
{
    //Describe the red fuzzy set
    if(x < 0.2188)
        return 0;
    else if(x < 0.5781)
        return (int)floor(linearSlope(x, 2.7832, -0.6090)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotGNeg1(double x)
{
    //Describe the green fuzzy set
    if(x < 0.5781)
        return 0;
    else if(x >= 0.5781 && x < 0.8125)
        return (int)floor(linearSlope(x, 4.2662, -2.4663)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotBNeg1(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.8125)
        return 0;
    else
        return (int)floor(linearSlope(x,5.3333,-4.3333)*255);
}

//=============================================================================================================

int ColorMap::hotRNeg2(double x)
{
    //Describe the red fuzzy set
    if(x < 0.5625)
        return 0;
    else if(x < 0.8438)
        return (int)floor(linearSlope(x, 3.5549, -1.9996)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotGNeg2(double x)
{
    //Describe the green fuzzy set
    if(x < 0.8438)
        return 0;
    else if(x >= 0.8438 && x < 0.9531)
        return (int)floor(linearSlope(x, 9.1491, -7.72)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::hotBNeg2(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.9531)
        return 0;
    else
        return (int)floor(linearSlope(x,21.3220,-20.3220)*255);
}

//=============================================================================================================

int ColorMap::boneR(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return (int)floor(linearSlope(x, 0.8471, 0)*255);
    else if(x >= 0.375 && x < 0.75)
        return (int)floor(linearSlope(x, 0.8889, -0.0157)*255);
    else
        return (int)floor(linearSlope(x, 1.396, -0.396)*255);
}

//=============================================================================================================

int ColorMap::boneG(double x)
{
    //Describe the green fuzzy set
    if(x < 0.375)
        return (int)floor(linearSlope(x, 0.8471, 0)*255);
    else if(x >= 0.375 && x < 0.75)
        return (int)floor(linearSlope(x, 1.2237, -0.1413)*255);
    else
        return (int)floor(linearSlope(x, 0.894, 0.106)*255);
}

//=============================================================================================================

int ColorMap::boneB(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.375)
        return (int)floor(linearSlope(x, 1.1712, 0.0039)*255);
    else if(x >= 0.375 && x < 0.75)
        return (int)floor(linearSlope(x, 0.8889, 0.1098)*255);
    else
        return (int)floor(linearSlope(x, 0.8941, 0.1059)*255);
}

//=============================================================================================================

int ColorMap::rbR(double x)
{
    //Describe the red fuzzy set
    if(x < 0)
        return (int)floor(linearSlope(x, 1, 1)*255);
    else
        return (int)floor(1.0*255);
}

//=============================================================================================================

int ColorMap::rbG(double x)
{
    //Describe the green fuzzy set
    if(x < 0)
        return (int)floor(linearSlope(x, 1, 1)*255);
    else
        return (int)floor(linearSlope(x, -1, 1)*255);
}

//=============================================================================================================

int ColorMap::rbB(double x)
{
    //Describe the blue fuzzy set
    if(x < 0)
        return (int)floor(1.0*255);
    else
        return (int)floor(linearSlope(x, -1, 1)*255);
}

//=============================================================================================================

int ColorMap::coolR(double x)
{
    //Describe the red fuzzy set
    return (int)floor(linearSlope(x, 1, 0)*255);
}

//=============================================================================================================

int ColorMap::coolG(double x)
{
    //Describe the green fuzzy set
    return (int)floor(linearSlope(x, -1, 1)*255);
}

//=============================================================================================================

int ColorMap::coolB(double x)
{
    Q_UNUSED(x)
    //Describe the blue fuzzy set
    return (int)floor(1.0*255);
}
