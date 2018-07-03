//=============================================================================================================
/**
* @file     filterdatadelegate.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
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
* @brief    Declaration of the FilterDataDelegate Class.
*
*/

#ifndef FILTERDATADELEGATE_H
#define FILTERDATADELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QItemDelegate>
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QThread>
#include <QStyledItemDelegate>
#include <QHBoxLayout>
#include <QEvent>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
* DECLARE CLASS BooleanWidget
*
* @brief The BooleanWidget class provides a QCheckBox inside of a Qwidget with including Layout
*/
class BooleanWidget : public QWidget
{
    Q_OBJECT

    public:

    QCheckBox * m_pCheckBox;

    BooleanWidget(QWidget * parent = 0)
        : QWidget(parent)
    {
        m_pCheckBox = new QCheckBox(this);
        QHBoxLayout * layout = new QHBoxLayout(this);
        layout->addWidget(m_pCheckBox,0, Qt::AlignCenter);
    }

    bool isChecked(){return m_pCheckBox->isChecked();}
    void setChecked(bool value){m_pCheckBox->setChecked(value);}
};

//=============================================================================================================
/**
* DECLARE CLASS FilterDataDelegate
*
* @brief The FilterDataDelegate class represents a filter data view delegate which creates a custom table view plot
*/
class DISPSHARED_EXPORT FilterDataDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterDataDelegate> SPtr;            /**< Shared pointer type for FilterDataDelegate class. */
    typedef QSharedPointer<const FilterDataDelegate> ConstSPtr; /**< Const shared pointer type for FilterDataDelegate class. */

    FilterDataDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
    *
    */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void setEditorData(QWidget *checkBox, const QModelIndex &index) const;
    virtual void setModelData(QWidget *checkBox, QAbstractItemModel *model, const QModelIndex &index) const;
    virtual void updateEditorGeometry(QWidget *checkBox, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

} // NAMESPACE DISPLIB

#endif // FILTERDATADELEGATE_H
