//=============================================================================================================
/**
 * @file     colormap.h
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
 * @brief    ColorMap class declaration
 *
 */

#ifndef COLORMAP_H
#define COLORMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QColor>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

const Eigen::MatrixX3i m_matViridrisData = (Eigen::MatrixX3i(256,3) << 68, 1, 84,
                                            68, 2, 86,
                                            69, 4, 87,
                                            69, 5, 89,
                                            70, 7, 90,
                                            70, 8, 92,
                                            70, 10, 93,
                                            70, 11, 94,
                                            71, 13, 96,
                                            71, 14, 97,
                                            71, 16, 99,
                                            71, 17, 100,
                                            71, 19, 101,
                                            72, 20, 103,
                                            72, 22, 104,
                                            72, 23, 105,
                                            72, 24, 106,
                                            72, 26, 108,
                                            72, 27, 109,
                                            72, 28, 110,
                                            72, 29, 111,
                                            72, 31, 112,
                                            72, 32, 113,
                                            72, 33, 115,
                                            72, 35, 116,
                                            72, 36, 117,
                                            72, 37, 118,
                                            72, 38, 119,
                                            72, 40, 120,
                                            72, 41, 121,
                                            71, 42, 122,
                                            71, 44, 122,
                                            71, 45, 123,
                                            71, 46, 124,
                                            71, 47, 125,
                                            70, 48, 126,
                                            70, 50, 126,
                                            70, 51, 127,
                                            70, 52, 128,
                                            69, 53, 129,
                                            69, 55, 129,
                                            69, 56, 130,
                                            68, 57, 131,
                                            68, 58, 131,
                                            68, 59, 132,
                                            67, 61, 132,
                                            67, 62, 133,
                                            66, 63, 133,
                                            66, 64, 134,
                                            66, 65, 134,
                                            65, 66, 135,
                                            65, 68, 135,
                                            64, 69, 136,
                                            64, 70, 136,
                                            63, 71, 136,
                                            63, 72, 137,
                                            62, 73, 137,
                                            62, 74, 137,
                                            62, 76, 138,
                                            61, 77, 138,
                                            61, 78, 138,
                                            60, 79, 138,
                                            60, 80, 139,
                                            59, 81, 139,
                                            59, 82, 139,
                                            58, 83, 139,
                                            58, 84, 140,
                                            57, 85, 140,
                                            57, 86, 140,
                                            56, 88, 140,
                                            56, 89, 140,
                                            55, 90, 140,
                                            55, 91, 141,
                                            54, 92, 141,
                                            54, 93, 141,
                                            53, 94, 141,
                                            53, 95, 141,
                                            52, 96, 141,
                                            52, 97, 141,
                                            51, 98, 141,
                                            51, 99, 141,
                                            50, 100, 142,
                                            50, 101, 142,
                                            49, 102, 142,
                                            49, 103, 142,
                                            49, 104, 142,
                                            48, 105, 142,
                                            48, 106, 142,
                                            47, 107, 142,
                                            47, 108, 142,
                                            46, 109, 142,
                                            46, 110, 142,
                                            46, 111, 142,
                                            45, 112, 142,
                                            45, 113, 142,
                                            44, 113, 142,
                                            44, 114, 142,
                                            44, 115, 142,
                                            43, 116, 142,
                                            43, 117, 142,
                                            42, 118, 142,
                                            42, 119, 142,
                                            42, 120, 142,
                                            41, 121, 142,
                                            41, 122, 142,
                                            41, 123, 142,
                                            40, 124, 142,
                                            40, 125, 142,
                                            39, 126, 142,
                                            39, 127, 142,
                                            39, 128, 142,
                                            38, 129, 142,
                                            38, 130, 142,
                                            38, 130, 142,
                                            37, 131, 142,
                                            37, 132, 142,
                                            37, 133, 142,
                                            36, 134, 142,
                                            36, 135, 142,
                                            35, 136, 142,
                                            35, 137, 142,
                                            35, 138, 141,
                                            34, 139, 141,
                                            34, 140, 141,
                                            34, 141, 141,
                                            33, 142, 141,
                                            33, 143, 141,
                                            33, 144, 141,
                                            33, 145, 140,
                                            32, 146, 140,
                                            32, 146, 140,
                                            32, 147, 140,
                                            31, 148, 140,
                                            31, 149, 139,
                                            31, 150, 139,
                                            31, 151, 139,
                                            31, 152, 139,
                                            31, 153, 138,
                                            31, 154, 138,
                                            30, 155, 138,
                                            30, 156, 137,
                                            30, 157, 137,
                                            31, 158, 137,
                                            31, 159, 136,
                                            31, 160, 136,
                                            31, 161, 136,
                                            31, 161, 135,
                                            31, 162, 135,
                                            32, 163, 134,
                                            32, 164, 134,
                                            33, 165, 133,
                                            33, 166, 133,
                                            34, 167, 133,
                                            34, 168, 132,
                                            35, 169, 131,
                                            36, 170, 131,
                                            37, 171, 130,
                                            37, 172, 130,
                                            38, 173, 129,
                                            39, 173, 129,
                                            40, 174, 128,
                                            41, 175, 127,
                                            42, 176, 127,
                                            44, 177, 126,
                                            45, 178, 125,
                                            46, 179, 124,
                                            47, 180, 124,
                                            49, 181, 123,
                                            50, 182, 122,
                                            52, 182, 121,
                                            53, 183, 121,
                                            55, 184, 120,
                                            56, 185, 119,
                                            58, 186, 118,
                                            59, 187, 117,
                                            61, 188, 116,
                                            63, 188, 115,
                                            64, 189, 114,
                                            66, 190, 113,
                                            68, 191, 112,
                                            70, 192, 111,
                                            72, 193, 110,
                                            74, 193, 109,
                                            76, 194, 108,
                                            78, 195, 107,
                                            80, 196, 106,
                                            82, 197, 105,
                                            84, 197, 104,
                                            86, 198, 103,
                                            88, 199, 101,
                                            90, 200, 100,
                                            92, 200, 99,
                                            94, 201, 98,
                                            96, 202, 96,
                                            99, 203, 95,
                                            101, 203, 94,
                                            103, 204, 92,
                                            105, 205, 91,
                                            108, 205, 90,
                                            110, 206, 88,
                                            112, 207, 87,
                                            115, 208, 86,
                                            117, 208, 84,
                                            119, 209, 83,
                                            122, 209, 81,
                                            124, 210, 80,
                                            127, 211, 78,
                                            129, 211, 77,
                                            132, 212, 75,
                                            134, 213, 73,
                                            137, 213, 72,
                                            139, 214, 70,
                                            142, 214, 69,
                                            144, 215, 67,
                                            147, 215, 65,
                                            149, 216, 64,
                                            152, 216, 62,
                                            155, 217, 60,
                                            157, 217, 59,
                                            160, 218, 57,
                                            162, 218, 55,
                                            165, 219, 54,
                                            168, 219, 52,
                                            170, 220, 50,
                                            173, 220, 48,
                                            176, 221, 47,
                                            178, 221, 45,
                                            181, 222, 43,
                                            184, 222, 41,
                                            186, 222, 40,
                                            189, 223, 38,
                                            192, 223, 37,
                                            194, 223, 35,
                                            197, 224, 33,
                                            200, 224, 32,
                                            202, 225, 31,
                                            205, 225, 29,
                                            208, 225, 28,
                                            210, 226, 27,
                                            213, 226, 26,
                                            216, 226, 25,
                                            218, 227, 25,
                                            221, 227, 24,
                                            223, 227, 24,
                                            226, 228, 24,
                                            229, 228, 25,
                                            231, 228, 25,
                                            234, 229, 26,
                                            236, 229, 27,
                                            239, 229, 28,
                                            241, 229, 29,
                                            244, 230, 30,
                                            246, 230, 32,
                                            248, 230, 33,
                                            251, 231, 35,
                                            253, 231, 37).finished();

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
     * @param[in] v      the double which has to be part of the intervall [0,1].
     * @param[in] sMap   the colormap to choose.
     *
     * @return the corresponding RGB value.
     */
    static inline QRgb valueToColor(double v, const QString& sMap);

    //=========================================================================================================
    /**
     * Returns a Jet RGB to a given double value [0,1]
     *
     * @param[in] v      the double which has to be part of the intervall [0,1].
     *
     * @return the corresponding Jet RGB.
     */
    static inline QRgb valueToJet(double v);

    //=========================================================================================================
    /**
     * Returns a Hot RGB to a given double value [0,1]
     *
     * @param[in] v      the double which has to be part of the intervall [0,1].
     *
     * @return the corresponding Hot RGB.
     */
    static inline QRgb valueToHot(double v);

    //=========================================================================================================
    /**
     * Returns a negative skewed hot RGB to a given double value [0,1]
     *
     * @param[in] v      the double which has to be part of the intervall [0,1].
     *
     * @return the corresponding negative skewed Hot RGB.
     */
    static inline QRgb valueToHotNegative1(double v);

    //=========================================================================================================
    /**
     * Returns a negative skewed hot RGB to a given double value [0,1]
     *
     * @param[in] v      the double which has to be part of the intervall [0,1].
     *
     * @return the corresponding negative skewed Hot RGB.
     */
    static inline QRgb valueToHotNegative2(double v);

    //=========================================================================================================
    /**
     * Returns a Bone RGB to a given double value [0,1]
     *
     * @param[in] v      the double which has to be part of the intervall [0,1].
     *
     * @return the corresponding Bone RGB.
     */
    static inline QRgb valueToBone(double v);

    //=========================================================================================================
    /**
     * Returns a RedBlue RGB to a given double value [-1,1]
     *
     * @param[in] v      the double which has to be part of the intervall [-1,1].
     *
     * @return the corresponding Bone RGB.
     */
    static inline QRgb valueToRedBlue(double v);

    //=========================================================================================================
    /**
     * Returns a Cool RGB to a given double value [-1,1]
     *
     * @param[in] v      the double which has to be part of the intervall [-1,1].
     *
     * @return the corresponding Cool RGB.
     */
    static inline QRgb valueToCool(double v);

    //=========================================================================================================
    /**
     * Returns a Viridis RGB to a given double value [-1,1]
     *
     * @param[in] v      the double which has to be part of the intervall [-1,1].
     *
     * @return the corresponding Cool RGB.
     */
    static QRgb valueToViridis(double v);

    //=========================================================================================================
    /**
     * Returns a negated Viridis RGB to a given double value [-1,1]
     *
     * @param[in] v      the double which has to be part of the intervall [-1,1].
     *
     * @return the corresponding Cool RGB.
     */
    static QRgb valueToViridisNegated(double v);

