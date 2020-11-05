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
    void setBem(const QString& sFileName);

    //=========================================================================================================
    void setMri(const QString& sFileName);

    //=========================================================================================================
    void setNoise(const QString& sFileName);

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
    Ui::DipoleFitViewWidget*    m_pUi;

signals:
    //=========================================================================================================
    void performDipoleFit();

    //=========================================================================================================
    void modalityChanged(int iModality);

    //=========================================================================================================
    void timeChanged(int iMin, int iMax, int iStep);

    //=========================================================================================================
    void fittingChanged(int iMinDistance, int iGridSize);



};
}//namespace

#endif // DIPOLEFITVIEW_H
