//=============================================================================================================
/**
 * @file     scalecontrol.h
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief The ScaleControl class packs together a QLable, a DoubleSpinbox and a Slider with the necessary facilities for the
 * interaction between them.
 */
class DISPSHARED_EXPORT ScaleControl : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel elemennt will be set.
     *
     * @param [in] label    text for the Qlabel
     */
    ScaleControl(const char* label);

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel elemennt will be set, and sets the parent of the underlyning
     * QWidget.
     *
     * @param [in] label    text for the Qlabel
     * @param [in] parent   parent of widget
     */
    ScaleControl(const char* label, QWidget* parent);

    //=========================================================================================================
    /**
     * Constructs a ScaleControl object who's Qlabel elemennt will be set, and sets the parent of the underlyning
     * QWidget. The min and max values for the control will also be initialized.
     *
     * @param [in] label    text for the Qlabel
     * @param [in] parent   parent of widget
     * @param [in] min      min value of the control.
     * @param [in] max      max value of the control.
     */
     ScaleControl(const char* label, QWidget* parent, double min, double max);

    //=========================================================================================================
     /**
     * getUI Retunr a pointer to the GUI of the ScaleControl.
     * @return Pointer to the ScaleControlWidget.
     */
    Ui::ScaleControlWidget* getUI();

    //=========================================================================================================
    /**
     * Return the value of the control
     * @return value of the control.
     */
    double value() const;

    //=========================================================================================================
    /**
     * Set the value of the maximum sensitivity point for the non-linear sensitivity curve of the sensor.
     * @param [in] s Maximum sensitivity value
     */
    void setMaxSensitivityPoint(double s);

    //=========================================================================================================
    /**
     * Set the sensitivity value of the non-linear mapping between the control slider and the control value.
     * @param [in] s Sensitivity, between [0, 1]
     */
    void setSensitivity(double s);

    //=========================================================================================================
    /**
     * @brief setRange
     * @param min
     * @param max
     */
    void setRange(double min, double max);

    //=========================================================================================================
    /**
     * @brief setDecimals
     * @param d
     */
    void setDecimals(int d);

    //=========================================================================================================
    /**
     * @brief invertSlider
     * @param inverted
     */
    void invertSlider(bool inverted);

public slots:

    //=========================================================================================================
    /**
     * @brief setValue
     * @param value
     */
    void setValue(double value);

signals:
    //=========================================================================================================
    /**
     * @brief valueChanged
     * @param value
     */
    void valueChanged(double value);

private:

    //=========================================================================================================
    /**
     * @brief initLabel
     */
    void initLabel(const char*);

    //=========================================================================================================
    /**
     * @brief initSpinBox
     */
    void initSpinBox();

    //=========================================================================================================
    /**
     * @brief initSlider
     */
    void initSlider();

    //=========================================================================================================
    /**
     * @brief spinBoxChanged
     * @param value
     */
    void spinBoxChanged(double value);

    //=========================================================================================================
    /**
     * @brief sliderChanged
     * @param value
     */
    void sliderChanged(int value);

    //=========================================================================================================
    /**
     * @brief setSliderRange
     * @param min
     * @param max
     */
    void setSliderRange(int min, int max);

    //=========================================================================================================
    /**
     * @brief mapSpinBoxToSlider
     * @param in
     * @return
     */
    inline int mapSpinBoxToSlider(double in);

    //=========================================================================================================
    /**
     * @brief mapSliderToSpinBox
     * @param in
     * @return
     */
    inline double mapSliderToSpinBox(int in);

    //=========================================================================================================
    /**
     * @brief updateNLMapConstants
     */
    void updateNLMapConstants();

    //=========================================================================================================
    /**
     * @brief weightedSensitivity
     * @param s
     * @return
     */
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
