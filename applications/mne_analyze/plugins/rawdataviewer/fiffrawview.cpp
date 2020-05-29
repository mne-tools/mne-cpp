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

#include "fiffrawview.h"
#include "fiffrawviewdelegate.h"
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/annotationmodel.h>

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

#if !defined(NO_OPENGL)
    #include <QOpenGLWidget>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawView::FiffRawView(QWidget *parent)
: QWidget(parent)
, m_fDefaultSectionSize(80.0f)
{
    m_pTableView = new QTableView;

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0, 0, 0, 0);
    neLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(neLayout);

    //Create position labels
    createLabels();

    m_pTableView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    //m_pTableView->horizontalScrollBar()->setRange(0, m_pTableView->horizontalScrollBar()->maximum() / 1000000);
    //m_pTableView->setShowGrid(true);

    connect(m_pTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &FiffRawView::updateLabels, Qt::UniqueConnection);
}

//=============================================================================================================

FiffRawView::~FiffRawView()
{
    delete m_pTableView;
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

#if !defined(NO_OPENGL)
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
            m_pModel.data(), &FiffRawViewModel::updateScrollPosition, Qt::UniqueConnection);

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

//    //Gestures
//    m_pTableView->grabGesture(Qt::PinchGesture);
//    m_pTableView->grabGesture(Qt::TapAndHoldGesture);
    updateLabels(0);
}

//=============================================================================================================

void FiffRawView::resizeEvent(QResizeEvent * event)
{
    if(m_pTableView) {
        emit tableViewDataWidthChanged(m_pTableView->width()-m_pTableView->columnWidth(0));
        m_pTableView->resizeColumnsToContents();
    }

    return QWidget::resizeEvent(event);
}

//=============================================================================================================

void FiffRawView::setScalingMap(const QMap<qint32, float>& scaleMap)
{
    m_qMapChScaling = scaleMap;
    m_pModel->setScaling(scaleMap);
}

//=============================================================================================================

void FiffRawView::setSignalColor(const QColor& signalColor)
{
    m_pDelegate->setSignalColor(signalColor);
}

//=============================================================================================================

void FiffRawView::setBackgroundColor(const QColor& backgroundColor)
{
    m_backgroundColor = backgroundColor;

    if(m_pModel) {
        m_pModel->setBackgroundColor(m_backgroundColor);
    }
}

//=============================================================================================================

void FiffRawView::setZoom(double zoomFac)
{
    m_fZoomFactor = zoomFac;

    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
}

//=============================================================================================================

void FiffRawView::setWindowSize(int T)
{
    int iNewPos;
    iNewPos = ((m_pTableView->horizontalScrollBar()->value() * m_iT) / T);

    if (iNewPos < 0) {
        iNewPos = m_pTableView->horizontalScrollBar()->value();
    }

    m_iT = T;

    m_pModel->setWindowSize(T,
                            m_pTableView->width() - m_pTableView->verticalHeader()->width()/*- m_pTableView->columnWidth(0)*/,
                            iNewPos);

    m_pTableView->resizeRowsToContents();
    m_pTableView->resizeColumnsToContents();

    m_pTableView->horizontalScrollBar()->setValue(iNewPos);
    m_pTableView->viewport()->repaint();
    updateLabels(m_pTableView->horizontalScrollBar()->value());
}

//=============================================================================================================

void FiffRawView::setDistanceTimeSpacer(int value)
{
    m_pModel->distanceTimeSpacerChanged(value);

    //Wiggle view area to trigger update (there's probably a better way of doing this)
    m_pTableView->horizontalScrollBar()->setValue(m_pTableView->horizontalScrollBar()->value()+1);
    m_pTableView->horizontalScrollBar()->setValue(m_pTableView->horizontalScrollBar()->value()-1);
}

//=============================================================================================================

void FiffRawView::onMakeScreenshot(const QString& imageType)
{
    qInfo() << "[FiffRawView::onMakeScreenshot] Saving Screenshot";
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
    } else if(imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-AnalyzeDataView.png").arg(sDate).arg(sTime);

        QPixmap pixMap = m_pTableView->grab();
        pixMap.save(fileName);
    }
}

//=============================================================================================================

