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

DataWindow::DataWindow(QWidget *parent)
: QWidget(parent)
, ui(new Ui::DataWindowDockWidget)
, m_pMainWindow(static_cast<MainWindow*>(parent))
, m_pDataMarker(new DataMarker(this))
, m_pCurrentDataMarkerLabel(new QLabel(this))
, m_iCurrentMarkerSample(0)
, m_pUndockedViewWidget(new QWidget(this,Qt::Window))
, m_pUndockedDataView(new QTableView(m_pUndockedViewWidget))
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

QTableView* DataWindow::getUndockedDataTableView()
{
    return m_pUndockedDataView;
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

void DataWindow::updateDataTableViews()
{
    ui->m_tableView_rawTableView->viewport()->update();
    m_pUndockedDataView->viewport()->update();
}


//*************************************************************************************************************

void DataWindow::showSelectedChannelsOnly(QStringList selectedChannels)
{
    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pRawModel->rowCount(); i++) {
        QModelIndex index = m_pRawModel->index(i, 0);
        QString channel = m_pRawModel->data(index, Qt::DisplayRole).toString();

        ui->m_tableView_rawTableView->showRow(i);
        m_pUndockedDataView->showRow(i);

        if(!selectedChannels.contains(channel)) {
            ui->m_tableView_rawTableView->hideRow(i);
            m_pUndockedDataView->hideRow(i);
        }
        else {
            ui->m_tableView_rawTableView->showRow(i);
            m_pUndockedDataView->showRow(i);
        }
    }

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::scaleChannelsInView(double height)
{
    for(int i = 0; i<ui->m_tableView_rawTableView->verticalHeader()->count(); i++)
        ui->m_tableView_rawTableView->setRowHeight(i, height);
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
    m_pKineticScroller = QScroller::scroller(ui->m_tableView_rawTableView);
    m_pKineticScroller->setSnapPositionsX(100,100);
    //m_pKineticScroller->setSnapPositionsY(100,100);

    //connect QScrollBar with model in order to reload data samples
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pRawModel,&RawModel::updateScrollPos);

    //connect selection of a channel to selection manager
    connect(ui->m_tableView_rawTableView->selectionModel(),&QItemSelectionModel::selectionChanged,
            this,&DataWindow::highlightChannelsInSelectionManager);

    //connect selection change to update data views
    connect(ui->m_tableView_rawTableView->selectionModel(),&QItemSelectionModel::selectionChanged,
            this,&DataWindow::updateDataTableViews);

    //Set MVC in delegate
    m_pRawDelegate->setModelView(m_pMainWindow->m_pEventWindow->getEventModel(),
                                 m_pMainWindow->m_pEventWindow->getEventTableView(),
                                 ui->m_tableView_rawTableView);

    //Set scale window in delegate
    m_pRawDelegate->setScaleWindow(m_pMainWindow->m_pScaleWindow);

    //Install event filter to overcome QGrabGesture and QScrollBar/QHeader problem
    ui->m_tableView_rawTableView->horizontalScrollBar()->installEventFilter(this);
    ui->m_tableView_rawTableView->verticalScrollBar()->installEventFilter(this);
    ui->m_tableView_rawTableView->verticalHeader()->installEventFilter(this);

    //Enable gestures for the view
    ui->m_tableView_rawTableView->grabGesture(Qt::PinchGesture);
    ui->m_tableView_rawTableView->installEventFilter(this);

    //Enable event fitlering for the viewport in order to intercept mouse events
    ui->m_tableView_rawTableView->viewport()->installEventFilter(this);

    //-----------------------------------
    //------ Init "dockable" view -------
    //-----------------------------------
    m_pUndockedDataView->setModel(m_pRawModel);
    m_pUndockedDataView->setItemDelegate(m_pRawDelegate);

    m_pUndockedDataView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_iDefaultPlotHeight);
    m_pUndockedDataView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    m_pUndockedDataView->resizeColumnsToContents();

    m_pUndockedDataView->setAutoScroll(false);
    m_pUndockedDataView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_pUndockedDataView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pUndockedDataView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pUndockedDataView->setShowGrid(false);
    m_pUndockedDataView->horizontalHeader()->setVisible(false);

    m_pUndockedDataView->verticalScrollBar()->setVisible(false);
    m_pUndockedDataView->horizontalScrollBar()->setVisible(false);

    //connect QScrollBar with model in order to reload data samples
    connect(m_pUndockedDataView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pRawModel,&RawModel::updateScrollPos);

    //---------------------------------------------------------
    //--------- Interconnect docked and undocked view ---------
    //---------------------------------------------------------
    //Scrolling ui->m_tableView_rawTableView ---> m_pUndockedDataView
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            m_pUndockedDataView->horizontalScrollBar(),&QScrollBar::setValue);
    connect(ui->m_tableView_rawTableView->verticalScrollBar(),&QScrollBar::valueChanged,
            m_pUndockedDataView->verticalScrollBar(),&QScrollBar::setValue);

    //Selection ui->m_tableView_rawTableView ---> m_pUndockedDataView
    connect(ui->m_tableView_rawTableView,&QTableView::clicked,
            m_pUndockedDataView,&QTableView::setCurrentIndex);
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

    //Add DC removal action
    m_pRemoveDCAction = new QAction(QIcon(":/Resources/Images/removeDC.png"),tr("Remove DC component"), this);
    m_pRemoveDCAction->setStatusTip(tr("Remove the DC component by subtracting the mean"));
    connect(m_pRemoveDCAction,&QAction::triggered, [=](){
        if(m_pRawDelegate->m_bRemoveDC) {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/removeDC.png"));
            m_pRawDelegate->m_bRemoveDC = false;
        }
        else {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/addDC.png"));
            m_pRawDelegate->m_bRemoveDC = true;
        }

        updateDataTableViews();
    });
    toolBar->addAction(m_pRemoveDCAction);

    toolBar->addSeparator();

    //undock view into new window (not dock widget)
    QAction* undockToWindowAction = new QAction(QIcon(":/Resources/Images/undockView.png"),tr("Undock data view"), this);
    undockToWindowAction->setStatusTip(tr("Undock data view to window"));
    connect(undockToWindowAction, SIGNAL(triggered()), this, SLOT(undockDataViewToWindow()));
    toolBar->addAction(undockToWindowAction);

    //Toggle visibility of the event manager
    QAction* showEventManager = new QAction(QIcon(":/Resources/Images/showEventManager.png"),tr("Toggle event manager"), this);
    showEventManager->setStatusTip(tr("Toggle the event manager"));
    connect(showEventManager, &QAction::triggered, m_pMainWindow, &MainWindow::showEventWindow);
    toolBar->addAction(showEventManager);

    //Toggle visibility of the Selection manager
    QAction* showSelectionManager = new QAction(QIcon(":/Resources/Images/showSelectionManager.png"),tr("Toggle selection manager"), this);
    showSelectionManager->setStatusTip(tr("Toggle the selection manager"));
    connect(showSelectionManager, &QAction::triggered, m_pMainWindow, &MainWindow::showSelectionManagerWindow);
    toolBar->addAction(showSelectionManager);

    //Toggle visibility of the scaling window
    QAction* showScalingWindow = new QAction(QIcon(":/Resources/Images/showScalingWindow.png"),tr("Toggle scaling window"), this);
    showScalingWindow->setStatusTip(tr("Toggle the scaling window"));
    connect(showScalingWindow, &QAction::triggered, m_pMainWindow, &MainWindow::showScaleWindow);
    toolBar->addAction(showScalingWindow);

    //Toggle visibility of the average manager
    QAction* showAverageManager = new QAction(QIcon(":/Resources/Images/showAverageManager.png"),tr("Toggle average manager"), this);
    showAverageManager->setStatusTip(tr("Toggle the average manager"));
    connect(showAverageManager, &QAction::triggered, m_pMainWindow, &MainWindow::showAverageWindow);
    toolBar->addAction(showAverageManager);

    //Toggle visibility of the scaling window
    QAction* showInformationWindow = new QAction(QIcon(":/Resources/Images/showInformationWindow.png"),tr("Toggle information window"), this);
    showInformationWindow->setStatusTip(tr("Toggle the information window"));
    connect(showInformationWindow, &QAction::triggered, m_pMainWindow, &MainWindow::showInformationWindow);
    toolBar->addAction(showInformationWindow);

    int layoutRows = ui->m_gridLayout->rowCount();
    int layoutColumns = ui->m_gridLayout->columnCount();

    ui->m_gridLayout->addWidget(toolBar, 1, layoutColumns, layoutRows-1, 1);
}


