//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     colormap.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ColorMap scalar-to-RGB lookup functions.
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
