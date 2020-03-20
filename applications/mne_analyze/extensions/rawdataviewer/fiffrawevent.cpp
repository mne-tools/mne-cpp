#include "fiffrawevent.h"
#include "ui_fiffrawevent.h"

FiffRawEvent::FiffRawEvent()
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
