//=============================================================================================================
/**
 * @file     scalecontrol.h
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     Feb, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of the ScaleControl Class.
 *
 */
#ifndef SCALECONTROL_H
#define SCALECONTROL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMenu>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class ScaleControlWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB {

//=============================================================================================================
/**
 * DECLARE CLASS ScaleControl
 *
 * @brief The ScaleControl class packs together a QLabel, a DoubleSpinbox and a Slider with the necessary facilities for the
 * interaction between them.
 */
class DISPSHARED_EXPORT ScaleControl : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel element will be set.
     *
     * @param[in] label    Text for the Qlabel.
     */
     explicit ScaleControl(const char* label);

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel element will be set, and sets the parent of the underlying
     * QWidget.
     *
     * @param[in] label    Text for the Qlabel.
     * @param[in] parent   Parent of widget.
     */
    explicit ScaleControl(const char* label,
                          QWidget* parent);

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel element will be set, and sets the parent of the underlying
     * QWidget. The min and max values for the control will also be initialized.
     *
     * @param[in] label    Text for the Qlabel.
     * @param[in] parent   Parent of widget.
     * @param[in] min      Min value of the control.
     * @param[in] max      Max value of the control.
     */
     explicit ScaleControl(const char* label,
                           QWidget* parent,
                           double min,
                           double max);

    //=========================================================================================================
     /**
     * getUI Return a pointer to the GUI of the ScaleControl.
     *
     * @return Pointer to the ScaleControlWidget.
     */
    Ui::ScaleControlWidget* getUI();

    //=========================================================================================================
    /**
     * Return the value of the control
     *
     * @return value of the control.
     */
    double value() const;

    //=========================================================================================================
    /**
     * Set the value of the maximum sensitivity point for the non-linear sensitivity curve of the sensor.
     *
     * @param[in] s Maximum sensitivity value.
     */
    void setMaxSensitivityPoint(double s);

    //=========================================================================================================
    /**
     * Set the sensitivity value of the non-linear mapping between the control slider and the control value.
     *
     * @param[in] s Sensitivity, between [0, 1].
     */
    void setSensitivity(double s);

    //=========================================================================================================
    /**
     * Set the range of the Scalecontrol object. Tha it the maximum and minimum values allowed.
     *
     * @param[in] min Minimum value to be allowed for the Scalecontrol.
     * @param[in] max Maximum value to be allowed for the Scalecontrol.
     */
    void setRange(double min,
                  double max);

    //=========================================================================================================
    /**
     * Set the number of decimals to show in the spinbox of the control.
     *
     * @param[in] d Number of decimals to show in the spinbox.
     */
    void setDecimals(int d);

    //=========================================================================================================
    /**
     * Invert the effect of sliding the control slider to the right. Slider right decreases the control value.
     *
     * @param[in] inverted (True = Invert the slider effect).
     */
    void invertSlider(bool inverted);

public slots:

    //=========================================================================================================
    /**
     * Set the value of the control.
     *
     * @param[in] dScale Value to set the control to.
     */
    void setValue(double dScale);

signals:
    //=========================================================================================================
    /**
     * Signal emitted when the value of the control changes.
     *
     * @param[in] dScale The new value of the control.
     */
    void valueChanged(double dScale);

private:

    //=========================================================================================================
    /**
     * Set the text label of the Control.
     *
     * @param[in] label New label text.
     */
    void initLabel(const char* charTextLabel);

    //=========================================================================================================
    /**
     * Initialize the spinbox settings.
     */
    void initSpinBox();

    //=========================================================================================================
    /**
     * Initialize the slider settings.
     */
    void initSlider();

    //=========================================================================================================
    /**
     * Initializes the menu options for fine control of the scaling.
     */
    void initMenu();

    //=========================================================================================================
    /**
     * Callback executed whenever the value of the spinbox has changed.
     *
     * @param[in] dScale The new value of the spinbox.
     */
    void spinBoxChanged(double dScale);

    //=========================================================================================================
    /**
     * Callback executed whenever the position of the slider has changed.
     *
     * @param[in] dScale New value of the slider.
     */
    void sliderChanged(int dScale);

    //=========================================================================================================
    /**
     * Member method to set the range of the slider. Normally this is set to [1,1000] and there should be no reason
     * to change it since the non-linear mapping function between the slider and the spinbox should take care of
     * adapting the actual value of the control (the spinbox) with the values of the slider. Not even if there is
     * a need to invert the behavior of the slider since the invertSlider should take care of that.
     *
     *@param[in] iMin Minimum value for the slider.
     *@param[in] iMax Maximum value for the slider.
     */
    void setSliderRange(int min,
                        int max);

    //=========================================================================================================
    /**
     * Member function to map the value of the spinbox to a position in the slider bar consistent with the maximum
     * and minimum levels of the control.
     *
     * @param[in] dIn Value of the spinbox or the control.
     * @return Value to set the slider to, consistent with the range of the slider.
     */
    inline int mapSpinBoxToSlider(double dIn);

    //=========================================================================================================
    /**
     * Member function to map the value of a slider poisition with a value for the control or the spinbox. This
     * functions adapts the ranges of the control/spinbox to the ranges of the slider for consistency between
     * ranges.
     *
     * @param[in] iIn Integer value representing the position of the slider.
     * @return Double value to set the spinbox to.
     */
    inline double mapSliderToSpinBox(int iIn);

    //=========================================================================================================
    /**
     * Member function to update the constant values of the mapping functions between spinbox and the slider.
     * These constant only have be updated whenever the ranges or the sensitivity values of the non-linear mapping
     * functions between slider and spinbox are modified.
     */
    void updateNLMapConstants();

    //=========================================================================================================
    /**
     * Member function to addapt the sensitivity value to different range levels.
     *
     * @param[in] fSensitivity Sensitivity value.
     * @return Weighted sensitivity value
     */
    inline float weightedSensitivity(float fSensitivity);

    //=========================================================================================================
    /**
     * Prompts user to set minimum value
     */
    void promptMinValueChange();

    //=========================================================================================================
    /**
     * @Prompts user to set maximum value
     */
    void promptMaxValueChange();

    Ui::ScaleControlWidget* m_pUi;                          /**< Pointer to the user interface object. */
    bool                    m_bManagingSpinBoxChange;       /**< Bool member guarding the state of the spinbox. */
    bool                    m_bManagingSliderChange;        /**< Bool member guarding the state of the slider. */
    float                   m_fSensitivity;                 /**< Sensitivity of the non-linear mapping fcn for the slider. */
    float                   m_fSensitivityWeighted;         /**< Sensitivity of the non-linear mapping fcn, weighted by the max value of the spinbox. */
    float                   m_fMaxSensitivityPoint;         /**< Max sensitivity point of the non-linear mapping fcn for the slider. */
    float                   m_fMapYconstant;                /**< Y constant in the non-linear mapping curve for the slider. */
    float                   m_fMapKconstant;                /**< K constant in the non-linear mapping curve for the slider. */
    bool                    m_bSliderInverted;              /**< State variable to store the inverted or not state of the slider. */

    QMenu*                  m_pSettingsMenu;
};

}//DISPLIB NAMESPACE

#endif // SCALECONTROL_H
