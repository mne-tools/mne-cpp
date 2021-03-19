//=============================================================================================================
/**
 * @file     draggableframelesswidget.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the DraggableFramelessWidget Class.
 *
 */

#ifndef DRAGGABLEFRAMELESSWIDGET_H
#define DRAGGABLEFRAMELESSWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS DraggableFramelessWidget
 *
 * @brief The DraggableFramelessWidget class provides draggable and frameless QWidget.
 */
class DISPSHARED_EXPORT DraggableFramelessWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<DraggableFramelessWidget> SPtr;              /**< Shared pointer type for DraggableFramelessWidget. */
    typedef QSharedPointer<const DraggableFramelessWidget> ConstSPtr;   /**< Const shared pointer type for DraggableFramelessWidget. */

    //=========================================================================================================
    /**
     * Constructs a DraggableFramelessWidget which is a child of parent.
     *
     * @param[in] parent        The parent of the widget.
     * @param[in] flags         The window flags.
     * @param[in] bRoundEdges   Flag specifying whether to round the edges.
     * @param[in] bDraggable    Flag specifying whether this widget is draggable.
     * @param[in] bFrameless    Flag specifying whether this widget is frameless.
     */
    DraggableFramelessWidget(QWidget *parent = 0,
                             Qt::WindowFlags flags = 0,
                             bool bRoundEdges = false,
                             bool bDraggable = true,
                             bool bFrameless = true);

    //=========================================================================================================
    /**
     * Destructs a DraggableFramelessWidget
     */
    ~DraggableFramelessWidget();

    //=========================================================================================================
    /**
     * Set the draggable flag of this widget.
     *
     * @param[in] bFlag  the flag to set.
     */
    void setDraggable(bool bFlag);

protected:
    //=========================================================================================================
    /**
     * Reimplmented mouseMoveEvent.
     */
    void mouseMoveEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
     * Reimplmented mouseMoveEvent.
     */
    void mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
     * Reimplmented mouseReleaseEvent.
     */
    void mouseReleaseEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
     * Reimplmented mouseMoveEvent.
     */
    void resizeEvent(QResizeEvent *event);

    //=========================================================================================================
    /**
     * Calculates a rect with rounded edged.
     *
     * @param[in] rect the rect which is supposed to be rounded.
     * @param[in] r the radius of round edges.
     * @return the rounded rect in form of a QRegion.
     */
    QRegion roundedRect(const QRect& rect, int r);

private:
    QPoint      m_dragPosition;     /**< The drag position of the window. */
    bool        m_bRoundEdges;      /**< Flag specifying whether to round the edges. */
    bool        m_bDraggable;       /**< Flag specifying whether this widget is draggable. */
    bool        m_bMousePressed;       /**< Flag specifying whether this widget is draggable. */
};
} // NAMESPACE DISPLIB

#endif // DRAGGABLEFRAMELESSWIDGET_H