void FiffRawView::customContextMenuRequested(const QPoint &pos)
{
    //double dScrollDiff = static_cast<double>(m_pTableView->horizontalScrollBar()->maximum()) / static_cast<double>(m_pModel->absoluteLastSample() - m_pModel->absoluteFirstSample());
    m_fLastClickedSample = floor((float)m_pModel->absoluteFirstSample() + //accounting for first sample offset
                             (m_pTableView->horizontalScrollBar()->value() / m_pModel->pixelDifference()) + //accounting for scroll offset
                             ((float)pos.x() / m_pModel->pixelDifference())); //accounting for mouse position offset

    QMenu* menu = new QMenu(this);

    //qDebug() << "MAX:" << m_pTableView->horizontalScrollBar()->maximum();

    QAction* markTime = menu->addAction(tr("Mark time"));
    connect(markTime, &QAction::triggered,
            this, &FiffRawView::addTimeMark, Qt::UniqueConnection);

    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}

//=============================================================================================================

void FiffRawView::addTimeMark(bool con)
{
    Q_UNUSED(con);
    m_pModel->addTimeMark(static_cast<int>(m_fLastClickedSample));
//    emit sendSamplePos(m_fLastClickedPoint);
}

//=============================================================================================================

void FiffRawView::toggleDisplayEvent(const int& iToggle)
{
    int m_iToggle = iToggle;
    //qDebug() << "toggleDisplayEvent" << iToggle;
    m_pModel->toggleDispAnn(m_iToggle);
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
    }

    //Activate grabbing gesture when scrollbars or vertical header are deselected
    if ((object == m_pTableView->horizontalScrollBar() ||
         object == m_pTableView->verticalScrollBar() ||
         object == m_pTableView->verticalHeader())
        && event->type() == QEvent::Leave) {
        QScroller::grabGesture(m_pTableView, QScroller::LeftMouseButtonGesture);
        return true;
    }

    return false;
}

//=============================================================================================================

void FiffRawView::updateScrollPosition()
{
    int iSample = m_pModel->getAnnotationModel()->getAnnotation(m_pModel->getAnnotationModel()->getSelectedAnn()) - m_pModel->absoluteFirstSample();
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

void FiffRawView::createLabels()
{
    QHBoxLayout *LabelLayout = new QHBoxLayout(this);
    QWidget* labelBar = new QWidget(this);

    m_pLeftLabel = new QLabel(this);
    m_pRightLabel = new QLabel(this);

    m_pLeftLabel->setText(" ");
    m_pLeftLabel->setAlignment(Qt::AlignLeft);

    m_pRightLabel->setText(" ");
    m_pRightLabel->setAlignment(Qt::AlignRight);

    LabelLayout->addWidget(m_pLeftLabel);
    LabelLayout->addWidget(m_pRightLabel);
    labelBar->setLayout(LabelLayout);

    this->layout()->addWidget(labelBar);

    m_pLeftLabel->show();
    m_pRightLabel->show();
    labelBar->show();
}

//=============================================================================================================

void FiffRawView::updateLabels(int iValue)
{
    Q_UNUSED(iValue);

    QString strLeft, strRight;

    //Left Label
    int iSample = static_cast<int>(m_pTableView->horizontalScrollBar()->value() / m_pModel->pixelDifference());
    strLeft = QString("%1 | %2 sec").arg(QString().number(iSample)).arg(QString().number(iSample / m_pModel->getFiffInfo()->sfreq, 'f', 2));
    m_pLeftLabel->setText(strLeft);

    //Right Label
    iSample += m_iT * m_pModel->getFiffInfo()->sfreq;
    strRight = QString("%1 | %2 sec").arg(QString().number(iSample)).arg(QString().number(iSample / m_pModel->getFiffInfo()->sfreq, 'f', 2));
    m_pRightLabel->setText(strRight);
}

//=============================================================================================================

void FiffRawView::disconnectModel()
{
    // Disconnect QScrollBar with model
    disconnect(m_pTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_pModel.data(), &FiffRawViewModel::updateScrollPosition);

    // Disconnect resizing of the table view to the MVC
    disconnect(this, &FiffRawView::tableViewDataWidthChanged,
            m_pModel.data(), &FiffRawViewModel::setDataColumnWidth);
}
