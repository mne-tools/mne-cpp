//=============================================================================================================
/**
* @file     averagelayoutview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the AverageLayoutView Class.
*
*/

#ifndef AVERAGELAYOUTVIEW_H
#define AVERAGELAYOUTVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsView;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class AverageScene;


//=============================================================================================================
/**
* DECLARE CLASS AverageLayoutView
*
* @brief The AverageLayoutView class provides a widget for a 2D average layout
*/
class DISPSHARED_EXPORT AverageLayoutView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<AverageLayoutView> SPtr;              /**< Shared pointer type for AverageLayoutView. */
    typedef QSharedPointer<const AverageLayoutView> ConstSPtr;   /**< Const shared pointer type for AverageLayoutView. */

    //=========================================================================================================
    /**
    * Constructs a AverageLayoutView which is a child of parent.
    *
    * @param [in] parent    parent of widget
    */
    AverageLayoutView(QWidget *toolbox);

    QSharedPointer<AverageScene> getAverageScene();
    QSharedPointer<QGraphicsView> getAverageGraphicsView();

protected:
    QSharedPointer<AverageScene>        m_pAverageScene;            /**< The pointer to the average scene. */
    QSharedPointer<QGraphicsView>       m_pAverageLayoutView;       /**< View for 2D average layout scene */


signals:

};

} // NAMESPACE

#endif // AVERAGELAYOUTVIEW_H
