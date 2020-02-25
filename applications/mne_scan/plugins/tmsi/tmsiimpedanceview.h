//=============================================================================================================
/**
 * @file     tmsiimpedanceview.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     June, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the TMSIImpedanceView class.
 *
 */

#ifndef TMSIIMPEDANCEVIEW_H
#define TMSIIMPEDANCEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include <tmsielectrodeitem.h>


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
