//=============================================================================================================
/**
* @file     channeldataview.cpp
* @author   Lorenz Esch <lesc@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the ChannelDataView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldataview.h"

#include "helpers/channeldatadelegate.h"
#include "helpers/channeldatamodel.h"

#include <utils/filterTools/filterdata.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QHeaderView>
#include <QTableView>
#include <QMenu>
#include <QSvgGenerator>
#include <QGLWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataView::ChannelDataView(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_iT(10)
, m_fSamplingRate(1024)
, m_fDefaultSectionSize(80.0f)
, m_fZoomFactor(1.0f)
, m_bHideBadChannels(false)
{
    // Use QGLWidget for rendering the table view.
    // Unfortunatley, QOpenGLWidget is not able to change the background color, which is a must for this ChanalDataViewer.
    m_pTableView = new QTableView;
    QGLFormat currentFormat = QGLFormat(QGL::SampleBuffers);
    currentFormat.setSamples(10);
    QGLWidget* pGLWidget = new QGLWidget(currentFormat);
    m_pTableView->setViewport(pGLWidget);

    // Install event filter for tracking mouse movements
    m_pTableView->viewport()->installEventFilter(this);
    m_pTableView->setMouseTracking(true);

    // Set layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pTableView);
    this->setLayout(neLayout);
}


//*************************************************************************************************************

void ChannelDataView::init(QSharedPointer<FIFFLIB::FiffInfo> &info)
{
    m_pFiffInfo = info;
    m_fSamplingRate = m_pFiffInfo->sfreq;

    //Init the model
    m_pModel = new ChannelDataModel(this);
    m_pModel->setFiffInfo(m_pFiffInfo);
    m_pModel->setSamplingInfo(m_fSamplingRate, m_iT, true);
    connect(m_pModel.data(), &ChannelDataModel::triggerDetected,
            this, &ChannelDataView::triggerDetected);

    //Init bad channel list
    m_qListBadChannels.clear();
    for(int i = 0; i<m_pModel->rowCount(); i++) {
        if(m_pModel->data(m_pModel->index(i,2)).toBool()) {
            m_qListBadChannels << i;
        }
    }

    //Init the delegate
    m_pDelegate = new ChannelDataDelegate(this);
    m_pDelegate->initPainterPaths(m_pModel);

    connect(this, &ChannelDataView::markerMoved,
            m_pDelegate.data(), &ChannelDataDelegate::markerMoved);

    //Init the view
    m_pTableView->setModel(m_pModel);
    m_pTableView->setItemDelegate(m_pDelegate);
    m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_pTableView.data(), &QTableView::doubleClicked,
            m_pModel.data(), &ChannelDataModel::toggleFreeze);

    connect(m_pTableView.data(), &QTableView::customContextMenuRequested,
            this, &ChannelDataView::channelContextMenu);

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

//    connect(m_pTableView->verticalScrollBar(), &QScrollBar::valueChanged,
//            this, &ChannelDataView::visibleRowsChanged);
}


//*************************************************************************************************************

void ChannelDataView::addData(const QList<Eigen::MatrixXd> &data)
{
    m_pModel->addData(data);
}


//*************************************************************************************************************

MatrixXd ChannelDataView::getLastBlock()
{
    return m_pModel->getLastBlock();
}


//*************************************************************************************************************

bool ChannelDataView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_pTableView->viewport() && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        emit markerMoved(mouseEvent->pos(), m_pTableView->rowAt(mouseEvent->pos().y()));
        return true;
    }

    return QWidget::eventFilter(object, event);
}


//*************************************************************************************************************

void ChannelDataView::setBackgroundColor(const QColor& backgroundColor)
{
    m_backgroundColor = backgroundColor;

    QPalette pal;
    pal.setColor(QPalette::Window, m_backgroundColor);
    m_pTableView->viewport()->setPalette(pal);
    m_pTableView->viewport()->setBackgroundRole(QPalette::Window);
}


//*************************************************************************************************************

QColor ChannelDataView::getBackgroundColor()
{
    return m_backgroundColor;
}


//*************************************************************************************************************

QMap<qint32, float> ChannelDataView::getScalingMap()
{
    return m_qMapChScaling;
}


//*************************************************************************************************************

void ChannelDataView::setScalingMap(const QMap<qint32, float>& scaleMap)
{
    m_qMapChScaling = scaleMap;
    m_pModel->setScaling(scaleMap);
}


//*************************************************************************************************************

void ChannelDataView::setSignalColor(const QColor& signalColor)
{
    m_pDelegate->setSignalColor(signalColor);
}


//*************************************************************************************************************

QColor ChannelDataView::getSignalColor()
{
    return m_pDelegate->getSignalColor();
}


//*************************************************************************************************************

void ChannelDataView::hideBadChannels()
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
    //visibleRowsChanged(0);
}


//*************************************************************************************************************

bool ChannelDataView::getBadChannelHideStatus()
{
    return m_bHideBadChannels;
}


//*************************************************************************************************************

void ChannelDataView::showSelectedChannelsOnly(const QStringList &selectedChannels)
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
    //visibleRowsChanged(0);
}


//*************************************************************************************************************

void ChannelDataView::setZoom(double zoomFac)
{
    m_fZoomFactor = zoomFac;

    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
}


//*************************************************************************************************************

double ChannelDataView::getZoom()
{
    return m_fZoomFactor;
}


//*************************************************************************************************************

void ChannelDataView::setWindowSize(int T)
{
    m_iT = T;

    m_pModel->setSamplingInfo(m_fSamplingRate, T);
}


//*************************************************************************************************************

int ChannelDataView::getWindowSize()
{
    return m_iT;
}


//*************************************************************************************************************

void ChannelDataView::takeScreenshot(const QString& fileName)
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

void ChannelDataView::updateProjection()
{
    m_pModel->updateProjection();
}


//*************************************************************************************************************

void ChannelDataView::updateCompensator(int to)
{
    m_pModel->updateCompensator(to);
}


//*************************************************************************************************************

void ChannelDataView::updateSpharaActivation(bool state)
{
    m_pModel->updateSpharaActivation(state);
}


//*************************************************************************************************************

void ChannelDataView::updateSpharaOptions(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond)
{
    m_pModel->updateSpharaOptions(sSytemType, nBaseFctsFirst, nBaseFctsSecond);
}


//*************************************************************************************************************

void ChannelDataView::setFilter(const FilterData& filterData)
{
    m_pModel->filterChanged(QList<FilterData>() << filterData);
}


//*************************************************************************************************************

void ChannelDataView::setFilterActive(bool state)
{
    m_pModel->setFilterActive(state);
}


//*************************************************************************************************************

void ChannelDataView::setFilterChannelType(const QString &channelType)
{
    m_pModel->setFilterChannelType(channelType);
}


//*************************************************************************************************************

void ChannelDataView::triggerInfoChanged(const QMap<double, QColor>& colorMap,
                                         bool active,
                                         const QString &triggerCh,
                                         double threshold)
{
    m_pModel->triggerInfoChanged(colorMap, active, triggerCh, threshold);
}


//*************************************************************************************************************

void ChannelDataView::setDistanceTimeSpacer(int value)
{
    m_iDistanceTimeSpacer = value;
    m_pModel->distanceTimeSpacerChanged(value);
}


//*************************************************************************************************************

int ChannelDataView::getDistanceTimeSpacer()
{
    return m_iDistanceTimeSpacer;
}


//*************************************************************************************************************

void ChannelDataView::resetTriggerCounter()
{
    m_pModel->resetTriggerCounter();
}


//*************************************************************************************************************

void ChannelDataView::channelContextMenu(QPoint pos)
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
                this, &ChannelDataView::markChBad);
    } else {
        QAction* doMarkChGood = menu->addAction(tr("Mark as good"));
        connect(doMarkChGood, &QAction::triggered,
                this, &ChannelDataView::markChBad);
    }

    // non C++11 alternative
    m_qListCurrentSelection.clear();
    for(qint32 i = 0; i < selected.size(); ++i)
        if(selected[i].column() == 1)
            m_qListCurrentSelection.append(m_pModel->getIdxSelMap()[selected[i].row()]);

    QAction* doSelection = menu->addAction(tr("Apply selection"));
    connect(doSelection, &QAction::triggered,
            this, &ChannelDataView::applySelection);

    //select channels
    QAction* hideSelection = menu->addAction(tr("Hide selection"));
    connect(hideSelection, &QAction::triggered, this,
            &ChannelDataView::hideSelection);

    //undo selection
    QAction* resetAppliedSelection = menu->addAction(tr("Reset selection"));
    connect(resetAppliedSelection, &QAction::triggered,
            m_pModel.data(), &ChannelDataModel::resetSelection);
    connect(resetAppliedSelection, &QAction::triggered,
            this, &ChannelDataView::resetSelection);

    //show context menu
    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void ChannelDataView::applySelection()
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
    //visibleRowsChanged(0);

    //m_pModel->selectRows(m_qListCurrentSelection);
}


//*************************************************************************************************************

void ChannelDataView::hideSelection()
{
    for(int i=0; i<m_qListCurrentSelection.size(); i++) {
        m_pTableView->hideRow(m_qListCurrentSelection.at(i));
    }

    //Update the visible channel list which are to be filtered
    //visibleRowsChanged(0);
}


//*************************************************************************************************************

void ChannelDataView::resetSelection()
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
    //visibleRowsChanged(0);
}


//*************************************************************************************************************

void ChannelDataView::visibleRowsChanged(int value)
{
    Q_UNUSED(value);
    //std::cout <<"Visible channels: "<< m_pTableView->rowAt(0) << "-" << m_pTableView->rowAt(m_pTableView->height())<<std::endl;

    int from = m_pTableView->rowAt(0);
    if(from != 0)
        from--;

    int to = m_pTableView->rowAt(m_pTableView->height()-1);
    if(to != m_pModel->rowCount()-1)
        to++;

    if(from > to)
        to = m_pModel->rowCount()-1;

    QStringList channelNames;

    for(int i = from; i<=to; i++) {
        channelNames << m_pModel->data(m_pModel->index(i, 0), Qt::DisplayRole).toString();
        //std::cout << m_pModel->data(m_pModel->index(i, 0), Qt::DisplayRole).toString().toStdString() << std::endl;
    }

    m_pModel->createFilterChannelList(channelNames);
}


//*************************************************************************************************************

void ChannelDataView::markChBad()
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

    m_pModel->updateProjection();

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
