#ifndef SCALECONTROL_H
#define SCALECONTROL_H


#include "disp_global.h"
#include <QObject>

class QLabel;
class QDoubleSpinBox;
class QSlider;
class QGridLayout;

namespace DISPLIB {

//=============================================================================================================
/**
 * DECLARE CLASS ScaleControl
 * @brief The ScaleControl class packs togethere a DoubleSpinbox and a Slider with the necessary facilities for the
 * interaction between them.
 */
class DISPSHARED_EXPORT ScaleControl : public QObject
{
public:
    ScaleControl(const char* label);
    ScaleControl(const char* label, double min, double max);

//    void addToLayout(QGridLayout* layout) const;
    void addToLayout(QGridLayout* layout, int i) const;

//    QLabel* getLabel() const;
//    QDoubleSpinBox* getSpinBox() const;
//    QSlider* getSlider();

    void setLabelVisible();
    void setSpinboxVisible();
    void setSliderVisible();
    void setSliderInverted(bool s);

    void setMaxSensitivityValue(double s);
    void setSensitivity(double s);
    void setRange(double min, double max);
    void setValue(double value);



signals:
    void valueChanged(double value);

private:

    void initLabel(const char*);
    void initSpinBox();
    void initSlider();

    void spinBoxChanged(double value);

    void sliderChanged(int value);
    void setSliderRange(int min, int max);

    inline int mapSpinBoxToSlider(double in);

    inline double mapSliderToSpinBox(int in);

    void updateMapConstants();


    QLabel*             m_pLabel;                       /**< Weak pointer to label control. */
    QDoubleSpinBox*     m_pSpinBox;                     /**< Weak pointer to spinbox control. */
    QSlider*            m_pSlider;                      /**< Weak pointer to slider control. */
    bool                m_bManagingSpinBoxChange;       /**< Bool member guarding the state of the spinbox. */
    bool                m_bManagingSliderChange;        /**< Bool member guarding the state of the slider. */
    float               m_fSensitivity;
    float               m_fMaxSensitivityPoint;
    float               m_fMapYconstant;
    float               m_fMapKconstant;
    float               m_bSliderInverted;
};


}//DISPLIB NAMESPACE

#endif // SCALECONTROL_H
