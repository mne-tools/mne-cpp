//=============================================================================================================
/**
* @file     cluststctabledelegate.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     June, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the ClustStcTableDelegate class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cluststctabledelegate.h"
#include "cluststcmodel.h" //Declar MetaType Eigen::Matrix3Xf; FSLIB::Label


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ClustStcTableDelegate::ClustStcTableDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{
}

//*************************************************************************************************************

void ClustStcTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
//    float t_fPlotHeight = option.rect.height();
    switch(index.column()) {
        case 0: { //index
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 1: { //vertex
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 2: { //stc value
            painter->save();

            qint32 val = index.model()->data(index,Qt::DisplayRole).value<VectorXd>().size();
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(val));

            painter->restore();
            break;
        }
        case 3: { //stc relative value
            painter->save();

            qint32 val = index.model()->data(index,Qt::DisplayRole).value<VectorXd>().size();
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(val));

            painter->restore();
            break;
        }
        case 4: { //Label
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).value<Label>().name);

            painter->restore();
            break;
        }
        case 5: { //Color
            painter->save();

            QColor c = index.model()->data(index,Qt::DisplayRole).value<QColor>();

            painter->setPen(c);
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,c.name());

            painter->restore();
            break;
        }
        case 6: { //Tri Coords
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(index.model()->data(index,Qt::DisplayRole).value<Matrix3Xf>().cols()));

            painter->restore();
            break;
        }
    }

}


//*************************************************************************************************************

QSize ClustStcTableDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
        case 0:
            size = QSize(20,option.rect.height());
            break;
        case 1:
            size = QSize(10,option.rect.height());
            break;
    }

    return size;
}
