//=============================================================================================================
/**
 * @file     fiffrawview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of FiffRawView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math.h>
#include "fiffrawview.h"
#include "fiffrawviewdelegate.h"

#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/eventmodel.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QTableView>
#include <QVBoxLayout>
#include <QDebug>
#include <QHeaderView>
#include <QScrollBar>
#include <QDate>
#include <QDir>
#include <QSvgGenerator>
#include <QAbstractScrollArea>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>

#if !defined(NO_QOPENGLWIDGET)
    #include <QOpenGLWidget>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawView::FiffRawView(QWidget *parent)
: DISPLIB::AbstractView(parent)
, m_pTableView(new QTableView())
, m_fDefaultSectionSize(80.0f)
{

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0, 0, 0, 0);
    neLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(neLayout);

    //Create position labels
    createBottomLabels();

    //Create Right-click Context menu
    initRightClickContextMenu();

    m_pTableView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    //m_pTableView->horizontalScrollBar()->setRange(0, m_pTableView->horizontalScrollBar()->maximum() / 1000000);
    //m_pTableView->setShowGrid(true);

    connect(m_pTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &FiffRawView::updateTimeLabels, Qt::UniqueConnection);
}

//=============================================================================================================

void FiffRawView::reset()
{
    if(m_pModel){
        disconnectModel();

        m_pTableView->setModel(Q_NULLPTR);
        m_pModel = Q_NULLPTR;
    }
}

//=============================================================================================================

void FiffRawView::setDelegate(const QSharedPointer<FiffRawViewDelegate>& pDelegate)
{
    if(!pDelegate) {
        qWarning() << "[FiffRawView::setDelegate] Passed delegate is NULL.";
        return;
    }
    m_pDelegate = pDelegate;

    m_pTableView->setItemDelegate(m_pDelegate.data());
}

//=============================================================================================================

QSharedPointer<FiffRawViewDelegate> FiffRawView::getDelegate()
{
    return m_pDelegate;
}

//=============================================================================================================

QSharedPointer<FiffRawViewModel> FiffRawView::getModel()
{
    return m_pModel;
}

//=============================================================================================================

void FiffRawView::setModel(const QSharedPointer<FiffRawViewModel>& pModel)
{
    if(!pModel) {
        qWarning() << "[FiffRawView::setModel] Passed model is NULL.";
        return;
    }
    disconnectModel();

    m_pModel = pModel;

    m_pTableView->setModel(m_pModel.data());

    #if !defined(NO_QOPENGLWIDGET)
    m_pTableView->setViewport(new QOpenGLWidget);
    #endif

    m_pTableView->setObjectName(QString::fromUtf8("m_pTableView"));
    QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(m_pTableView->sizePolicy().hasHeightForWidth());
    m_pTableView->setSizePolicy(sizePolicy3);
    m_pTableView->setMinimumSize(QSize(0, 0));
    m_pTableView->setMouseTracking(false);
    m_pTableView->setAutoScroll(false);
    m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pTableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_pTableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pTableView->setShowGrid(false);
    m_pTableView->horizontalHeader()->setVisible(false);

    //m_pTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_iDefaultPlotHeight);
    m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    m_pTableView->setColumnHidden(2,true); //because we do not want to plot the mean values

    //Install event filter to overcome QGrabGesture and QScrollBar/QHeader problem
    m_pTableView->horizontalScrollBar()->installEventFilter(this);
    m_pTableView->verticalScrollBar()->installEventFilter(this);
    m_pTableView->verticalHeader()->installEventFilter(this);

    //Enable gestures for the view
    m_pTableView->grabGesture(Qt::PinchGesture);
    m_pTableView->installEventFilter(this);

    QScroller::grabGesture(m_pTableView, QScroller::LeftMouseButtonGesture);
    m_pKineticScroller = QScroller::scroller(m_pTableView);
    m_pKineticScroller->setSnapPositionsX(100,100);

    //Enable event fitlering for the viewport in order to intercept mouse events
    m_pTableView->viewport()->installEventFilter(this);

    // Connect QScrollBar with model in order to reload data samples
    connect(m_pTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_pModel.data(), &FiffRawViewModel::updateHorizontalScrollPosition, Qt::UniqueConnection);

    connect(m_pTableView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &FiffRawView::updateVerticalScrollPosition, Qt::UniqueConnection);

    // Connect and init resizing of the table view to the MVC
    connect(this, &FiffRawView::tableViewDataWidthChanged,
            m_pModel.data(), &FiffRawViewModel::setDataColumnWidth, Qt::UniqueConnection);

    pModel->setDataColumnWidth(m_pTableView->width()-m_pTableView->columnWidth(0));
    m_pTableView->resizeColumnsToContents();

//    m_pTableView->setColumnHidden(0,true);
//    m_pTableView->setColumnHidden(2,true);
//    m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
//    m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); //Stretch 2 column to maximal width
//    m_pTableView->horizontalHeader()->hide();
//    m_pTableView->resizeColumnsToContents();
//    m_pTableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
//    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_iT = 10;

    m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pTableView.data(), &QWidget::customContextMenuRequested,
            this, &FiffRawView::customContextMenuRequested, Qt::UniqueConnection);

    connect(m_pModel.data(), &FiffRawViewModel::newRealtimeData,
            this, &FiffRawView::onNewRealtimeData, Qt::UniqueConnection);

//    //Gestures
//    m_pTableView->grabGesture(Qt::PinchGesture);
//    m_pTableView->grabGesture(Qt::TapAndHoldGesture);
    updateTimeLabels(0);
    updateFileLabel();
    updateFilterLabel();
}

//=============================================================================================================

void FiffRawView::resizeEvent(QResizeEvent * event)
{
    if(m_pTableView) {
        emit tableViewDataWidthChanged(m_pTableView->width()-m_pTableView->columnWidth(0));
        m_pTableView->resizeColumnsToContents();
        setWindowSize(m_iT);
        setZoom(m_fZoomFactor);
    }

    return QWidget::resizeEvent(event);
}

//=============================================================================================================

void FiffRawView::setScalingMap(const QMap<qint32, float>& scaleMap)
{
    if(!m_pModel) {
        return;
    }

    m_qMapChScaling = scaleMap;
    m_qMapChScaling.detach();
    m_pModel->setScaling(m_qMapChScaling);
}

//=============================================================================================================

void FiffRawView::setSignalColor(const QColor& signalColor)
{
    m_pDelegate->setSignalColor(signalColor);
}

//=============================================================================================================

void FiffRawView::setBackgroundColor(const QColor& backgroundColor)
{
    if(!m_pModel) {
        return;
    }

    m_pModel->setBackgroundColor(backgroundColor);
}

//=============================================================================================================

void FiffRawView::setZoom(double dZoomFac)
{
    m_fZoomFactor = dZoomFac;

    m_pTableView->verticalHeader()->setDefaultSectionSize(m_pTableView->height() / m_fZoomFactor/**m_fDefaultSectionSize*/);//Row Height

    QFont font = m_pTableView->font();
    font.setPointSize(std::min((m_pTableView->height() / m_fZoomFactor) / 4.,  12.));
    m_pTableView->setFont(font);

    updateView();
}

