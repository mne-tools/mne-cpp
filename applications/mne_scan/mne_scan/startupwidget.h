//=============================================================================================================
/**
 * @file     startupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains declaration of StartUpWidget class.
 *
 */

#ifndef STARTUPWIDGET_H
#define STARTUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QLabel;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
/**
 * DECLARE CLASS StartUpWidget
 *
 * @brief The StartUpWidget class provides the widget which is shown at start up in the central widget of the main application.
 */
class StartUpWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<StartUpWidget> SPtr;               /**< Shared pointer type for StartUpWidget. */
    typedef QSharedPointer<const StartUpWidget> ConstSPtr;    /**< Const shared pointer type for StartUpWidget. */

    //=========================================================================================================
    /**
     * Constructs a StartUpWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new StartUpWidget becomes a window. If parent is another widget, StartUpWidget becomes a child window inside parent. StartUpWidget is deleted when its parent is deleted.
     */
    StartUpWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the StartUpWidget.
     */
    ~StartUpWidget();

private:
    QLabel* m_pLabel_Info;      /**< Holds the start up widget label. */
};
}//NAMESPACE

#endif // STARTUPWIDGET_H
