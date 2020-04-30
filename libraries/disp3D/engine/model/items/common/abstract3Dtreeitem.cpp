//=============================================================================================================
/**
 * @file     abstract3Dtreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    Abstract3DTreeItem class definition.
 *
 */
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstract3Dtreeitem.h"
#include "../common/metatreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QPhongMaterial>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Abstract3DTreeItem::Abstract3DTreeItem(QEntity* p3DEntityParent, int iType, const QString& text)
: QStandardItem(text)
, Renderable3DEntity(p3DEntityParent)
, m_iType(iType)
{
    initItem();
}

//=============================================================================================================

void Abstract3DTreeItem::initItem()
{
    this->setToolTip("Abstract 3D Tree Item");

    //Transformation
    QList<QStandardItem*> list;
    QVariant data;

    if(!m_pItemTransformationOptions) {
        m_pItemTransformationOptions = new MetaTreeItem(MetaTreeItemTypes::UnknownItem, "Transformation");
    }

    m_pItemTransformationOptions->setEditable(false);
    list.clear();
    list << m_pItemTransformationOptions;
    list << new QStandardItem("The transformation options");
    this->appendRow(list);

    MetaTreeItem *itemXTrans = new MetaTreeItem(MetaTreeItemTypes::TranslateX, QString::number(0));
    itemXTrans->setEditable(true);
    connect(itemXTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onTranslationXChanged);
    list.clear();
    list << itemXTrans;
    list << new QStandardItem(itemXTrans->toolTip());
    m_pItemTransformationOptions->appendRow(list);

    MetaTreeItem *itemYTrans = new MetaTreeItem(MetaTreeItemTypes::TranslateY, QString::number(0));
    itemYTrans->setEditable(true);
    connect(itemYTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onTranslationYChanged);
    list.clear();
    list << itemYTrans;
    list << new QStandardItem(itemYTrans->toolTip());
    m_pItemTransformationOptions->appendRow(list);

    MetaTreeItem *itemZTrans = new MetaTreeItem(MetaTreeItemTypes::TranslateZ, QString::number(0));
    itemZTrans->setEditable(true);
    connect(itemZTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onTranslationZChanged);
    list.clear();
    list << itemZTrans;
    list << new QStandardItem(itemZTrans->toolTip());
    m_pItemTransformationOptions->appendRow(list);

    float fScale = 1.0f;
    MetaTreeItem *itemScale = new MetaTreeItem(MetaTreeItemTypes::Scale, QString::number(fScale));
    itemScale->setEditable(true);
    connect(itemScale, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onScaleChanged);
    list.clear();
    list << itemScale;
    list << new QStandardItem(itemZTrans->toolTip());
    data.setValue(fScale);
    itemScale->setData(data, MetaTreeItemRoles::Scale);
    m_pItemTransformationOptions->appendRow(list);

    //Color
    if(!m_pItemAppearanceOptions) {
        m_pItemAppearanceOptions = new MetaTreeItem(MetaTreeItemTypes::UnknownItem, "Appearance");
    }

    m_pItemAppearanceOptions->setEditable(false);
    list.clear();
    list << m_pItemAppearanceOptions;
    list << new QStandardItem("The color options");
    this->appendRow(list);

    float fAlpha = 0.75f;
    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::AlphaValue, QString("%1").arg(fAlpha));
    connect(itemAlpha, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onAlphaChanged);
    list.clear();
    list << itemAlpha;
    list << new QStandardItem(itemAlpha->toolTip());
    m_pItemAppearanceOptions->appendRow(list);
    data.setValue(fAlpha);
    itemAlpha->setData(data, MetaTreeItemRoles::AlphaValue);

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::Color, "Color");
    connect(pItemSurfCol, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    m_pItemAppearanceOptions->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, MetaTreeItemRoles::Color);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    //Do the connects
    connect(this, &Abstract3DTreeItem::checkStateChanged,
                this, &Abstract3DTreeItem::onCheckStateChanged);
}

//=============================================================================================================

void Abstract3DTreeItem::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);

    switch(role) {
        case Qt::CheckStateRole:{
            emit checkStateChanged(this->checkState());
            break;
        }
    }
}

//=============================================================================================================

int Abstract3DTreeItem::type() const
{
    return m_iType;
}

//=============================================================================================================

QList<QStandardItem*> Abstract3DTreeItem::findChildren(int type)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->type() == type) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}

//=============================================================================================================

QList<QStandardItem*> Abstract3DTreeItem::findChildren(const QString& text)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->text() == text) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}

//=============================================================================================================

Abstract3DTreeItem& Abstract3DTreeItem::operator<<(Abstract3DTreeItem* newItem)
{
    this->appendRow(newItem);

    return *this;
}

//=============================================================================================================

Abstract3DTreeItem& Abstract3DTreeItem::operator<<(Abstract3DTreeItem& newItem)
{
    this->appendRow(&newItem);

    return *this;
}

//=============================================================================================================

Eigen::MatrixX4f Abstract3DTreeItem::createVertColor(int numVert, const QColor& color)
{
    Eigen::MatrixX4f matColor(numVert,4);

    for(int i = 0; i < numVert; ++i) {
        matColor(i,0) = color.redF();
        matColor(i,1) = color.greenF();
        matColor(i,2) = color.blueF();
        matColor(i,3) = color.alphaF();
    }

    return matColor;
}

//=============================================================================================================

void Abstract3DTreeItem::setAlpha(float fAlpha)
{
    QVariant data;
    data.setValue(fAlpha);

    onAlphaChanged(data);
}

//=============================================================================================================

void Abstract3DTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i<this->rowCount(); i++) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }

    this->setVisible(checkState == Qt::Unchecked ? false : true);
}

//=============================================================================================================

void Abstract3DTreeItem::onTranslationXChanged(const QVariant& fTransX)
{
    if(fTransX.canConvert<float>()) {
        QVector3D position = this->position();
        position.setX(fTransX.toFloat());
        this->setPosition(position);
    }
}

//=============================================================================================================

void Abstract3DTreeItem::onTranslationYChanged(const QVariant& fTransY)
{
    if(fTransY.canConvert<float>()) {
        QVector3D position = this->position();
        position.setY(fTransY.toFloat());
        this->setPosition(position);
    }
}

//=============================================================================================================

void Abstract3DTreeItem::onTranslationZChanged(const QVariant& fTransZ)
{
    if(fTransZ.canConvert<float>()) {
        QVector3D position = this->position();
        position.setZ(fTransZ.toFloat());
        this->setPosition(position);
    }
}

//=============================================================================================================

void Abstract3DTreeItem::onScaleChanged(const QVariant& fScale)
{
    if(fScale.canConvert<float>()) {
        this->setScale(fScale.toFloat());
    }
}

//=============================================================================================================

void Abstract3DTreeItem::onColorChanged(const QVariant& color)
{
    //ka = ambient for standard QT materials, overlaod onColorchanged() if you use your own materials (i.e. fssurfacetreeitem)
    this->setMaterialParameter(color, "ka");
}

//=============================================================================================================

void Abstract3DTreeItem::onAlphaChanged(const QVariant& fAlpha)
{
    this->setMaterialParameter(fAlpha, "alpha");
}