//=============================================================================================================

void FiffRawView::setWindowSize(int iT)
{
    if(!m_pModel) {
        return;
    }

    int iNewPos;
    iNewPos = ((m_pTableView->horizontalScrollBar()->value() * m_iT) / iT);

    if (iNewPos < 0) {
        iNewPos = m_pTableView->horizontalScrollBar()->value();
    }

    m_iT = iT;

    m_pModel->setWindowSize(iT,
                            m_pTableView->width() - (m_pTableView->verticalHeader()->width()/* + m_pTableView->verticalScrollBar()->width()*/) /*- m_pTableView->columnWidth(0)*/,
                            iNewPos);

    m_pTableView->resizeRowsToContents();
    m_pTableView->resizeColumnsToContents();

    m_pTableView->horizontalScrollBar()->setValue(iNewPos);
    m_pTableView->viewport()->repaint();
    updateTimeLabels(m_pTableView->horizontalScrollBar()->value());
}

//=============================================================================================================

void FiffRawView::setDistanceTimeSpacer(int iValue)
{
    if(!m_pModel) {
        return;
    }

    m_pModel->distanceTimeSpacerChanged(iValue);

    //Wiggle view area to trigger update (there's probably a better way of doing this)
    m_pTableView->horizontalScrollBar()->setValue(m_pTableView->horizontalScrollBar()->value()+1);
    m_pTableView->horizontalScrollBar()->setValue(m_pTableView->horizontalScrollBar()->value()-1);
}

