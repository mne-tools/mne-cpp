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
* @brief    ColorMap class declaration
*
*/

#ifndef COLORMAP_H
#define COLORMAP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QColor>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Provides diffenrent color maps like Jet,...
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
    * Destructs the ColorMap class.
    */
    ~ColorMap();

    //=========================================================================================================
    /**
    * Returns a Jet RGB to a given double value [0,1] and a colormap specified by sMap.
    * If no matching colormap was found return Jet.
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    * @param[in] sMap   the colormap to choose
    *
    * @return the corresponding RGB value
    */
    static inline QRgb valueToColor(double v, const QString& sMap);

    //=========================================================================================================
    /**
    * Returns a Jet RGB to a given double value [0,1]
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    *
    * @return the corresponding Jet RGB
    */
    static inline QRgb valueToJet(double v);

    //=========================================================================================================
    /**
    * Returns a Hot RGB to a given double value [0,1]
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    *
    * @return the corresponding Hot RGB
    */
    static inline QRgb valueToHot(double v);

    //=========================================================================================================
    /**
    * Returns a negative skewed hot RGB to a given double value [0,1]
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    *
    * @return the corresponding negative skewed Hot RGB
    */
    static inline QRgb valueToHotNegative1(double v);

    //=========================================================================================================
    /**
    * Returns a negative skewed hot RGB to a given double value [0,1]
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    *
    * @return the corresponding negative skewed Hot RGB
    */
    static inline QRgb valueToHotNegative2(double v);

    //=========================================================================================================
    /**
    * Returns a Bone RGB to a given double value [0,1]
    *
    * @param[in] v      the double which has to be part of the intervall [0,1]
    *
    * @return the corresponding Bone RGB
    */
    static inline QRgb valueToBone(double v);

    //=========================================================================================================
    /**
    * Returns a RedBlue RGB to a given double value [-1,1]
    *
    * @param[in] v      the double which has to be part of the intervall [-1,1]
    *
    * @return the corresponding Bone RGB
    */
    static inline QRgb valueToRedBlue(double v);
    
protected:
    //=========================================================================================================
    /**
    * Describes a linear function (y = mx + n) and returns the output value y
    *
    * @param[in] x  input value
    * @param[in] m  slope
    * @param[in] n  offset
    *
    * @return the output value
    */
    static double linearSlope(double x, double m, double n);

    //=========================================================================================================
    /**
    * Describes the red Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int jetR(double v);
    //=========================================================================================================
    /**
    * Describes the green Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int jetG(double v);
    //=========================================================================================================
    /**
    * Describes the blue Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int jetB(double v);

    //=========================================================================================================
    /**
    * Describes the red Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotR(double v);
    //=========================================================================================================
    /**
    * Describes the green Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotG(double v);
    //=========================================================================================================
    /**
    * Describes the blue Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotB(double v);

    //=========================================================================================================
    /**
    * Describes the red Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotRNeg1(double v);
    //=========================================================================================================
    /**
    * Describes the green Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotGNeg1(double v);
    //=========================================================================================================
    /**
    * Describes the blue Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotBNeg1(double v);

    //=========================================================================================================
    /**
    * Describes the red Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotRNeg2(double v);
    //=========================================================================================================
    /**
    * Describes the green Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotGNeg2(double v);
    //=========================================================================================================
    /**
    * Describes the blue Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int hotBNeg2(double v);

    //=========================================================================================================
    /**
    * Describes the red Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int boneR(double v);
    //=========================================================================================================
    /**
    * Describes the green Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int boneG(double v);
    //=========================================================================================================
    /**
    * Describes the blue Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int boneB(double v);

    //=========================================================================================================
    /**
    * Describes the red RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int rbR(double v);
    //=========================================================================================================
    /**
    * Describes the green RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int rbG(double v);
    //=========================================================================================================
    /**
    * Describes the blue RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
    *
    * @param[in] v  input value [0,1]
    *
    * @return the output color value [0.255]
    */
    static int rbB(double v);

private:
    
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QRgb ColorMap::valueToColor(double v, const QString& sMap)
{
    if(sMap == "Hot") {
        return valueToHot(v);
    }

    if(sMap == "HotNegative1") {
        return valueToHotNegative1(v);
    }

    if(sMap == "HotNegative2") {
        return valueToHotNegative2(v);
    }

    if(sMap == "Bone") {
        return valueToBone(v);
    }

    if(sMap == "RedBlue") {
        return valueToRedBlue(v);
    }

    // If no matching colormap was found return Jet
    return valueToJet(v);
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToJet(double v)
{
    QRgb p_qRgb = qRgb(jetR(v), jetG(v), jetB(v));
    return p_qRgb;
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToHot(double v)
{
    QRgb p_qRgb = qRgb(hotR(v), hotG(v), hotB(v));
    return p_qRgb;
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToHotNegative1(double v)
{
    QRgb p_qRgb = qRgb(hotRNeg1(v), hotGNeg1(v), hotBNeg1(v));
    return p_qRgb;
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToHotNegative2(double v)
{
    QRgb p_qRgb = qRgb(hotRNeg2(v), hotGNeg2(v), hotBNeg2(v));
    return p_qRgb;
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToBone(double v)
{
    QRgb p_qRgb = qRgb(boneR(v), boneG(v), boneB(v));
    return p_qRgb;
}


//*************************************************************************************************************

inline QRgb ColorMap::valueToRedBlue(double v)
{
    QRgb p_qRgb = qRgb(rbR(v), rbG(v), rbB(v));
    return p_qRgb;
}


} // NAMESPACE

#endif // COLORMAP_H

