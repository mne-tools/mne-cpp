//=============================================================================================================
/**
 * @file     multiviewwindow.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
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
 * @brief    MultiViewWindow class declaration.
 *
 */

#ifndef MULTIVIEWWINDOW_H
#define MULTIVIEWWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QSharedPointer>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE
{

//*************************************************************************************************************
//=============================================================================================================
// MNEANALYZE FORWARD DECLARATIONS
//=============================================================================================================

class MdiView;

//=============================================================================================================
/**
 * @brief The MultiViewWindow class inherits from QMdiArea and allows printing of subwindows.
 */
class MultiViewWindow : public QDockWidget
{
    Q_OBJECT
public:
    typedef QSharedPointer<MultiViewWindow> SPtr;            /**< Shared pointer type for MultiViewWindow. */
    typedef QSharedPointer<const MultiViewWindow> ConstSPtr; /**< Const shared pointer type for MultiViewWindow. */

    //=========================================================================================================
    /**
     * Constructs an MultiViewWindow.
     */
    explicit MultiViewWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs an MultiViewWindow.
     */
    ~MultiViewWindow();

    void onTopLevelChanged(bool flag);

    QWidget * window;
    QWidget * oldparent = Q_NULLPTR;
    MdiView * mdView;
    bool windowmode;
private:
};

}// NAMESPACE

#endif // MULTIVIEWWINDOW_H
