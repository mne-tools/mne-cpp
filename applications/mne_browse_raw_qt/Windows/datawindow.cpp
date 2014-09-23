//=============================================================================================================
/**
* @file     datawindow.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the DataWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datawindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataWindow::DataWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataWindowDockWidget),
    m_pMainWindow(static_cast<MainWindow*>(parent)),
    m_pDataMarker(new DataMarker(this)),
    m_pCurrentDataMarkerLabel(new QLabel(this)),
    m_iCurrentMarkerSample(0),
    m_pUndockedViewWidget(new QWidget(this,Qt::Window))
{
    ui->setupUi(this);

    //------------------------
    //--- Setup data model ---
    //------------------------
    if(m_pMainWindow->m_qFileRaw.exists())
        m_pRawModel = new RawModel(m_pMainWindow->m_qFileRaw, this);
    else
        m_pRawModel = new RawModel(this);
}


//*************************************************************************************************************

DataWindow::~DataWindow()
{
    delete ui;
}


//*************************************************************************************************************

void DataWindow::init()
{
    initMVCSettings();
    initUndockedWindow();
    initToolBar();
    initMarker();
    initLabels();
}


//*************************************************************************************************************

QTableView* DataWindow::getDataTableView()
{
    return ui->m_tableView_rawTableView;
}


//*************************************************************************************************************

RawModel* DataWindow::getDataModel()
{
    return m_pRawModel;
}


//*************************************************************************************************************

RawDelegate* DataWindow::getDataDelegate()
{
    return m_pRawDelegate;
}


//*************************************************************************************************************

void DataWindow::initMVCSettings()
{
    //-----------------------------------
    //------ Init data window view ------
    //-----------------------------------
    //Set MVC model
    ui->m_tableView_rawTableView->setModel(m_pRawModel);

    //Set MVC delegate
    m_pRawDelegate = new RawDelegate(this);
    ui->m_tableView_rawTableView->setItemDelegate(m_pRawDelegate);

    //set some settings for m_pRawTableView
    ui->m_tableView_rawTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_iDefaultPlotHeight);
    ui->m_tableView_rawTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    ui->m_tableView_rawTableView->resizeColumnsToContents();

    //set context menu
    ui->m_tableView_rawTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->m_tableView_rawTableView,&QWidget::customContextMenuRequested,
            this,&DataWindow::customContextMenuRequested);

    //activate kinetic scrolling
    QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);

    //connect QScrollBar with model in order to reload data samples
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pRawModel,&RawModel::updateScrollPos);

    //Set MVC in delegate
    m_pRawDelegate->setModelView(m_pMainWindow->m_pEventWindow->getEventModel(),
                                 m_pMainWindow->m_pEventWindow->getEventTableView(),
                                 ui->m_tableView_rawTableView);

    //-----------------------------------
    //------ Init "dockable" view -------
    //-----------------------------------
    m_pUndockedDataView = new QTableView(m_pUndockedViewWidget);
    m_pUndockedDataView->setModel(m_pRawModel);
    m_pUndockedDataView->setItemDelegate(m_pRawDelegate);

    m_pUndockedDataView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_iDefaultPlotHeight);
    m_pUndockedDataView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    m_pUndockedDataView->resizeColumnsToContents();

    m_pUndockedDataView->setAutoScroll(false);
    m_pUndockedDataView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_pUndockedDataView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pUndockedDataView->setShowGrid(false);
    m_pUndockedDataView->horizontalHeader()->setVisible(false);

    //activate kinetic scrolling
    QScroller::grabGesture(m_pUndockedDataView, QScroller::MiddleMouseButtonGesture);

    //connect QScrollBar with model in order to reload data samples
    connect(m_pUndockedDataView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pRawModel,&RawModel::updateScrollPos);

    //-------------------------------------------------------
    //- Interconnect scrollbars of docked and undocked view -
    //-------------------------------------------------------
    //m_pUndockedDataView ---> ui->m_tableView_rawTableView
    connect(m_pUndockedDataView->horizontalScrollBar(),&QScrollBar::valueChanged,
            ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::setValue);
    connect(m_pUndockedDataView->verticalScrollBar(),&QScrollBar::valueChanged,
            ui->m_tableView_rawTableView->verticalScrollBar(),&QScrollBar::setValue);

    //ui->m_tableView_rawTableView ---> m_pUndockedDataView
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pUndockedDataView->horizontalScrollBar(),&QScrollBar::setValue);
    connect(ui->m_tableView_rawTableView->verticalScrollBar(),&QScrollBar::valueChanged,
            m_pUndockedDataView->verticalScrollBar(),&QScrollBar::setValue);

    //Selection test
    //ui->m_tableView_rawTableView->hideRow(0);
}


//*************************************************************************************************************

void DataWindow::initUndockedWindow()
{
    //Add second data view to undocked window
    m_pUndockedDataViewLayout = new QVBoxLayout(m_pUndockedViewWidget);
    m_pUndockedDataViewLayout->addWidget(m_pUndockedDataView);
    m_pUndockedViewWidget->setLayout(m_pUndockedDataViewLayout);
    m_pUndockedViewWidget->hide();

    m_pUndockedViewWidget->setWindowTitle("Data plot");
}


//*************************************************************************************************************

void DataWindow::initToolBar()
{
    //Create toolbar
    QToolBar *toolBar = new QToolBar();
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(false);

    //Add actions to tool bar
    //Add event
    QAction* addEventAction = new QAction(QIcon(":/Resources/Images/addEvent.png"),tr("Add event"), this);
    addEventAction->setStatusTip(tr("Add an event to the event list"));
    connect(addEventAction, SIGNAL(triggered()), this, SLOT(addEventToEventModel()));
    toolBar->addAction(addEventAction);

    toolBar->addSeparator();

    //undock view into new window (not dock widget)
    QAction* undockToWindowAction = new QAction(QIcon(":/Resources/Images/undockView.png"),tr("Undock data view"), this);
    undockToWindowAction->setStatusTip(tr("Undock data view to window"));
    connect(undockToWindowAction, SIGNAL(triggered()), this, SLOT(undockDataViewToWindow()));
    toolBar->addAction(undockToWindowAction);

    int layoutRows = ui->m_gridLayout->rowCount();
    int layoutColumns = ui->m_gridLayout->columnCount();

    ui->m_gridLayout->addWidget(toolBar, 1, layoutColumns, layoutRows-1, 1);
}


//*************************************************************************************************************

void DataWindow::initLabels()
{
    //Setup range samples
    //Connect range sample labels to horizontal sroll bar changes
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            this,&DataWindow::setRangeSampleLabels);

    //Setup marker label
    //Set current marker sample label to vertical spacer position and initalize text
    m_pCurrentDataMarkerLabel->resize(m_pCurrentDataMarkerLabel->width()+15, m_pCurrentDataMarkerLabel->height());
    m_pCurrentDataMarkerLabel->setAlignment(Qt::AlignHCenter);
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left(), m_pDataMarker->geometry().top() + 5);
    QString numberString = QString().number(m_iCurrentMarkerSample);
    m_pCurrentDataMarkerLabel->setText(numberString.append(QString(" / %1").arg("0 sec")));

    //Set color
    QPalette palette;
    QColor textColor = m_qSettings.value("DataMarker/data_marker_color").value<QColor>();
    textColor.setAlpha(m_qSettings.value("DataMarker/data_marker_opacity").toInt());
    palette.setColor(QPalette::WindowText, textColor);

    QColor windowColor;
    windowColor.setAlpha(0); //make qlabel bounding rect fully transparent
    palette.setColor(QPalette::Window, windowColor);

    m_pCurrentDataMarkerLabel->setAutoFillBackground(true);
    m_pCurrentDataMarkerLabel->setPalette(palette);

    //Connect current marker sample label to marker move signal
    connect(m_pDataMarker,&DataMarker::markerMoved,
            this,&DataWindow::setMarkerSampleLabel);

    //Connect current marker sample label to horizontal scroll bar changes
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            this,&DataWindow::setMarkerSampleLabel);
}


//*************************************************************************************************************

void DataWindow::initMarker()
{
    //Set marker as front top-most widget
    m_pDataMarker->raise();

    //Get boundary rect coordinates for table view
    QRect boundingRect = ui->m_tableView_rawTableView->geometry();
    boundingRect.setLeft(boundingRect.x() + ui->m_tableView_rawTableView->verticalHeader()->width());
    boundingRect.setRight(boundingRect.right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1);

    //Inital position of the marker
    m_pDataMarker->move(boundingRect.x(), boundingRect.y() + 1);

    //Create Region from bounding rect - this region is used to restrain the marker inside the data view
    QRegion region(boundingRect);
    m_pDataMarker->setMovementBoundary(region);

    //Set marker size to table view size minus horizontal scroll bar height
    m_pDataMarker->resize(m_qSettings.value("DataMarker/data_marker_width").toInt(),
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);
}


//*************************************************************************************************************

void DataWindow::resizeEvent(QResizeEvent * event)
{
    //On every resize update marker position
    updateMarkerPosition();

    //On every resize set sample informaiton
    setRangeSampleLabels();

    return QWidget::resizeEvent(event);
}


//*************************************************************************************************************

void DataWindow::keyPressEvent(QKeyEvent* event)
{
    QScrollBar* horizontalScrollBar = ui->m_tableView_rawTableView->horizontalScrollBar();

    switch(event->key()) {
    case Qt::Key_Left:
        horizontalScrollBar->setValue(horizontalScrollBar->value() - 25);
        break;
    case Qt::Key_Right:
        horizontalScrollBar->setValue(horizontalScrollBar->value() + 25);
        break;
    }

    return QWidget::keyPressEvent(event);
}


//*************************************************************************************************************

void DataWindow::customContextMenuRequested(QPoint pos)
{
    //obtain index where index was clicked
    //QModelIndex index = m_pRawTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = ui->m_tableView_rawTableView->selectionModel()->selectedIndexes();

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Marking ****************
    QMenu *markingSubMenu = new QMenu("Mark channels",menu);

    QAction* doMarkChBad = markingSubMenu->addAction(tr("Mark as bad"));
    connect(doMarkChBad,&QAction::triggered, [=](){
        m_pRawModel->markChBad(selected,1);
    });

    QAction* doMarkChGood = markingSubMenu->addAction(tr("Mark as good"));
    connect(doMarkChGood,&QAction::triggered, [=](){
        m_pRawModel->markChBad(selected,0);
    });

    //**************** FilterOperators ****************
    //selected channels
    QMenu *filtOpSubMenu = new QMenu("Apply FilterOperator to selected channel",menu);
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pRawModel->m_Operators);
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pRawModel->applyOperator(selected,it.value());
        });
    }

    //all channels
    QMenu *filtOpAllSubMenu = new QMenu("Apply FilterOperator to all channels",menu);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpAllSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pRawModel->applyOperator(QModelIndexList(),it.value());
        });
    }

    //undoing filtering
    QMenu *undoFiltOpSubMenu = new QMenu("Undo filtering",menu);
    QMenu *undoFiltOpSelSubMenu = new QMenu("to selected channels",undoFiltOpSubMenu);

    //undo certain FilterOperators to selected channels
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* undoApplyFilter = undoFiltOpSelSubMenu->addAction(tr("%1").arg(it.key()));

        connect(undoApplyFilter,&QAction::triggered, [=](){
            m_pRawModel->undoFilter(selected,it.value());
        });
    }

    undoFiltOpSubMenu->addMenu(undoFiltOpSelSubMenu);

    //undo all filterting to selected channels
    QAction* undoApplyFilterSel = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to selected channels"));
    connect(undoApplyFilterSel,&QAction::triggered, [=](){
        m_pRawModel->undoFilter(selected);
    });

    //undo all filtering to all channels
    QAction* undoApplyFilterAll = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to all channels"));
    connect(undoApplyFilterAll,&QAction::triggered, [=](){
        m_pRawModel->undoFilter();
    });

    //add everything to main contextmenu
    menu->addMenu(markingSubMenu);
    menu->addMenu(filtOpSubMenu);
    menu->addMenu(filtOpAllSubMenu);
    menu->addMenu(undoFiltOpSubMenu);

    //show context menu
    menu->popup(ui->m_tableView_rawTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void DataWindow::setRangeSampleLabels()
{
    //Set sapce width so that min sample and max sample are in line with the data plot
    ui->m_horizontalSpacer_Min->setFixedWidth(ui->m_tableView_rawTableView->verticalHeader()->width());
    ui->m_horizontalSpacer_Max->setFixedWidth(ui->m_tableView_rawTableView->verticalScrollBar()->width());

    //calculate sample range which is currently displayed in the view
    //Note: the viewport holds the width of the area which is changed through scrolling
    int minSampleRange = ui->m_tableView_rawTableView->horizontalScrollBar()->value()/* + m_pMainWindow->m_pRawModel->firstSample()*/;
    int maxSampleRange = minSampleRange + ui->m_tableView_rawTableView->viewport()->width();

    //Set values as string
    QString stringTemp;
    int minSampleRangeSec = (minSampleRange/m_pRawModel->m_fiffInfo.sfreq)*1000;
    ui->m_label_sampleMin->setText(QString("%1 / %2 sec").arg(stringTemp.number(minSampleRange)).arg(stringTemp.number((double)minSampleRangeSec/1000,'g')));
    int maxSampleRangeSec = (maxSampleRange/m_pRawModel->m_fiffInfo.sfreq)*1000;
    ui->m_label_sampleMax->setText(QString("%1 / %2 sec").arg(stringTemp.number(maxSampleRange)).arg(stringTemp.number((double)maxSampleRangeSec/1000,'g')));
}


