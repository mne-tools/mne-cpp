#ifndef COLORLIB_H
#define COLORLIB_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <vector>
#include <initializer_list>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#ifdef QT_CORE_LIB
#include <QGradient>
#include <QColor>
#endif

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

struct Color;
struct ColorPoint;
class ColorMap;
class ColorGradient;


//=============================================================================================================
/**
 * Holds data on an RBG Color
 */
struct Color{
    float r;
    float g;
    float b;
    Color() : Color(0,0,0) {}
    Color(float red,
          float green,
          float blue) : r(red), g(green), b(blue) {}
#ifdef QT_CORE_LIB
    QColor getQColor() const {return QColor(r, g, b);}
#endif
};

//=============================================================================================================
/**
 * Holds data on an RGB Color  associated with a value between 0-1
 */
struct ColorPoint
{
    ColorPoint(float red,
               float green,
               float blue,
               float value): m_color(red, green, blue), val(value) {}
    ColorPoint(Color color,
               int value): m_color(color), val(value) {}
    Color m_color;
    float val;
    float r() const {return m_color.r;}
    float g() const {return m_color.g;}
    float b() const {return m_color.b;}
#ifdef QT_CORE_LIB
    QColor getQColor() const {return m_color.getQColor();}
#endif
};

//=============================================================================================================
/**
 * Holds a colormap
 */
//class ColorMap
//{
//public:
//    uint size() const {return m_vColors.size();}

//    int resolution() const {return m_iResolution;}

//    static ColorMap fromGradient(const ColorGradient& gradient,
//                                 int resolution = 256);
//private:
//    ColorMap();

//    int m_iResolution;
//    std::vector<Color> m_vColors;
//};

//=============================================================================================================
/**
 * A Gradient between multiple colors
 */
class ColorGradient
{
public:
    ColorGradient() = delete;
    ColorGradient(std::initializer_list<ColorPoint> list);

    Color getColor(float value) const;

#ifdef QT_CORE_LIB
    QLinearGradient getQGradient(QPoint from, QPoint to) const;
#endif
//    ColorMap getColorMap();

private:

    std::vector<ColorPoint>     m_vecColors;
};


}//namespace

#endif // COLORGRADIENT_H
