//=============================================================================================================
/**
* @file     digitizersettreeitem.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Jana Kiesel and Matti Hamalainen. All rights reserved.
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
* @brief    DigitizerSetTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizersettreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"
#include "../common/renderable3Dentity.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerSetTreeItem::DigitizerSetTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Digitizer Set");
}


//*************************************************************************************************************

DigitizerSetTreeItem::~DigitizerSetTreeItem()
{
    //Delete entity so that the SceneGraph is NOT plotting it anymore.
    //QPointer only deletes if the parent is destroyed. What happens if this item is destroyed before the parent is destroyed?
    //Cannot delete m_pParentEntity since we do not know who else holds it.
    delete m_pRenderable3DEntity;
}


//*************************************************************************************************************

QVariant DigitizerSetTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  DigitizerSetTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool DigitizerSetTreeItem::addData(const FIFFLIB::FiffDigPointSet& tDigitizer, Qt3DCore::QEntity* parent)
{
    //Add data
    bool state = false;

    //parsing the digitizer List
    QList<FIFFLIB::FiffDigPoint> tCardinal;
    QList<FIFFLIB::FiffDigPoint> tHpi;
    QList<FIFFLIB::FiffDigPoint> tEeg;
    QList<FIFFLIB::FiffDigPoint> tExtra;

    for(int i = 0; i < tDigitizer.size(); ++i){
        switch (tDigitizer[i].kind) {
        case FIFFV_POINT_CARDINAL:
            tCardinal.append(tDigitizer[i]);
            break;
        case FIFFV_POINT_HPI:
            tHpi.append(tDigitizer[i]);
            break;
        case FIFFV_POINT_EEG:
            tEeg.append(tDigitizer[i]);
            break;
        case FIFFV_POINT_EXTRA:
            tExtra.append(tDigitizer[i]);
            break;
        default:
            break;
        }
    }

    // Find the Digitizer Items
    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
    bool bFoundCardinalItem = false;

    for(int i = 0; i < itemList.size(); ++i) {
        DigitizerTreeItem* item = dynamic_cast<DigitizerTreeItem*>(itemList.at(i));

        if(item->text() == "Cardinal" && !tCardinal.empty()) {
            item->addData(tCardinal, parent);
            bFoundCardinalItem = true;
        }
    }

    if (!bFoundCardinalItem && !tCardinal.empty()){
        //Create a cardinal digitizer item
        QList<QStandardItem*> itemListCardinal;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(Data3DTreeModelItemTypes::DigitizerItem,"Cardinal");
        state = digitizerItem->addData(tCardinal, parent);
        itemListCardinal << digitizerItem;
        itemListCardinal << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListCardinal);
    }

//    if (!tHpi.empty()){
//        //Create a HPI digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(Data3DTreeModelItemTypes::DigitizerItem,"HPI");
//        state = digitizerItem->addData(tHpi, parent);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tEeg.empty()){
//        //Create a EEG digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(Data3DTreeModelItemTypes::DigitizerItem,"EEG/ECG");
//        state = digitizerItem->addData(tEeg, parent);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tExtra.empty()){
//        //Create a extra digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(Data3DTreeModelItemTypes::DigitizerItem,"Extra");
//        state = digitizerItem->addData(tExtra, parent);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
    return state;
}


//*************************************************************************************************************

void DigitizerSetTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i < this->rowCount(); ++i) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }
}