//=============================================================================================================

void FiffRawView::onMakeScreenshot(const QString& imageType)
{
    // Create file name
    QString fileName;
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    if(imageType.contains("SVG")) {
        fileName = QString("./Screenshots/%1-%2-AnalyzeDataView.svg").arg(sDate).arg(sTime);

        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        svgGen.setSize(m_pTableView->size());
        svgGen.setViewBox(m_pTableView->rect());

        m_pTableView->render(&svgGen);
        qInfo() << "[FiffRawView::onMakeScreenshot] Saving SVG Screenshot";
    } else if(imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-AnalyzeDataView.png").arg(sDate).arg(sTime);

        QPixmap pixMap = m_pTableView->grab();
        pixMap.save(fileName);
        qInfo() << "[FiffRawView::onMakeScreenshot] Saving PNG Screenshot";
    }
}

//=============================================================================================================

void FiffRawView::initRightClickContextMenu()
{
    m_pRightClickContextMenu = new QMenu(this);

    m_pAddEventAction = m_pRightClickContextMenu->addAction(tr("Add Event"));
    connect(m_pAddEventAction, &QAction::triggered,
            this, &FiffRawView::addTimeMark, Qt::UniqueConnection);
}

//=============================================================================================================

void FiffRawView::customContextMenuRequested(const QPoint &pos)
{
    if(!m_pModel || m_pModel->isEmpty()) {
        return;
    }

    int iFirstSampleOffset = m_pModel->absoluteFirstSample();
    int iScrollBarOffset = static_cast<int>(std::round(m_pTableView->horizontalScrollBar()->value() / m_pModel->pixelDifference()));
    int iMouseOffset = static_cast<int>(std::round(pos.x() / m_pModel->pixelDifference()));

    m_iLastClickedSample = iFirstSampleOffset + iScrollBarOffset + iMouseOffset;

    m_pRightClickContextMenu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}

//=============================================================================================================

void FiffRawView::addTimeMark(bool con)
{
    Q_UNUSED(con);

    if(!m_pModel) {
        return;
    }

    emit sendSamplePos(m_iLastClickedSample);
}

//=============================================================================================================

void FiffRawView::toggleDisplayEvent(const int& iToggle)
{
    if(!m_pModel) {
        return;
    }

    int m_iToggle = iToggle;

    m_pModel->toggleDispEvent(m_iToggle);
    m_pTableView->viewport()->repaint();
}

//=============================================================================================================

void FiffRawView::updateView()
{
    m_pTableView->viewport()->repaint();
}

//=============================================================================================================

bool FiffRawView::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_pTableView->horizontalScrollBar() ||
         object == m_pTableView->verticalScrollBar() ||
         object == m_pTableView->verticalHeader())
        && event->type() == QEvent::Enter) {
        QScroller::ungrabGesture(m_pTableView);
        return true;
    } else

    //Activate grabbing gesture when scrollbars or vertical header are deselected
    if ((object == m_pTableView->horizontalScrollBar() ||
         object == m_pTableView->verticalScrollBar() ||
         object == m_pTableView->verticalHeader())
        && event->type() == QEvent::Leave) {
        QScroller::grabGesture(m_pTableView, QScroller::LeftMouseButtonGesture);
        return true;
    } else

    if(event->type() == QEvent::KeyPress){
        qDebug() << "Key Press";
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch(keyEvent->key()){
        case Qt::Key_E :{
            if(m_pModel){
                m_iLastClickedSample = m_pModel->getScrollerPosition();
                addTimeMark(true);
            }
        }
        }
    }

    if(event->type() == QEvent::MouseButtonPress){
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        switch(mouseEvent->button()){
        case Qt::LeftButton:{
            if(m_pModel && object == m_pTableView->viewport()){
            QPoint pos = mouseEvent->pos();
            m_pModel->setScrollerSample(static_cast<int>(floor((float)m_pModel->absoluteFirstSample() + //accounting for first sample offset
                                     (m_pTableView->horizontalScrollBar()->value() / m_pModel->pixelDifference()) + //accounting for scroll offset
                                     ((float)pos.x() / m_pModel->pixelDifference())))); //accounting for mouse position offset
            m_pTableView->viewport()->repaint();
            }
        }
        }

    }


    return QWidget::eventFilter(object, event);
}

