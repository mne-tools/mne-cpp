//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsimpedanceview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the EEGoSportsImpedanceView class.
 */

#ifndef EEGOSPORTSIMPEDANCEVIEW_H
#define EEGOSPORTSIMPEDANCEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <eegosportselectrodeitem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsView>
#include <QWheelEvent>

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
/**
 * EEGoSportsImpedanceView...
 *
 * @brief The EEGoSportsImpedanceView class provides a reimplemented QGraphicsView.
 */
class EEGoSportsImpedanceView : public QGraphicsView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     *  Constructs a EEGoSportsImpedanceView.
     */
    explicit EEGoSportsImpedanceView(QWidget *parent = 0);

private:
    //=========================================================================================================
    /**
     *  Reimplemented wheel event used for zoomin in and out of the scene.
     */
    void wheelEvent(QWheelEvent* event);

    //=========================================================================================================
    /**
     *  Reimplemented resize event used scaling fitting the scene into the view after a resize occured.
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     *  Reimplemented mouse press event handler.
     */
    void mouseDoubleClickEvent(QMouseEvent* event);
};
} // NAMESPACE

#endif // EEGOSPORTSIMPEDANCEVIEW_H