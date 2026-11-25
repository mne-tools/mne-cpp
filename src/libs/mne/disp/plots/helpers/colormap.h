//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     colormap.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Static scalar-to-colour lookup helpers (Jet, Hot, Bone, Viridis, Cool, RedBlue, MNE) used by every plot widget.
 *
 * ColorMap exposes pure-function @c valueToXxx(double v) accessors that map
 * a value in @f$[0,1]@f$ to a @c QRgb; the maps themselves are Qt-free in
 * mne::scene (colormap.h). Bundled palettes include the perceptually-uniform
 * Viridis, the MATLAB classics (Jet, Hot, Bone, Cool), a diverging
 * Red-Blue map for signed data and the asymmetric MNE-Python @c hot/
 * @c hotNeg variants used by the source-estimate overlays.
 */

#ifndef COLORMAP_H
#define COLORMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include <mne/scene/colormap.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief Static lookup helpers mapping scalar values to @c QRgb / @c QColor across all mne_disp palettes.
 *
 * The @c valueToXxx accessors map any value in @f$[0,1]@f$ to a colour
 * without instantiating an object; mne::scene::map_color() is the Qt-free
 * equivalent. Bundled palettes include Viridis, Jet, Hot, HotNeg1,
 * HotNeg2, Bone, Cool, RedBlue and the MNE-Python @c hot variant.
 *
 * @snippet ex_disp_plots/main.cpp colormap_usage
 */
class DISPSHARED_EXPORT ColorMap
{
public:
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

private:
    static QRgb toQRgb(scene::Color c);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QRgb ColorMap::valueToColor(double v, const QString& sMap)
{
    return toQRgb(scene::map_color(sMap.toStdString(), v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToJet(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::Jet, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToHot(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::Hot, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToHotNegative1(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::HotNegative1, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToHotNegative2(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::HotNegative2, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToBone(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::Bone, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToRedBlue(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::RedBlue, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToCool(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::Cool, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToViridis(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::Viridis, v));
}

//=============================================================================================================

inline QRgb ColorMap::valueToViridisNegated(double v)
{
    return toQRgb(scene::map_color(scene::Colormap::ViridisNegated, v));
}

//=============================================================================================================

inline QRgb ColorMap::toQRgb(scene::Color c)
{
    return qRgba(c.r, c.g, c.b, c.a);
}

} // NAMESPACE

#endif // COLORMAP_H
