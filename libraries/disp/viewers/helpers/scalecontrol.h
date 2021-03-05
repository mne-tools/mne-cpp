#ifndef SCALECONTROL_H
#define SCALECONTROL_H


#include "disp_global.h"
#include <QWidget>

//class QLabel;
//class QDoubleSpinBox;
//class QSlider;
//class QGridLayout;

namespace Ui {
    class ScaleControlWidget;
}

namespace DISPLIB {

//=============================================================================================================
/**
 * DECLARE CLASS ScaleControl
 * @brief The ScaleControl class packs togethere a DoubleSpinbox and a Slider with the necessary facilities for the
 * interaction between them.
 */
class DISPSHARED_EXPORT ScaleControl : public QWidget
{
    Q_OBJECT

public:
    ScaleControl(const char* label);
    ScaleControl(const char* label, QWidget* parent);
    ScaleControl(const char* label, QWidget* parent, double min, double max);

    Ui::ScaleControlWidget* getUI();

    double value() const;
    void setMaxSensitivityPoint(double s);
    void setSensitivity(double s);
    void setRange(double min, double max);
    void setDecimals(int d);
    void invertSlider(bool inverted);

public slots:
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

    void updateNLMapConstants();
    inline float weightedSensitivity(float s);

    Ui::ScaleControlWidget* m_pUi;                          /**< Pointer to the user interface object. */
    bool                    m_bManagingSpinBoxChange;       /**< Bool member guarding the state of the spinbox. */
    bool                    m_bManagingSliderChange;        /**< Bool member guarding the state of the slider. */
    float                   m_fSensitivity;                 /**< Sensitivity of the non-linear mapping fcn for the slider. */
    float                   m_fSensitivityWeighted;         /**< Sensitivity of the non-linear mapping fcn, weighted by the max value of the spinbox. */
    float                   m_fMaxSensitivityPoint;         /**< Max sensitivity point of the non-linear mapping fcn for the slider. */
    float                   m_fMapYconstant;                /**< Y constant in the non-linear mapping curve for the slider. */
    float                   m_fMapKconstant;                /**< K constant in the non-linear mapping curve for the slider. */
    float                   m_bSliderInverted;              /**< State variable to store the inverted or not state of the slider. */
};


}//DISPLIB NAMESPACE

#endif // SCALECONTROL_H
