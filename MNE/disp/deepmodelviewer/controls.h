//=============================================================================================================
/**
* @file     controls.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Controls class declaration.
*
*/

#ifndef CONTROL_H
#define CONTROL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QGroupBox;
QT_END_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class View;


//=============================================================================================================
/**
* Controls Widget for Deep Viewer
*
* @brief Line Plot
*/
class DISPSHARED_EXPORT Controls : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs the Controls Widget without an attached view which is a child of parent
    *
    * @param [in] parent    The parent widget
    */
    Controls(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Constructs the Controls Widget with an attached view which is a child of parent
    *
    * @param [in] v         The view which should be controled by this
    * @param [in] parent    The parent widgetarent widget
    */
    Controls(View* v, QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Sets the associated view if none was set before
    *
    * @param [in] v     The view to set
    */
    void setView(View* v);

private:
    //=========================================================================================================
    /**
    * Create the common controls
    *
    * @param [in] parent    the group box where the controls should be attached to
    */
    void createCommonControls(QWidget* parent);

    //=========================================================================================================
    /**
    * Creates the layout
    */
    void createLayout();

private:
    View* m_pView;                  /**< The view which this control is connected to */

    QGroupBox *m_pCapGroup;         /**< The Cap Group */
    QGroupBox *m_pJoinGroup;        /**< The Join Group */
    QGroupBox *m_pStyleGroup;       /**< The Style Group */
    QGroupBox *m_pPathModeGroup;    /**< The Path Mode Group */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // CONTROL_H
