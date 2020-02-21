//=============================================================================================================
/**
 * @file     multiview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    MultiView class declaration.
 *
 */

#ifndef MULTIVIEW_H
#define MULTIVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMdiArea>
#include <QSharedPointer>
#include <QPointer>
#include <QSplitter>

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

class MultiViewWindow;

//=============================================================================================================
/**
 * @brief The MultiView class inherits from QWidget and provides a view with one vertical and one horizontal
 *        QSplitter. The horizontal splitter lives in the most upper element of the vertical QSplitter.
 */
class DISPSHARED_EXPORT MultiView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<MultiView> SPtr;            /**< Shared pointer type for MultiView. */
    typedef QSharedPointer<const MultiView> ConstSPtr; /**< Const shared pointer type for MultiView. */

    //=========================================================================================================
    /**
     * Constructs an MultiView.
     */
    explicit MultiView(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructs an MultiView.
     */
    ~MultiView();

    //=========================================================================================================
    /**
     * Adds a QWidget to the horizontal QSplitter.
     *
     * @param[in] pWidget   The widget to be added.
     * @param[in] sName     The window title shown in the QSplitter.
     *
     * @return Returns a pointer to the added widget in form of a MultiViewWindow.
     */
    MultiViewWindow* addWidgetH(QWidget* pWidget,
                                const QString &sName);

    //=========================================================================================================
    /**
     * Adds a QWidget to the vertical QSplitter.
     *
     * @param[in] pWidget   The widget to be added.
     * @param[in] sName     The window title shown in the QSplitter.
     *
     * @return Returns a pointer to the added widget in form of a MultiViewWindow.
     */
    MultiViewWindow* addWidgetV(QWidget* pWidget,
                                const QString& sName);

private:
    QPointer<QSplitter> m_pSplitterHorizontal;      /**< The horizontal QSplitter. */
    QPointer<QSplitter> m_pSplitterVertical;        /**< The vertical QSplitter. */
};

}// NAMESPACE

#endif // MULTIVIEW_H
