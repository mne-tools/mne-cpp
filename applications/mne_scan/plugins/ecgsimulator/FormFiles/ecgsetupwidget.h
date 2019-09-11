//=============================================================================================================
/**
* @file     ecgsetupwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ECGSetupWidget class.
*
*/

#ifndef ECGSETUPWIDGET_H
#define ECGSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include "../ui_ecgsetup.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSIMULATORPLUGIN
//=============================================================================================================

namespace ECGSIMULATORPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ECGSimulator;


//=============================================================================================================
/**
* DECLARE CLASS ECGSetupWidget
*
* @brief The ECGSetupWidget class provides the ECG configuration window.
*/
class ECGSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ECGSetupWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new ECGSetupWidget becomes a window. If parent is another widget, ECGSetupWidget becomes a child window inside parent. ECGSetupWidget is deleted when its parent is deleted.
    * @param [in] simulator a pointer to the corresponding ECGSimulator.
    */
    ECGSetupWidget(ECGSimulator* simulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ECGSetupWidget.
    * All ECGSetupWidget's children are deleted first. The application exits if ECGSetupWidget is the main widget.
    */
    ~ECGSetupWidget();

    //=========================================================================================================
    /**
    * Initializes the sampling rate and the downsampling factor.
    *
    */
    void initSamplingFactors();

    //=========================================================================================================
    /**
    * Initializes each channel comboBox with given the channel file.
    *
    */
    void initSelectedChannelFile();

    //=========================================================================================================
    /**
    * Initializes the channel check boxes for visibility and whether channels are enabled.
    *
    */
    void initChannelStates();

private:

    //=========================================================================================================
    /**
    * Sets the SamplingRate.
    *
    */
    void setSamplingRate(double value);

    //=========================================================================================================
    /**
    * Sets the SamplingRate.
    *
    */
    void setDownsamplingRate(int value);

    //=========================================================================================================
    /**
    * Enables Channel I.
    *
    */
    void setEnabledChannel_I(bool state);

    //=========================================================================================================
    /**
    * Enables Channel II.
    *
    */
    void setEnabledChannel_II(bool state);

    //=========================================================================================================
    /**
    * Enables Channel III.
    *
    */
    void setEnabledChannel_III(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel I.
    *
    */
    void setVisibleChannel_I(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel II.
    *
    */
    void setVisibleChannel_II(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel III.
    *
    */
    void setVisibleChannel_III(bool state);

    //=========================================================================================================
    /**
    * Sets the selected file of Channel I.
    *
    */
    void setFileOfChannel_I(qint32);

    //=========================================================================================================
    /**
    * Sets the selected file of Channel II.
    *
    */
    void setFileOfChannel_II(qint32);

    //=========================================================================================================
    /**
    * Sets the selected file of Channel III.
    *
    */
    void setFileOfChannel_III(qint32);

    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();


    ECGSimulator*           m_pECGSimulator;    /**< a pointer to corresponding ECGSimulator.*/

    Ui::ECGSetupClass ui;                       /**< the user interface for the ECGSetupWidget.*/
};

} // NAMESPACE

#endif // ECGSETUPWIDGET_H