//*************************************************************************************************************

void DataWindow::initMarker()
{
    //Set marker as front top-most widget
    m_pDataMarker->raise();

    //Get boundary rect coordinates for table view
    double boundingLeft = ui->m_tableView_rawTableView->verticalHeader()->geometry().right() + ui->m_tableView_rawTableView->geometry().left();
    double boundingRight = ui->m_tableView_rawTableView->geometry().right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1;
    QRect boundingRect;
    boundingRect.setLeft(boundingLeft);
    boundingRect.setRight(boundingRight);

    //Inital position of the marker
    m_pDataMarker->move(boundingRect.x(), boundingRect.y() + 1);

    //Create Region from bounding rect - this region is used to restrain the marker inside the data view
    QRegion region(boundingRect);
    m_pDataMarker->setMovementBoundary(region);

    //Set marker size to table view size minus horizontal scroll bar height
    m_pDataMarker->resize(DATA_MARKER_WIDTH,
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);

    //Connect current marker to marker move signal
    connect(m_pDataMarker,&DataMarker::markerMoved,
            this,&DataWindow::updateMarkerPosition);

    //If no file has been loaded yet dont show the marker and its label
    if(!m_pRawModel->m_bFileloaded) {
        m_pDataMarker->hide();
        m_pCurrentDataMarkerLabel->hide();
        ui->m_label_sampleMin->hide();
        ui->m_label_sampleMax->hide();
    }

    //Connect current marker to loading a fiff file - no loaded file - no visible marker
    connect(m_pRawModel, &RawModel::fileLoaded,[this](){
        bool state = m_pRawModel->m_bFileloaded;
        if(state) {
            //Inital position of the marker
            m_pDataMarker->move(74, m_pDataMarker->y());
            m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (DATA_MARKER_WIDTH/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);

            m_pDataMarker->show();
            m_pCurrentDataMarkerLabel->show();

            ui->m_label_sampleMin->show();
            ui->m_label_sampleMax->show();
        }
        else {
            m_pDataMarker->hide();
            m_pCurrentDataMarkerLabel->hide();

            ui->m_label_sampleMin->hide();
            ui->m_label_sampleMax->hide();
        }
    });
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
    m_pCurrentDataMarkerLabel->resize(150, m_pCurrentDataMarkerLabel->height());
    m_pCurrentDataMarkerLabel->setAlignment(Qt::AlignHCenter);
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left(), m_pDataMarker->geometry().top() + 5);
    QString numberString = QString().number(m_iCurrentMarkerSample);
    m_pCurrentDataMarkerLabel->setText(numberString.append(QString(" / %1").arg("0 sec")));

    //Set color
    QPalette palette;
    QColor textColor = m_qSettings.value("DataMarker/data_marker_color", QColor (227,6,19)).value<QColor>();
    textColor.setAlpha(DATA_MARKER_OPACITY);
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

