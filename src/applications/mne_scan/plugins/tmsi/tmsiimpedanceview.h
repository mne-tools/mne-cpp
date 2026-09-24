//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsiimpedanceview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the TMSIImpedanceView class.
 */

#ifndef TMSIIMPEDANCEVIEW_H
#define TMSIIMPEDANCEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include "tmsielectrodeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QGraphicsView>
#include <QWheelEvent>

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
/**
 * TMSIImpedanceView...
 *
 * @brief The TMSIImpedanceView class provides a reimplemented QGraphicsView.
 */
class TMSIImpedanceView : public QGraphicsView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSIImpedanceView.
     */
    explicit TMSIImpedanceView(QWidget *parent = 0);

private:
    //=========================================================================================================
    /**
     * Reimplemented wheel event used for zoomin in and out of the scene.
     */
    void wheelEvent(QWheelEvent* event);

    //=========================================================================================================
    /**
     * Reimplemented resize event used scaling fitting the scene into the view after a resize occured.
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     * Reimplemented mouse press event handler.
     */
    void mouseDoubleClickEvent(QMouseEvent* event);
};
} // NAMESPACE

#endif // TMSIIMPEDANCEVIEW_H
