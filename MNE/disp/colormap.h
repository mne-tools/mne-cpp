//=============================================================================================================
/**
* @file     colormap.h
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
* @brief    ColorMap class declaration
*
*/

#ifndef COLORMAP_H
#define COLORMAP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QColor>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace DISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Provides diffenrent color maps like HSV,...
*
* @brief Color map RGB transformations
*/
class DISPSHARED_EXPORT ColorMap
{
public:
    typedef QSharedPointer<ColorMap> SPtr;            /**< Shared pointer type for ColorMap class. */
    typedef QSharedPointer<const ColorMap> ConstSPtr; /**< Const shared pointer type for ColorMap class. */
    
    //=========================================================================================================
    /**
    * Default constructor
    */
    ColorMap();
    
    //=========================================================================================================
    /**
    * Destroys the ColorMap class.
    */
    ~ColorMap();

    static inline QRgb valueToHsv(double v);
    
protected:
    static int hsvR(double v);
    static int hsvG(double v);
    static int hsvB(double v);
    static double hsvSlopeMRaising(double x, double n);
    static double hsvSlopeMFalling(double x, double n);

private:
    
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QRgb ColorMap::valueToHsv(double v)
{
    QRgb p_qRgb = qRgb(hsvR(v), hsvG(v), hsvB(v));
    return p_qRgb;
}

} // NAMESPACE

#endif // COLORMAP_H