protected:
    //=========================================================================================================
    /**
     * Describes a linear function (y = mx + n) and returns the output value y
     *
     * @param[in] x  input value.
     * @param[in] m  slope.
     * @param[in] n  offset.
     *
     * @return the output value.
     */
    static double linearSlope(double x, double m, double n);

    //=========================================================================================================
    /**
     * Describes the red Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int jetR(double v);

    //=========================================================================================================
    /**
     * Describes the green Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int jetG(double v);

    //=========================================================================================================
    /**
     * Describes the blue Jet fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int jetB(double v);

    //=========================================================================================================
    /**
     * Describes the red Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotR(double v);

    //=========================================================================================================
    /**
     * Describes the green Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotG(double v);

    //=========================================================================================================
    /**
     * Describes the blue Hot fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotB(double v);

    //=========================================================================================================
    /**
     * Describes the red Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotRNeg1(double v);

    //=========================================================================================================
    /**
     * Describes the green Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotGNeg1(double v);

    //=========================================================================================================
    /**
     * Describes the blue Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotBNeg1(double v);

    //=========================================================================================================
    /**
     * Describes the red Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotRNeg2(double v);

    //=========================================================================================================
    /**
     * Describes the green Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotGNeg2(double v);

    //=========================================================================================================
    /**
     * Describes the blue Hot negative skewed fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int hotBNeg2(double v);

    //=========================================================================================================
    /**
     * Describes the red Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int boneR(double v);

    //=========================================================================================================
    /**
     * Describes the green Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int boneG(double v);

    //=========================================================================================================
    /**
     * Describes the blue Bone fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int boneB(double v);

    //=========================================================================================================
    /**
     * Describes the red RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int rbR(double v);

    //=========================================================================================================
    /**
     * Describes the green RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int rbG(double v);

    //=========================================================================================================
    /**
     * Describes the blue RedBlue fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int rbB(double v);

    //=========================================================================================================
    /**
     * Describes the red Cool fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int coolR(double v);

    //=========================================================================================================
    /**
     * Describes the green Cool fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int coolG(double v);

    //=========================================================================================================
    /**
     * Describes the blue Cool fuzzy set. Calculates to an input value v [0,1] the corresponding output color value [0,255]
     *
     * @param[in] v  input value [0,1].
     *
     * @return the output color value [0.255].
     */
    static int coolB(double v);

