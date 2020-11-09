//=============================================================================================================
/**
 * @file     dipolefitview.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
 * @date     Novemeber, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the DipoleFitView Class.
 *
 */


#ifndef DIPOLEFITVIEW_H
#define DIPOLEFITVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class DipoleFitViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
class DISPSHARED_EXPORT DipoleFitView : public AbstractView
{
    Q_OBJECT

public:
    DipoleFitView(QWidget *parent = 0,
                  Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Updates GUI to show selected BEM file
     *
     * @param sFileName
     */
    void setBem(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selected MRI file
     *
     * @param sFileName
     */
    void setMri(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selecte Noise file
     *
     * @param sFileName
     */
    void setNoise(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selected measurement file
     *
     * @param sFileName
     */
    void setMeas(const QString& sFileName);

    //=========================================================================================================
    /**
     * @brief requestParams
     */
    void requestParams();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

private:

    //=========================================================================================================
    void initGui();


    Ui::DipoleFitViewWidget*    m_pUi;              /**< Holds GUI for view */

signals:
    //=========================================================================================================
    /**
     * Triggers dipole fit calculation
     */
    void performDipoleFit();

    //=========================================================================================================
    /**
     * Send new modality selection
     *
     * @param [in] bEEG     whether to use EEG (true = yes, false = no)
     * @param [in] bMEG     whether to use MEG (true = yes, false = no)
     */
    void modalityChanged(bool bEEG,
                         bool bMEG);

    //=========================================================================================================
    /**
     * Send new time values
     *
     * @param [in] iMin     new minimum in milliseconds
     * @param [in] iMax     new maximum in milliseconds
     * @param [in] iStep    new step value in milliseconds
     */
    void timeChanged(int iMin,
                     int iMax,
                     int iStep);

    //=========================================================================================================
    /**
     * Send new fitting values
     *
     * @param [in] iMinDistance     distance to inner skull in millimeters
     * @param [in] iGridSize        grid pacing in millimeters
     */
    void fittingChanged(int iMinDistance,
                        int iGridSize);

    //=========================================================================================================
    /**
     * Send new basline values
     *
     * @param [in] iBMin    new minimum in milliseconds
     * @param [in] iBMax    new maximum in milliseconds
     */
    void baselineChanged(int iBMin,
                         int iBMax);

    //=========================================================================================================
    /**
     * Triggers clearing of noise model (ignored in dipole fit until reselected)
     */
    void clearNoise();

    //=========================================================================================================
    /**
     * Triggers clearing of BEM model (ignored in dipole fit until reselected)
     */
    void clearBem();

    //=========================================================================================================
    /**
     * Triggers clearing of MRI model (ignored in dipole fit until reselected)
     */
    void clearMri();

};
}//namespace

#endif // DIPOLEFITVIEW_H