bool DataWindow::eventFilter(QObject *object, QEvent *event)
{    
    //Detect double mouse clicks and move data marker to current mouse position
    if (object == ui->m_tableView_rawTableView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEventCast = static_cast<QMouseEvent*>(event);
        if(mouseEventCast->button() == Qt::LeftButton)
            m_pDataMarker->move(mouseEventCast->localPos().x() + ui->m_tableView_rawTableView->verticalHeader()->width() + ui->m_tableView_rawTableView->x(), m_pDataMarker->y());

        return true;
    }

    //Deactivate grabbing gesture when scrollbars or vertical header are selected
    if ((object == m_pUndockedDataView->horizontalScrollBar() || object == ui->m_tableView_rawTableView->horizontalScrollBar() ||
         object == m_pUndockedDataView->verticalScrollBar() || object == ui->m_tableView_rawTableView->verticalScrollBar() ||
         object == m_pUndockedDataView->verticalHeader() || object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Enter) {
        QScroller::ungrabGesture(m_pUndockedDataView);
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
        return true;
    }

    //Activate grabbing gesture when scrollbars or vertical header are deselected
    if ((object == m_pUndockedDataView->horizontalScrollBar() || object == ui->m_tableView_rawTableView->horizontalScrollBar()||
         object == m_pUndockedDataView->verticalScrollBar() || object == ui->m_tableView_rawTableView->verticalScrollBar()||
         object == m_pUndockedDataView->verticalHeader() || object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Leave) {
        QScroller::grabGesture(m_pUndockedDataView, QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);
        return true;
    }

    //Look for swipe gesture in order to scale the channels
    if (object == ui->m_tableView_rawTableView && event->type() == QEvent::Gesture) {
        QGestureEvent* gestureEventCast = static_cast<QGestureEvent*>(event);
        return gestureEvent(static_cast<QGestureEvent*>(gestureEventCast));
    }

    return false;
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

    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (DATA_MARKER_WIDTH/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);
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

    double boundingLeft = ui->m_tableView_rawTableView->verticalHeader()->geometry().right() + ui->m_tableView_rawTableView->geometry().left();
    double boundingRight = ui->m_tableView_rawTableView->geometry().right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1;
    boundingRect.setLeft(boundingLeft);
    boundingRect.setRight(boundingRight);

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
    m_pDataMarker->resize(DATA_MARKER_WIDTH,
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);

    //Update current marker sample lable
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);
}


//*************************************************************************************************************

void DataWindow::highlightChannelsInSelectionManager()
{
    if(m_pMainWindow->m_pSelectionManagerWindow->isVisible()) {
        QStringList currentSelectedChannelNames;

        QModelIndexList selectedIndexes = ui->m_tableView_rawTableView->selectionModel()->selectedIndexes();

        for(int i = 0; i<selectedIndexes.size(); i++) {
            QModelIndex index = m_pRawModel->index(selectedIndexes.at(i).row(), 0);
            currentSelectedChannelNames << m_pRawModel->data(index).toString();
        }

        m_pMainWindow->m_pSelectionManagerWindow->highlightChannels(currentSelectedChannelNames);
    }
}


//*************************************************************************************************************

bool DataWindow::gestureEvent(QGestureEvent *event)
{
    //Pinch event
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));

    return true;
}


//*************************************************************************************************************

bool DataWindow::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        emit scaleChannels(gesture->scaleFactor());
        qDebug()<<"ungrab";
        ui->m_tableView_rawTableView->setSelectionMode(QAbstractItemView::NoSelection);
        QScroller::ungrabGesture(m_pUndockedDataView);
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
    }

    if (gesture->state() == Qt::GestureFinished) {
        qDebug()<<"Finished gesture - grab again";
        ui->m_tableView_rawTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        QScroller::grabGesture(m_pUndockedDataView, QScroller::LeftMouseButtonGesture);
        QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);
    }

    return true;
}
