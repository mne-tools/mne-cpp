//=============================================================================================================
/**
 * @file     noiseestimatesetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NoiseEstimateSetupWidget class.
 *
 */

#ifndef NOISEESTIMATESETUPWIDGET_H
#define NOISEESTIMATESETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_noiseestimatesetup.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NOISEESTIMATEPLUGIN
//=============================================================================================================

namespace NOISEESTIMATEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NoiseEstimate;


//=============================================================================================================
/**
 * DECLARE CLASS NoiseEstimateSetupWidget
 *
 * @brief The NoiseEstimateSetupWidget class provides the NoiseEstimate configuration window.
 */
class NoiseEstimateSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a NoiseEstimateSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding NoiseEstimate.
    * @param [in] parent pointer to parent widget; If parent is 0, the new NoiseEstimateSetupWidget becomes a window. If parent is another widget, RtHpiSetupWidget becomes a child window inside parent. RtHpiSetupWidget is deleted when its parent is deleted.
    */
    NoiseEstimateSetupWidget(NoiseEstimate* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the NoiseEstimateSetupWidget.
    * All NoiseEstimateSetupWidget's children are deleted first. The application exits if NoiseEstimateSetupWidget is the main widget.
    */
    ~NoiseEstimateSetupWidget();

    void init();
    void chgnFFT(int idx);
    void chgDataLen(int idx);
    void chgXAxisType();

private slots:

private:

    NoiseEstimate* m_pNoiseEstimate;	/**< Holds a pointer to corresponding RtHpi.*/

    Ui::NoiseEstimateSetupWidgetClass ui;	/**< Holds the user interface for the RtHpiSetupWidget.*/
};

} // NAMESPACE

#endif // NoiseEstimateSETUPWIDGET_H
