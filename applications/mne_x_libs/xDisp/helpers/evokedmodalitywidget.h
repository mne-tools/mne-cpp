//=============================================================================================================
/**
* @file     evokedmodalitywidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the EvokedModalityWidget Class.
*
*/

#ifndef EVOKEDMODALITYWIDGET_H
#define EVOKEDMODALITYWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QCheckBox>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RealTimeEvokedWidget;


//=============================================================================================================
/**
* DECLARE CLASS EvokedModalityWidget
*
* @brief The EvokedModalityWidget class provides the sensor selection widget
*/
class EvokedModalityWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a EvokedModalityWidget which is a child of parent.
    *
    * @param [in] parent    parent of widget
    * @param [in] f         widget flags
    */
    EvokedModalityWidget(RealTimeEvokedWidget *toolbox);

    void updateSelection(qint32 state);

signals:
    void selectionChanged(QStringList);

private:
    RealTimeEvokedWidget * m_pRealTimeEvokedWidget; /**< Connected real-time evoked widget */

    QList<QCheckBox*>   m_qListModalityCheckBox;    /**< List of modality checkboxes */

    QStringList m_qListModalities;              /**< List of modalities */
};

} // NAMESPACE

#endif // EVOKEDMODALITYWIDGET_H
