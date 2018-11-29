//=============================================================================================================
/**
* @file     averageselectionview.h
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
* @brief    Declaration of the AverageSelectionView Class.
*
*/

#ifndef AVERAGESELECTIONVIEW_H
#define AVERAGESELECTIONVIEW_H


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
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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


//=============================================================================================================
/**
* DECLARE CLASS AverageSelectionView
*
* @brief The AverageSelectionView class provides a view to activate and choose colors for different averages
*/
class DISPSHARED_EXPORT AverageSelectionView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<AverageSelectionView> SPtr;              /**< Shared pointer type for AverageSelectionView. */
    typedef QSharedPointer<const AverageSelectionView> ConstSPtr;   /**< Const shared pointer type for AverageSelectionView. */

    //=========================================================================================================
    /**
    * Constructs a AverageSelectionView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    AverageSelectionView(QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    void setAverageColor(const QMap<QString, QColor>& qMapAverageColor);
    void setAverageActivation(const QMap<QString, bool>& qMapAverageActivation);

protected:
    //=========================================================================================================
    /**
    * Sets corresponding evoked set
    *
    * @param [in] pEvokedSet      The evoked set
    */
    void update();

    //=========================================================================================================
    /**
    * Call this slot whenever the averages changed.
    */
    void onAveragesChanged();

    int m_iMaxNumAverages;

    QMap<QString, QColor>                   m_qMapAverageColor;             /**< Average colors. */
    QMap<QString, bool>                     m_qMapAverageActivation;        /**< Average activation status. */

signals:    
    void newAverageColor(const QMap<QString, QColor>& qMapAverageColor);
    void newAverageActivation(const QMap<QString, bool>& qMapAverageActivation);
};

} // NAMESPACE

#endif // AVERAGESELECTIONVIEW_H
