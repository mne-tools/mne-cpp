//=============================================================================================================
/**
* @file     ChannelDataViewNew.cpp
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
* @brief    Definition of the ChannelDataViewNew Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ChannelDataViewNew.h"

#include "helpers/channeldataitem.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QOpenGLWidget>
#include <QPointer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGLWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataViewNew::ChannelDataViewNew(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    m_pGraphicsScene = new QGraphicsScene;
    m_pQGraphicsView = new QGraphicsView;
    m_pQGraphicsView->setViewport(new QGLWidget);
    m_pQGraphicsView->setScene(m_pGraphicsScene);

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0,0,0,0);

    neLayout->addWidget(m_pQGraphicsView);

    //set layouts
    this->setLayout(neLayout);
}


//*************************************************************************************************************

void ChannelDataViewNew::init(QSharedPointer<FIFFLIB::FiffInfo> &info)
{
    m_pFiffInfo = info;

    for(int j = 0; j < m_pFiffInfo->chs.size(); j++) {
        if(!m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(j).ch_name)) {
            QPointer<ChannelDataItem> pItem = new ChannelDataItem(m_pFiffInfo->chs.at(j).kind,
                                                                  m_pFiffInfo->chs.at(j).unit);
            pItem->setPos(QPointF(0.0,j*10.0));
            m_pGraphicsScene->addItem(pItem);
            m_lItemList << pItem;
        }
    }
}


//*************************************************************************************************************

void ChannelDataViewNew::addData(const QList<Eigen::MatrixXd> &data)
{
    if(data.isEmpty()) {
        return;
    }

    for(int j = 0; j < m_lItemList.size(); j++) {
        for(int i = 0; i < data.size(); i++) {
            m_lItemList.at(j)->addData(data.at(i).row(j));
        }
    }

    m_pGraphicsScene->update();
}

