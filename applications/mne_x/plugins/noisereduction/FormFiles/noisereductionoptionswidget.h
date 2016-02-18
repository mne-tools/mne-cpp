//=============================================================================================================
/**
* @file     dummytoolbox.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the NoiseReductionOptionsWidget class.
*
*/

#ifndef NOISEREDUCTIONOPTIONSWIDGET_H
#define NOISEREDUCTIONOPTIONSWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_noisereductionoptionswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NoiseReductionPlugin
//=============================================================================================================

namespace NoiseReductionPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NoiseReduction;


//=============================================================================================================
/**
* DECLARE CLASS NoiseReductionOptionsWidget
*
* @brief The NoiseReductionOptionsWidget class provides a NoiseReduction option toolbar widget structure.
*/
}
class NoiseReductionOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NoiseReductionOptionsWidget> SPtr;         /**< Shared pointer type for NoiseReductionOptionsWidget. */
    typedef QSharedPointer<NoiseReductionOptionsWidget> ConstSPtr;    /**< Const shared pointer type for NoiseReductionOptionsWidget. */

    //=========================================================================================================
    /**
    * Constructs a DummyToolbox.
    */
    explicit NoiseReductionOptionsWidget(NoiseReductionPlugin::NoiseReduction* toolbox, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DummyToolbox.
    */
    ~NoiseReductionOptionsWidget();

    //=========================================================================================================
    /**
    * Set the acquisition system type (BabyMEG, VecotrView, EEG).
    *
    * @param[in] sSystem    The type of the acquisition system.
    */
    void setAcquisitionSystem(const QString &sSystem);

protected slots:
    //=========================================================================================================
    /**
    * Call this slot whenever the number basis functions changed.
    */
    void onNBaseFctsChanged();

private:
    Ui::NoiseReductionOptionsWidgetClass*   ui;                         /**< The UI class specified in the designer. */

    NoiseReductionPlugin::NoiseReduction*   m_pNoiseReductionToolbox;
};

#endif // NOISEREDUCTIONOPTIONSWIDGET_H
