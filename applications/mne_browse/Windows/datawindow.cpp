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
* @brief    Definition of the DataWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datawindow.h"

#include <QGLWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


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
, m_bHideBadChannels(false)
, m_pRawDelegate(Q_NULLPTR)
, m_pKineticScroller(Q_NULLPTR)
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

void DataWindow::scaleData(const QMap<QString,double> &scaleMap)
{
    m_pRawDelegate->setScaleMap(scaleMap);
    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::updateDataTableViews()
{
    ui->m_tableView_rawTableView->viewport()->update();
}


//*************************************************************************************************************

void DataWindow::showSelectedChannelsOnly(QStringList selectedChannels)
{
    m_slSelectedChannels = selectedChannels;

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pRawModel->rowCount(); i++) {
        QString channel = m_pRawModel->data(m_pRawModel->index(i, 0), Qt::DisplayRole).toString();
        QVariant v = m_pRawModel->data(m_pRawModel->index(i,1), Qt::BackgroundRole);

        if(!selectedChannels.contains(channel))
            ui->m_tableView_rawTableView->hideRow(i);
        else
            ui->m_tableView_rawTableView->showRow(i);

        //if channel is a bad channel and bad channels are to be hidden -> do not show
        if(v.canConvert<QBrush>() && m_bHideBadChannels)
            ui->m_tableView_rawTableView->hideRow(i);
    }

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::changeRowHeight(int height)
{
    for(int i = 0; i<ui->m_tableView_rawTableView->verticalHeader()->count(); i++)
        ui->m_tableView_rawTableView->setRowHeight(i, height);

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::hideBadChannels(bool hideChannels)
{
    m_bHideBadChannels = hideChannels;

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pRawModel->rowCount(); i++) {
        QVariant v = m_pRawModel->data(m_pRawModel->index(i,1),Qt::BackgroundRole);
        QString chName = m_pRawModel->data(m_pRawModel->index(i,0),Qt::DisplayRole).toString();

        //Check if channel is marked as bad
        if(v.canConvert<QBrush>() && m_bHideBadChannels)
            ui->m_tableView_rawTableView->hideRow(i);
        else if(m_slSelectedChannels.contains(chName))
            ui->m_tableView_rawTableView->showRow(i);
    }

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::initMVCSettings()
{
    //-----------------------------------
    //------ Init data window view ------
    //-----------------------------------
    //Set MVC model
    ui->m_tableView_rawTableView->setModel(m_pRawModel);

    //Use GPU rendering
    QGLFormat currentFormat = QGLFormat(QGL::SampleBuffers);
    currentFormat.setSamples(6);
    QGLWidget* pGLWidget = new QGLWidget(currentFormat);
    ui->m_tableView_rawTableView->setViewport(pGLWidget);

    //Set MVC delegate
    m_pRawDelegate = new RawDelegate(this);
    ui->m_tableView_rawTableView->setItemDelegate(m_pRawDelegate);

    //set some settings for m_pRawTableView
    ui->m_tableView_rawTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_iDefaultPlotHeight);
    ui->m_tableView_rawTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    ui->m_tableView_rawTableView->setColumnHidden(2,true); //because we do not want to plot the mean values
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
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_pRawModel, &RawModel::updateScrollPos);

    //connect selection of a channel to selection manager
    connect(ui->m_tableView_rawTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DataWindow::highlightChannelsInSelectionManager);

    //connect selection change to update data views
    connect(ui->m_tableView_rawTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DataWindow::updateDataTableViews);

    //Set MVC in delegate
    m_pRawDelegate->setModelView(m_pMainWindow->m_pEventWindow->getEventModel(),
                                 m_pMainWindow->m_pEventWindow->getEventTableView(),
                                 ui->m_tableView_rawTableView);

    //Install event filter to overcome QGrabGesture and QScrollBar/QHeader problem
    ui->m_tableView_rawTableView->horizontalScrollBar()->installEventFilter(this);
    ui->m_tableView_rawTableView->verticalScrollBar()->installEventFilter(this);
    ui->m_tableView_rawTableView->verticalHeader()->installEventFilter(this);

    //Enable gestures for the view
    ui->m_tableView_rawTableView->grabGesture(Qt::PinchGesture);
    ui->m_tableView_rawTableView->installEventFilter(this);

    //Enable event fitlering for the viewport in order to intercept mouse events
    ui->m_tableView_rawTableView->viewport()->installEventFilter(this);
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
    QColor textColor = m_qSettings.value("DataMarker/data_marker_color", QColor(93,177,47)).value<QColor>();
    //textColor.setAlpha(DATA_MARKER_OPACITY);
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

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_D))
        ui->m_tableView_rawTableView->clearSelection();

    return QWidget::keyPressEvent(event);
}