//=============================================================================================================

void FiffRawView::updateScrollPositionToEvent()
{
    if(!m_pModel) {
        return;
    }

    auto events = m_pModel->getEventModel()->getEventsToDisplay(0, m_pModel->absoluteLastSample());
    int iSample = events->at(m_pModel->getEventModel()->getEventSelection().front()).sample - m_pModel->absoluteFirstSample();
    double dDx = m_pModel->pixelDifference();

    //qDebug() << "Div:" << iSample * dDx;
    int iPos = static_cast<int>(iSample * dDx) - static_cast<int>(m_pTableView->width() / 2);
    if(iPos < 0){
        iPos = 0;
    }
    //qDebug() << "Current scroll:" << m_pTableView->horizontalScrollBar()->value();

    m_pTableView->horizontalScrollBar()->setValue(iPos);
}

//=============================================================================================================

void FiffRawView::setFilter(const FilterKernel& filterData)
{
    if(!m_pModel) {
        return;
    }

    m_pModel->setFilter(filterData);
    updateFilterLabel();
}

//=============================================================================================================

void FiffRawView::setFilterActive(bool state)
{
    if(!m_pModel) {
        return;
    }
    m_pModel->setFilterActive(state);
    updateFilterLabel();
}

//=============================================================================================================

void FiffRawView::setFilterChannelType(const QString &channelType)
{
    if(!m_pModel) {
        return;
    }

    m_pModel->setFilterChannelType(channelType);
}

//=============================================================================================================

void FiffRawView::createBottomLabels()
{
    QHBoxLayout *LabelLayout = new QHBoxLayout(this);
    QWidget* labelBar = new QWidget(this);

    m_pInitialTimeLabel = new QLabel(this);
    m_pInitialTimeLabel->setText(" ");
    m_pInitialTimeLabel->setAlignment(Qt::AlignLeft);

    m_pEndTimeLabel = new QLabel(this);
    m_pEndTimeLabel->setText(" ");
    m_pEndTimeLabel->setAlignment(Qt::AlignRight);

    m_pFileLabel = new QLabel(this);
    m_pFileLabel->setText(" ");
    m_pFileLabel->setAlignment(Qt::AlignRight);

    m_pFilterLabel = new QLabel(this);
    m_pFilterLabel->setText(" ");
    m_pFilterLabel->setAlignment(Qt::AlignRight);

    LabelLayout->addWidget(m_pInitialTimeLabel);
    LabelLayout->addWidget(m_pFileLabel);
    LabelLayout->addWidget(m_pFilterLabel);
    LabelLayout->addWidget(m_pEndTimeLabel);
    labelBar->setLayout(LabelLayout);

    this->layout()->addWidget(labelBar);

    m_pInitialTimeLabel->show();
    m_pEndTimeLabel->show();
    labelBar->show();
}

//=============================================================================================================

void FiffRawView::updateTimeLabels(int iValue)
{
    Q_UNUSED(iValue);

    if(m_pModel->isEmpty()) {
        m_pEndTimeLabel->setText("0 | 0 sec");
        m_pInitialTimeLabel->setText("0 | 0 sec");
        return;
    }

    QString strLeft, strRight;
    //Left Label
    int iSample = static_cast<int>(m_pTableView->horizontalScrollBar()->value() / m_pModel->pixelDifference());
    strLeft = QString("%1 | %2 sec").arg(QString().number(iSample)).arg(QString().number(iSample / m_pModel->getFiffInfo()->sfreq, 'f', 2));
    m_pInitialTimeLabel->setText(strLeft);

    //Right Label
    iSample += m_iT * m_pModel->getFiffInfo()->sfreq;
    strRight = QString("%1 | %2 sec").arg(QString().number(iSample)).arg(QString().number(iSample / m_pModel->getFiffInfo()->sfreq, 'f', 2));
    m_pEndTimeLabel->setText(strRight);
}


