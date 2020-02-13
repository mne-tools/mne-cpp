//=============================================================================================================
/**
 * @file     rtfiffrawview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RtFiffRawView Class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfiffrawview.h"

#include "helpers/rtfiffrawviewdelegate.h"
#include "helpers/rtfiffrawviewmodel.h"

#include <utils/filterTools/filterdata.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QHeaderView>
#include <QTableView>
#include <QMenu>
#include <QSvgGenerator>
#include <QSettings>
#include <QScrollBar>
#include <QMouseEvent>

#if defined(USE_OPENGL)
    #include <QOpenGLWidget>
#endif


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFiffRawView::RtFiffRawView(const QString& sSettingsPath,
                                 QWidget *parent,
                                 Qt::WindowFlags f)
: QWidget(parent, f)
, m_iT(10)
, m_fSamplingRate(1024)
, m_fDefaultSectionSize(80.0f)
, m_fZoomFactor(1.0f)
, m_bHideBadChannels(false)
, m_sSettingsPath(sSettingsPath)
, m_iDistanceTimeSpacer(1)
{
    m_pTableView = new QTableView;

#if defined(USE_OPENGL)
    m_pTableView->setViewport(new QOpenGLWidget);
#endif

    // Install event filter for tracking mouse movements
    m_pTableView->viewport()->installEventFilter(this);
    m_pTableView->setMouseTracking(true);

    // Set layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pTableView);
    neLayout->setContentsMargins(0,0,0,0);
    this->setLayout(neLayout);

    loadSettings(m_sSettingsPath);
}


//*************************************************************************************************************

RtFiffRawView::~RtFiffRawView()
{
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void RtFiffRawView::init(QSharedPointer<FIFFLIB::FiffInfo> &info)
{
    m_pFiffInfo = info;
    m_fSamplingRate = m_pFiffInfo->sfreq;

    //Init the model
    m_pModel = new RtFiffRawViewModel(this);
    m_pModel->setFiffInfo(m_pFiffInfo);
    m_pModel->setSamplingInfo(m_fSamplingRate, m_iT, true);
    connect(m_pModel.data(), &RtFiffRawViewModel::triggerDetected,
            this, &RtFiffRawView::triggerDetected);

    //Init bad channel list
    m_qListBadChannels.clear();
    for(int i = 0; i<m_pModel->rowCount(); i++) {
        if(m_pModel->data(m_pModel->index(i,2)).toBool()) {
            m_qListBadChannels << i;
        }
    }

    //Init the delegate
    m_pDelegate = new RtFiffRawViewDelegate(this);
    m_pDelegate->initPainterPaths(m_pModel);

    connect(this, &RtFiffRawView::markerMoved,
            m_pDelegate.data(), &RtFiffRawViewDelegate::markerMoved);

    //Init the view
    m_pTableView->setModel(m_pModel);
    m_pTableView->setItemDelegate(m_pDelegate);
    m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_pTableView.data(), &QTableView::doubleClicked,
            m_pModel.data(), &RtFiffRawViewModel::toggleFreeze);

    connect(m_pTableView.data(), &QTableView::customContextMenuRequested,
            this, &RtFiffRawView::channelContextMenu);

    //set some size settings for m_pTableView
    m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    m_pTableView->setShowGrid(false);
    m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
    m_pTableView->horizontalHeader()->hide();
    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
    m_pTableView->setAutoScroll(false);
    m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    m_pTableView->setColumnHidden(2,true);
    m_pTableView->resizeColumnsToContents();
    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(m_pTableView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &RtFiffRawView::visibleRowsChanged);
}


//*************************************************************************************************************

void RtFiffRawView::addData(const QList<Eigen::MatrixXd> &data)
{
    m_pModel->addData(data);
}


//*************************************************************************************************************

MatrixXd RtFiffRawView::getLastBlock()
{
    return m_pModel->getLastBlock();
}


//*************************************************************************************************************

bool RtFiffRawView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_pTableView->viewport() && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        emit markerMoved(mouseEvent->pos(), m_pTableView->rowAt(mouseEvent->pos().y()));
        return true;
    }

    return QWidget::eventFilter(object, event);
}


//*************************************************************************************************************

void RtFiffRawView::setBackgroundColor(const QColor& backgroundColor)
{
    m_backgroundColor = backgroundColor;

    if(m_pModel) {
        m_pModel->setBackgroundColor(m_backgroundColor);
    }

//    QPalette pal;
//    pal.setColor(QPalette::Window, m_backgroundColor);
//    m_pTableView->viewport()->setPalette(pal);
//    m_pTableView->viewport()->setBackgroundRole(QPalette::Window);
}


//*************************************************************************************************************

QColor RtFiffRawView::getBackgroundColor()
{
    return m_backgroundColor;
}


//*************************************************************************************************************

QMap<qint32, float> RtFiffRawView::getScalingMap()
{
    return m_qMapChScaling;
}


//*************************************************************************************************************

void RtFiffRawView::setScalingMap(const QMap<qint32, float>& scaleMap)
{
    m_qMapChScaling = scaleMap;
    m_pModel->setScaling(scaleMap);
}


//*************************************************************************************************************

void RtFiffRawView::setSignalColor(const QColor& signalColor)
{
    m_pDelegate->setSignalColor(signalColor);
}


//*************************************************************************************************************

QColor RtFiffRawView::getSignalColor()
{
    return m_pDelegate->getSignalColor();
}


//*************************************************************************************************************

void RtFiffRawView::hideBadChannels()
{
    if(m_bHideBadChannels) {
        m_bHideBadChannels = false;
    } else {
        m_bHideBadChannels = true;
    }

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_qListBadChannels.size(); i++) {
        if(m_bHideBadChannels) {
            m_pTableView->hideRow(m_qListBadChannels.at(i));
        } else {
            m_pTableView->showRow(m_qListBadChannels.at(i));
        }
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged();
}


//*************************************************************************************************************

bool RtFiffRawView::getBadChannelHideStatus()
{
    return m_bHideBadChannels;
}


//*************************************************************************************************************

void RtFiffRawView::showSelectedChannelsOnly(const QStringList &selectedChannels)
{
    m_slSelectedChannels = selectedChannels;

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pModel->rowCount(); i++) {
        QString channel = m_pModel->data(m_pModel->index(i, 0), Qt::DisplayRole).toString();

        //if channel is a bad channel and bad channels are to be hidden -> do not show
        if(!selectedChannels.contains(channel) || (m_qListBadChannels.contains(i) && m_bHideBadChannels)) {
            m_pTableView->hideRow(i);
        } else {
            m_pTableView->showRow(i);
        }
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged();
}


//*************************************************************************************************************

void RtFiffRawView::setZoom(double zoomFac)
{
    m_fZoomFactor = zoomFac;

    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
}


//*************************************************************************************************************

double RtFiffRawView::getZoom()
{
    return m_fZoomFactor;
}


//*************************************************************************************************************

void RtFiffRawView::setWindowSize(int T)
{
    m_iT = T;

    m_pModel->setSamplingInfo(m_fSamplingRate, T);
}


//*************************************************************************************************************

int RtFiffRawView::getWindowSize()
{
    return m_iT;
}


//*************************************************************************************************************

void RtFiffRawView::takeScreenshot(const QString& fileName)
{
    if(fileName.contains(".svg", Qt::CaseInsensitive)) {
        // Generate screenshot
        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        svgGen.setSize(m_pTableView->size());
        svgGen.setViewBox(m_pTableView->rect());

        m_pTableView->render(&svgGen);
    }

    if(fileName.contains(".png", Qt::CaseInsensitive)) {
        QPixmap pixMap = QPixmap::grabWidget(m_pTableView);
        pixMap.save(fileName);
    }
}

//*************************************************************************************************************

void RtFiffRawView::updateProjection(const QList<FIFFLIB::FiffProj>& projs)
{
    m_pModel->updateProjection(projs);
}


//*************************************************************************************************************

void RtFiffRawView::updateCompensator(int to)
{
    m_pModel->updateCompensator(to);
}


//*************************************************************************************************************

void RtFiffRawView::updateSpharaActivation(bool state)
{
    m_pModel->updateSpharaActivation(state);
}


//*************************************************************************************************************

void RtFiffRawView::updateSpharaOptions(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond)
{
    m_pModel->updateSpharaOptions(sSytemType, nBaseFctsFirst, nBaseFctsSecond);
}


//*************************************************************************************************************

void RtFiffRawView::setFilter(const FilterData& filterData)
{
    m_pModel->setFilter(QList<FilterData>() << filterData);
}


//*************************************************************************************************************

void RtFiffRawView::setFilterActive(bool state)
{
    m_pModel->setFilterActive(state);
}


//*************************************************************************************************************

void RtFiffRawView::setFilterChannelType(const QString &channelType)
{
    m_pModel->setFilterChannelType(channelType);
}


//*************************************************************************************************************

void RtFiffRawView::triggerInfoChanged(const QMap<double, QColor>& colorMap,
                                         bool active,
                                         const QString &triggerCh,
                                         double threshold)
{
    m_pModel->triggerInfoChanged(colorMap, active, triggerCh, threshold);
}


//*************************************************************************************************************

void RtFiffRawView::setDistanceTimeSpacer(int value)
{
    m_iDistanceTimeSpacer = value;
    m_pModel->distanceTimeSpacerChanged(value);
}


//*************************************************************************************************************

int RtFiffRawView::getDistanceTimeSpacer()
{
    return m_iDistanceTimeSpacer;
}


//*************************************************************************************************************

void RtFiffRawView::resetTriggerCounter()
{
    m_pModel->resetTriggerCounter();
}


//*************************************************************************************************************

void RtFiffRawView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
}


//*************************************************************************************************************

void RtFiffRawView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;
}


//*************************************************************************************************************

void RtFiffRawView::channelContextMenu(QPoint pos)
{
    //obtain index where index was clicked
    QModelIndex index = m_pTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = m_pTableView->selectionModel()->selectedIndexes();

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Marking ****************
    if(!m_qListBadChannels.contains(index.row())) {
        QAction* doMarkChBad = menu->addAction(tr("Mark as bad"));
        connect(doMarkChBad, &QAction::triggered,
                this, &RtFiffRawView::markChBad);
    } else {
        QAction* doMarkChGood = menu->addAction(tr("Mark as good"));
        connect(doMarkChGood, &QAction::triggered,
                this, &RtFiffRawView::markChBad);
    }

    // non C++11 alternative
    m_qListCurrentSelection.clear();
    for(qint32 i = 0; i < selected.size(); ++i)
        if(selected[i].column() == 1)
            m_qListCurrentSelection.append(m_pModel->getIdxSelMap()[selected[i].row()]);

    QAction* doSelection = menu->addAction(tr("Apply selection"));
    connect(doSelection, &QAction::triggered,
            this, &RtFiffRawView::applySelection);

    //select channels
    QAction* hideSelection = menu->addAction(tr("Hide selection"));
    connect(hideSelection, &QAction::triggered, this,
            &RtFiffRawView::hideSelection);

    //undo selection
    QAction* resetAppliedSelection = menu->addAction(tr("Reset selection"));
    connect(resetAppliedSelection, &QAction::triggered,
            m_pModel.data(), &RtFiffRawViewModel::resetSelection);
    connect(resetAppliedSelection, &QAction::triggered,
            this, &RtFiffRawView::resetSelection);

    //show context menu
    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void RtFiffRawView::applySelection()
{
    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pModel->rowCount(); i++) {
        //if channel is a bad channel and bad channels are to be hidden -> do not show
        if(m_qListCurrentSelection.contains(i)) {
            m_pTableView->showRow(i);
        } else {
            m_pTableView->hideRow(i);
        }
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged();

    //m_pModel->selectRows(m_qListCurrentSelection);
}


//*************************************************************************************************************

void RtFiffRawView::hideSelection()
{
    for(int i=0; i<m_qListCurrentSelection.size(); i++) {
        m_pTableView->hideRow(m_qListCurrentSelection.at(i));
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged();
}


//*************************************************************************************************************

void RtFiffRawView::resetSelection()
{
    // non C++11 alternative
    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if(m_qListBadChannels.contains(i)) {
            if(!m_bHideBadChannels) {
                m_pTableView->showRow(i);
            }
        } else {
            m_pTableView->showRow(i);
        }
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged();
}


//*************************************************************************************************************

void RtFiffRawView::visibleRowsChanged()
{
    if(!m_pTableView || !m_pModel || !m_pDelegate) {
        return;
    }

    int from = m_pTableView->rowAt(0);
    if(from != 0)
        from--;

    int to = m_pTableView->rowAt(m_pTableView->height()-1);
    if(to != m_pModel->rowCount()-1)
        to++;

    if(from > to)
        to = m_pModel->rowCount()-1;

//    //Update visible rows in order to only filter the visible rows
//    QStringList channelNames;

//    for(int i = from; i<=to; i++) {
//        channelNames << m_pModel->data(m_pModel->index(i, 0), Qt::DisplayRole).toString();
//    }

//    m_pModel->createFilterChannelList(channelNames);

    m_pDelegate->setUpperItemIndex(from+1);

    //qDebug() <<"RtFiffRawView::visibleRowsChanged - from "<< from << " to" << to;
}


//*************************************************************************************************************

void RtFiffRawView::markChBad()
{
    QModelIndexList selected = m_pTableView->selectionModel()->selectedIndexes();

    for(int i=0; i<selected.size(); i++) {
        if(m_qListBadChannels.contains(selected[i].row())) { //mark as good
            m_pModel->markChBad(selected[i], false);
            m_qListBadChannels.removeAll(selected[i].row());
        }
        else {
            m_pModel->markChBad(selected[i], true);
            m_qListBadChannels.append(selected[i].row());
        }
    }

    m_pModel->updateProjection(m_pFiffInfo->projs);

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_qListBadChannels.size(); i++) {
        if(m_bHideBadChannels) {
            m_pTableView->hideRow(m_qListBadChannels.at(i));
        } else {
            m_pTableView->showRow(m_qListBadChannels.at(i));
        }
    }

    emit channelMarkingChanged();
}