//*************************************************************************************************************

void DataWindow::setMarkerSampleLabel()
{
    m_pCurrentDataMarkerLabel->raise();

    //Update the text and position in the current sample marker label
    m_iCurrentMarkerSample = ui->m_tableView_rawTableView->horizontalScrollBar()->value() +
            (m_pDataMarker->geometry().x() - ui->m_tableView_rawTableView->geometry().x() - ui->m_tableView_rawTableView->verticalHeader()->width());

    int currentSeconds = (m_iCurrentMarkerSample/m_pRawModel->m_fiffInfo.sfreq)*1000;

    QString numberString = QString("%1 / %2 sec").arg(QString().number(m_iCurrentMarkerSample)).arg(QString().number((double)currentSeconds/1000,'g'));
    m_pCurrentDataMarkerLabel->setText(numberString);

    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (m_qSettings.value("DataMarker/data_marker_width").toInt()/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);
}


//*************************************************************************************************************

void DataWindow::addEventToEventModel()
{
    m_pMainWindow->m_pEventWindow->getEventModel()->setCurrentMarkerPos(m_iCurrentMarkerSample);
    m_pMainWindow->m_pEventWindow->getEventModel()->insertRow(0, QModelIndex());
}


//*************************************************************************************************************

void DataWindow::undockDataViewToWindow()
{
    m_pUndockedViewWidget->show();
}


