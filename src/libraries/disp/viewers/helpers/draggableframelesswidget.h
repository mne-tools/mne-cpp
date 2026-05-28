//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     draggableframelesswidget.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Frameless QWidget that can be dragged by clicking anywhere on its body.
 *
 * DraggableFramelessWidget removes the native window frame and
 * implements the mouse-press / mouse-move handlers needed to move
 * the window with the cursor. It is the base class of
 * @ref QuickControlView and any other floating popup that wants to
 * look like an in-app palette rather than a system window.
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
 * @brief Frameless QWidget that can be dragged anywhere on its body.
 *
 * Base class of @ref QuickControlView and any other floating popup
 * that wants to look like an in-app palette rather than a system
 * window.
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
                             Qt::WindowFlags flags = Qt::Window,
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
