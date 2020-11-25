//=============================================================================================================
/**
 * @file     spectrumview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Definition of the SpectrumView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrumview.h"

#include "helpers/frequencyspectrummodel.h"
#include "helpers/frequencyspectrumdelegate.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QHeaderView>
#include <QTableView>
#include <QMouseEvent>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpectrumView::SpectrumView(const QString& sSettingsPath,
                           QWidget *parent,
                           Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    m_pTableView = new QTableView;

    m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pTableView->setMouseTracking(true);
    m_pTableView->viewport()->installEventFilter(this);

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);

    neLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(neLayout);
    loadSettings();
}

//=============================================================================================================

SpectrumView::~SpectrumView()
{
    saveSettings();
}

//=============================================================================================================

void SpectrumView::init(FiffInfo::SPtr &info,
                        int iScaleType)
{
    m_pFSModel = new FrequencySpectrumModel(this);

    m_pFSModel->setInfo(info);
    m_pFSModel->setScaleType(iScaleType); /*Added by Limin; 10/19/2014 for passing the scale type to the model*/

    m_pFSDelegate = new FrequencySpectrumDelegate(m_pTableView, this);
    m_pFSDelegate->setScaleType(iScaleType);

    connect(m_pTableView.data(), &QTableView::doubleClicked,
            m_pFSModel.data(), &FrequencySpectrumModel::toggleFreeze);

    // add a connection for sending mouse location to the delegate;
    connect(this, &SpectrumView::sendMouseLoc,
            m_pFSDelegate.data(), &FrequencySpectrumDelegate::rcvMouseLoc);

    m_pTableView->setModel(m_pFSModel);
    m_pTableView->setItemDelegate(m_pFSDelegate);

    //set some size settings for m_pTableView
    m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    m_pTableView->setShowGrid(false);

    m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
    m_pTableView->horizontalHeader()->hide();
    m_pTableView->verticalHeader()->setDefaultSectionSize(140);//m_fZoomFactor*m_fDefaultSectionSize);//Row Height

    m_pTableView->setAutoScroll(false);
    m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

    m_pTableView->resizeColumnsToContents();

    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    //set context menu
    m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    //connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(channelContextMenu(QPoint)));
}

//=============================================================================================================

void SpectrumView::addData(const MatrixXd &data)
{
    m_pFSModel->addData(data);
}

//=============================================================================================================

void SpectrumView::setBoundaries(int iLower,
                                 int iUpper)
{
    m_pFSModel->setBoundaries(iLower, iUpper);
}

//=============================================================================================================

bool SpectrumView::eventFilter(QObject * watched,
                               QEvent * event)
{
    if(event->type() == QEvent::MouseMove){
        QMouseEvent *mouseEvent = static_cast <QMouseEvent*>( event );
        //qDebug()<<"MouseMove event!@"<<mouseEvent->x()<<":"<<mouseEvent->y();

        int currentRow = m_pTableView->rowAt(mouseEvent->y());
        m_pTableView->selectRow(currentRow);

        QModelIndex item = m_pTableView->currentIndex();

        emit sendMouseLoc(item.row(), mouseEvent->x(), mouseEvent->y(),m_pTableView->visualRect(item) );

        return true;
    } else {
        return QWidget::eventFilter(watched, event);
    }
}

//=============================================================================================================

void SpectrumView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpectrumView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpectrumView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void SpectrumView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void SpectrumView::clearView()
{

}