//=============================================================================================================

void FiffRawView::disconnectModel()
{
    // Disconnect QScrollBar with model
    disconnect(m_pTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
               m_pModel.data(), &FiffRawViewModel::updateHorizontalScrollPosition);

    disconnect(m_pTableView->verticalScrollBar(), &QScrollBar::valueChanged,
               this, &FiffRawView::updateVerticalScrollPosition);

    // Disconnect resizing of the table view to the MVC
    disconnect(this, &FiffRawView::tableViewDataWidthChanged,
            m_pModel.data(), &FiffRawViewModel::setDataColumnWidth);

    disconnect(m_pModel.data(), &FiffRawViewModel::newRealtimeData,
               this, &FiffRawView::onNewRealtimeData);
}

//=============================================================================================================

void FiffRawView::updateVerticalScrollPosition(qint32 newScrollPosition)
{
    Q_UNUSED(newScrollPosition);
    if(FiffRawViewDelegate *pDelegate = qobject_cast<FiffRawViewDelegate *>(m_pTableView->itemDelegate())) {
        pDelegate->setUpperItemIndex(m_pTableView->rowAt(0));
    }
}

//=============================================================================================================

void FiffRawView::saveSettings()
{
}

//=============================================================================================================

void FiffRawView::loadSettings()
{
}

//=============================================================================================================

void FiffRawView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void FiffRawView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void FiffRawView::showSelectedChannelsOnly(const QList<int> selectedChannelsIndexes)
{
    if (selectedChannelsIndexes.contains(-1)){
        for(int i = 0; i<m_pModel->rowCount(); i++) {
            m_pTableView->showRow(i);
        }
    } else {
        for(int i = 0; i<m_pModel->rowCount(); i++) {
            if (selectedChannelsIndexes.contains(i)){
                m_pTableView->showRow(i);
            } else {
                m_pTableView->hideRow(i);
            }
        }
    }

    if(FiffRawViewDelegate *pDelegate = qobject_cast<FiffRawViewDelegate *>(m_pTableView->itemDelegate())) {
        pDelegate->setUpperItemIndex(m_pTableView->rowAt(0));
    }
}

//=============================================================================================================

void FiffRawView::showAllChannels()
{
    for(int i = 0; i<m_pModel->rowCount(); i++) {
        m_pTableView->showRow(i);
    }

    if(FiffRawViewDelegate *pDelegate = qobject_cast<FiffRawViewDelegate *>(m_pTableView->itemDelegate())) {
        pDelegate->setUpperItemIndex(m_pTableView->rowAt(0));
    }
}

//=============================================================================================================

void FiffRawView::clearView()
{
    if(parentWidget())
    {
        parentWidget()->setWindowTitle("Signal Viewer");
    }
    reset();
}

//=============================================================================================================

void FiffRawView::updateFileLabel()
{
    if(parentWidget())
    {
        QString label;

        float fFrequency = m_pModel->getSamplingFrequency();

        int iFileLengthInSamples = m_pModel->absoluteLastSample() - m_pModel->absoluteFirstSample();
        float fFileLengthInSeconds = static_cast<float>(iFileLengthInSamples) / fFrequency;

        if(m_pModel->isRealtime()){
            label += "   |   MNE Scan Session";
        } else {
            label += "   |   " + m_pModel->getModelName();
        }
        label += "  -  Sampling Freq. " + QString::number(m_pModel->getSamplingFrequency(),'g',5) + "Hz";
        label += "  -  Length: " + QString::number(fFileLengthInSeconds,'g',5) + "s.";
        m_pFileLabel->setText(label);
    }
}

//=============================================================================================================

void FiffRawView::updateFilterLabel()
{
    QString label;
    if(m_pModel->isFilterActive())
    {
        label += "   |   Filter ON";
    } else
    {
        label += "   |   Filter OFF";
    }
    label += "  -  " + m_pModel->getFilter().getShortDescription() + "  |";
    m_pFilterLabel->setText(label);
}

//=============================================================================================================

void FiffRawView::onNewRealtimeData()
{
    updateView();

    updateTimeLabels(0);
    updateFileLabel();
    updateFilterLabel();

    emit realtimeDataUpdated();
}
