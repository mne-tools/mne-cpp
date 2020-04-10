//=============================================================================================================
/**
 * @file     annotationview.cpp
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
 * @brief    Definition of the AnnotationView Class.
 *
 */

#include "annotationview.h"
#include "ui_annotationview.h"

AnnotationView::AnnotationView()
: ui(new Ui::EventWindowDockWidget)
{
    ui->setupUi(this);
}

//void EventWindow::jumpToEvent(const QModelIndex & current, const QModelIndex & previous)
//{
//    Q_UNUSED(previous);

//    if(ui->m_checkBox_activateEvents->isChecked()) {
//        //Always get the first column 0 (sample) of the model - Note: Need to map index from sorting model back to source model
//        QModelIndex index = m_pEventModel->index(current.row(), 0);

//        //Get the sample value
//        int sample = m_pEventModel->data(index, Qt::DisplayRole).toInt();

//        //Jump to sample - put sample in the middle of the view - the viewport holds the width of the are which is changed through scrolling
//        int rawTableViewColumnWidth = m_pMainWindow->m_pDataWindow->getDataTableView()->viewport()->width();

//        if(sample-rawTableViewColumnWidth/2 < rawTableViewColumnWidth/2) //events lie in the first half of the data window at the beginning of the loaded data -> cannot centralize view on event
//            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(0);
//        else if(sample+rawTableViewColumnWidth/2 > m_pMainWindow->m_pDataWindow->getDataModel()->lastSample()-rawTableViewColumnWidth/2) //events lie in the last half of the data window at the end of the loaded data -> cannot centralize view on event
//            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(m_pMainWindow->m_pDataWindow->getDataTableView()->maximumWidth());
//        else //centralize view on event
//            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(sample-rawTableViewColumnWidth/2);

//        qDebug()<<"Jumping to Event at sample "<<sample<<"rawTableViewColumnWidth"<<rawTableViewColumnWidth;

//        m_pMainWindow->m_pDataWindow->updateDataTableViews();
//    }
//}

//void EventWindow::removeEventfromEventModel()
//{
//    QModelIndexList indexList = ui->m_tableView_eventTableView->selectionModel()->selectedIndexes();

//    for(int i = 0; i<indexList.size(); i++)
//        m_pEventModel->removeRow(indexList.at(i).row() - i); // - i because the internal data structure gets smaller by one with each succession in this for statement
//}

//void EventWindow::addEventToEventModel()
//{
//    m_pEventModel->insertRow(0, QModelIndex());
//}

//void EventWindow::addNewEventType()
//{
//    //Open add event type dialog
//    m_pEventModel->addNewEventType(QString().number(ui->m_spinBox_addEventType->value()), m_pColordialog->getColor(Qt::black, this));
//    m_pEventModel->setEventFilterType(QString().number(ui->m_spinBox_addEventType->value()));
//    m_pMainWindow->m_pDataWindow->updateDataTableViews();
//}