//*************************************************************************************************************

bool DataWindow::eventFilter(QObject *object, QEvent *event)
{    
    //Detect double mouse clicks and move data marker to current mouse position
    if (object == ui->m_tableView_rawTableView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEventCast = static_cast<QMouseEvent*>(event);
        if(mouseEventCast->button() == Qt::LeftButton) {
            m_pDataMarker->move(mouseEventCast->localPos().x() + ui->m_tableView_rawTableView->verticalHeader()->width() + ui->m_tableView_rawTableView->x(), m_pDataMarker->y());

            //Deselect channel which was selected through the double click -> dirty hack
            //ui->m_tableView_rawTableView->selectionModel()->select(ui->m_tableView_rawTableView->selectionModel()->currentIndex(), QItemSelectionModel::Deselect);
        }
    }

    //Deactivate grabbing gesture when scrollbars or vertical header are selected
    if ((object == ui->m_tableView_rawTableView->horizontalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Enter) {
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
        return true;
    }

    //Activate grabbing gesture when scrollbars or vertical header are deselected
    if ((object == ui->m_tableView_rawTableView->horizontalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Leave) {
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
    int minSampleRangeSec = (minSampleRange/m_pRawModel->m_pFiffInfo->sfreq)*1000;
    ui->m_label_sampleMin->setText(QString("%1 / %2 sec").arg(stringTemp.number(minSampleRange)).arg(stringTemp.number((double)minSampleRangeSec/1000,'g')));
    int maxSampleRangeSec = (maxSampleRange/m_pRawModel->m_pFiffInfo->sfreq)*1000;
    ui->m_label_sampleMax->setText(QString("%1 / %2 sec").arg(stringTemp.number(maxSampleRange)).arg(stringTemp.number((double)maxSampleRangeSec/1000,'g')));
}


//*************************************************************************************************************

void DataWindow::setMarkerSampleLabel()
{
    m_pCurrentDataMarkerLabel->raise();

    //Update the text and position in the current sample marker label
    m_iCurrentMarkerSample = ui->m_tableView_rawTableView->horizontalScrollBar()->value() +
            (m_pDataMarker->geometry().x() - ui->m_tableView_rawTableView->geometry().x() - ui->m_tableView_rawTableView->verticalHeader()->width());

    int currentSeconds = (m_iCurrentMarkerSample/m_pRawModel->m_pFiffInfo->sfreq)*1000;

    QString numberString = QString("%1 / %2 sec").arg(QString().number(m_iCurrentMarkerSample)).arg(QString().number((double)currentSeconds/1000,'g'));
    m_pCurrentDataMarkerLabel->setText(numberString);

    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (DATA_MARKER_WIDTH/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - 20);

    //Set current marker posisiton in event model
    m_pMainWindow->m_pEventWindow->getEventModel()->setCurrentMarkerPos(m_iCurrentMarkerSample);
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
    if(m_pMainWindow->m_pChannelSelectionView->isVisible()) {
        QModelIndexList selectedIndexes = ui->m_tableView_rawTableView->selectionModel()->selectedIndexes();

        m_pMainWindow->m_pChannelSelectionView->highlightChannels(selectedIndexes);
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
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
    }

    if (gesture->state() == Qt::GestureFinished) {
        qDebug()<<"Finished gesture - grab again";
        ui->m_tableView_rawTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);
    }

    return true;
}
