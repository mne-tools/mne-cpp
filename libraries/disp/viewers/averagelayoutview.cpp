//=============================================================================================================
/**
* @file     averagelayoutview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
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
* @brief    Definition of the AverageLayoutView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagelayoutview.h"

#include "helpers/averagescene.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsView>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageLayoutView::AverageLayoutView(QWidget *parent)
: QWidget(parent, Qt::Window)
{
    this->setWindowTitle("Average Layout");

    m_pAverageLayoutView = QSharedPointer<QGraphicsView>(new QGraphicsView(this));
    m_pAverageScene = AverageScene::SPtr(new AverageScene(m_pAverageLayoutView.data(), this));

    m_pAverageLayoutView->setScene(m_pAverageScene.data());
    QBrush brush(Qt::black);
    m_pAverageScene->setBackgroundBrush(brush);

    //set layouts
    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pAverageLayoutView.data());
    this->setLayout(neLayout);
}


//*************************************************************************************************************

QSharedPointer<AverageScene> AverageLayoutView::getAverageScene()
{
    return m_pAverageScene;
}


//*************************************************************************************************************

QSharedPointer<QGraphicsView> AverageLayoutView::getAverageGraphicsView()
{
    return m_pAverageLayoutView;
}