//*************************************************************************************************************

void DataWindow::updateMarkerPosition()
{
    //Get boundary rect coordinates for table view
    QRect boundingRect = ui->m_tableView_rawTableView->geometry();

    m_pDataMarker->move(m_pDataMarker->x(), boundingRect.y()+1);

    boundingRect.setLeft(boundingRect.left() + ui->m_tableView_rawTableView->verticalHeader()->width());
    boundingRect.setRight(boundingRect.right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1);

    //Create Region from bounding rect - this region is used to restrain the marker inside the data view
    QRegion region(boundingRect);
    m_pDataMarker->setMovementBoundary(region);

    //If marker is outside of the bounding rect move to edges of bounding rect
    if(m_pDataMarker->pos().x() < boundingRect.left()) {
        m_pDataMarker->move(boundingRect.left(), boundingRect.y()+1);
        setMarkerSampleLabel();
    }

    if(m_pDataMarker->pos().x() > boundingRect.right()) {
        m_pDataMarker->move(boundingRect.right()-2, boundingRect.y()+1);
        setMarkerSampleLabel();
    }

    //Set marker size to table view size minus horizontal scroll bar height
    m_pDataMarker->resize(m_qSettings.value("DataMarker/data_marker_width").toInt(),
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);

    //Update current marker sample lable
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);
}