private:
    
};

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

    if(sMap == "Cool") {
        return valueToCool(v);
    }

    if(sMap == "Viridis") {
        return valueToViridis(v);
    }

    if(sMap == "ViridisNegated") {
        return valueToViridisNegated(v);
    }

    // If no matching colormap was found return Jet
    return valueToJet(v);
}

//=============================================================================================================

inline QRgb ColorMap::valueToJet(double v)
{
    QRgb p_qRgb = qRgb(jetR(v), jetG(v), jetB(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToHot(double v)
{
    QRgb p_qRgb = qRgb(hotR(v), hotG(v), hotB(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToHotNegative1(double v)
{
    QRgb p_qRgb = qRgb(hotRNeg1(v), hotGNeg1(v), hotBNeg1(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToHotNegative2(double v)
{
    QRgb p_qRgb = qRgb(hotRNeg2(v), hotGNeg2(v), hotBNeg2(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToBone(double v)
{
    QRgb p_qRgb = qRgb(boneR(v), boneG(v), boneB(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToRedBlue(double v)
{
    QRgb p_qRgb = qRgb(rbR(v), rbG(v), rbB(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToCool(double v)
{
    QRgb p_qRgb = qRgb(coolR(v), coolG(v), coolB(v));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToViridis(double v)
{
    if((uint)v*255 >= m_matViridrisData.rows()) {
        return QRgb();
    }

    QRgb p_qRgb = qRgb(m_matViridrisData((uint)v*255,0), m_matViridrisData((uint)v*255,1), m_matViridrisData((uint)v*255,2));
    return p_qRgb;
}

//=============================================================================================================

inline QRgb ColorMap::valueToViridisNegated(double v)
{
    if(255-(uint)v*255 >= m_matViridrisData.rows() || 255-(uint)v*255 < 0) {
        return QRgb();
    }

    QRgb p_qRgb = qRgb(m_matViridrisData(255-(uint)v*255,0), m_matViridrisData(255-(uint)v*255,1), m_matViridrisData(255-(uint)v*255,2));
    return p_qRgb;
}
} // NAMESPACE

#endif // COLORMAP_H

