//=============================================================================================================
/**
 * @file     annotationview.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the AnnotationView Class.
 *
 */

#ifndef ANNOTATIONVIEW_H
#define ANNOTATIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMap>

namespace Ui {
    class EventWindowDockWidget;
}

class AnnotationView : public QWidget
{
public:
    AnnotationView();

protected slots:

    //=========================================================================================================
    /**
     * jumpToEvent jumps to a event specified in the event table view
     *
     * @param [in] current model item focused in the view
     * @param [in] previous model item focused in the view
     */
    void jumpToEvent(const QModelIndex &current, const QModelIndex &previous);

    //=========================================================================================================
    /**
     * jumpToEvent jumps to a event specified in the event table view
     */
    void removeEventfromEventModel();

    //=========================================================================================================
    /**
     * Adds an event to the event model and its QTableView
     */
    void addEventToEventModel();

    //=========================================================================================================
    /**
     * call this function whenever a new event type is to be added
     */
    void addNewEventType();


    Ui::EventWindowDockWidget* ui;
};

#endif // ANNOTATIONVIEW_H
