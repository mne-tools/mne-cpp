//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "colorlib.h"

#include <algorithm>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

ColorMap ColorMap::fromGradient(const ColorGradient &gradient,
                                int resolution)
{
    ColorMap map;
    map.m_iResolution = resolution;

    for (int i = 0 ; i < resolution; i++){
        float value = static_cast<float>(i) / static_cast<float>(resolution);
        map.m_vColors.push_back(gradient.getColor(value));
    }

    return map;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ColorGradient::ColorGradient(std::initializer_list<ColorPoint> list)
{
    for (ColorPoint color : list){
        m_vecColors.push_back(color);
    }
}

//=============================================================================================================

Color ColorGradient::getColor(float value) const
{
    if(!m_vecColors.size()){
        return Color(0,0,0);
    }

    Color color(0,0,0);

    for (int i = 0; i< m_vecColors.size(); i++){
        const ColorPoint& currentColor = m_vecColors[i];
        if(value < currentColor.val){
            int index = ((i-1) > 0) ? i-1 : 0;
            const ColorPoint& prevColor = m_vecColors[index];

            float fDifference = currentColor.val - prevColor.val;
            float fRatio = (fDifference) ? ((value - currentColor.val)/fDifference) : 0;

            color.r = (fRatio * (currentColor.r() - prevColor.r())) + currentColor.r();
            color.g = (fRatio * (currentColor.g() - prevColor.g())) + currentColor.g();
            color.b = (fRatio * (currentColor.b() - prevColor.b())) + currentColor.b();

            return color;
        }
    }
    return color;
}

//=============================================================================================================

#ifdef QT_CORE_LIB
QLinearGradient ColorGradient::getQGradient(QPoint from, QPoint to) const
{
    QLinearGradient gradient(from, to);
    for(const ColorPoint& color : m_vecColors){
        gradient.setColorAt(color.val, color.getQColor());
    }

    return gradient;
}
#endif
