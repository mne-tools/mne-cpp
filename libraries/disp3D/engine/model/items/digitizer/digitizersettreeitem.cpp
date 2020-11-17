//=============================================================================================================
/**
 * @file     digitizersettreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Gabriel B Motta, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizersettreeitem.h"

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QTransform>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerSetTreeItem::DigitizerSetTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}

//=============================================================================================================

void DigitizerSetTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Digitizer set item");
}

//=============================================================================================================

void DigitizerSetTreeItem::addData(const FIFFLIB::FiffDigPointSet& tDigitizer, Qt3DCore::QEntity* parent)
{
    if(!m_pRenderable3DEntity) {
        m_pRenderable3DEntity = new Renderable3DEntity(parent);
    }

    //Add data
    //parsing the digitizer List
    QList<FIFFLIB::FiffDigPoint> tNasion;
    QList<FIFFLIB::FiffDigPoint> tLAP;
    QList<FIFFLIB::FiffDigPoint> tRAP;
    QList<FIFFLIB::FiffDigPoint> tHpi;
    QList<FIFFLIB::FiffDigPoint> tEeg;
    QList<FIFFLIB::FiffDigPoint> tExtra;

    for(int i = 0; i < tDigitizer.size(); ++i){

        switch (tDigitizer[i].kind) {
        case FIFFV_POINT_CARDINAL:

            switch (tDigitizer[i].ident) {
                case FIFFV_POINT_LPA:
                    tLAP.append(tDigitizer[i]);
                    tLAP.append(tDigitizer[i]);
                break;

                case FIFFV_POINT_NASION:
                    tNasion.append(tDigitizer[i]);
                    tNasion.append(tDigitizer[i]);
                break;

                case FIFFV_POINT_RPA:
                    tRAP.append(tDigitizer[i]);
                    tRAP.append(tDigitizer[i]);
                break;

                default:
                break;
                            }
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

//    //Create items all new - A bit more inefficient but we do not run into the problem that the QEntity
//    //is delted with deleteLater() which could let to deletion after the new Qentity has been created
//    //Delete all childs first. We do this because we always want to start fresh with the newly added digitizer data.
//    if(this->hasChildren()) {
//        this->removeRows(0, this->rowCount());
//    }

//    QList<QStandardItem*> itemList;

//    if (!tLAP.empty()){
//        //Create a LAP digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"LAP");
//        digitizerItem->addData(tLAP);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tNasion.empty()){
//        //Create a Nasion digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"Nasion");
//         digitizerItem->addData(tNasion);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tRAP.empty()){
//        //Create a RAO digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"RAP");
//        digitizerItem->addData(tRAP);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tHpi.empty()){
//        //Create a HPI digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"HPI");
//        digitizerItem->addData(tHpi);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tEeg.empty()){
//        //Create a EEG digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"EEG/ECG");
//        digitizerItem->addData(tEeg);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }
//    if (!tExtra.empty()){
//        //Create a extra digitizer item
//        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"Extra");
//        digitizerItem->addData(tExtra);
//        itemList << digitizerItem;
//        itemList << new QStandardItem(digitizerItem->toolTip());
//        this->appendRow(itemList);
//        itemList.clear();
//    }

    //Find exiting Digitizer Items and add data respectivley
    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::DigitizerItem);
    bool bFoundNasionlItem = false;
    bool bFoundLAPItem = false;
    bool bFoundRAPItem = false;
    bool bFoundHPIItem = false;
    bool bFoundEEGItem = false;
    bool bFoundExtraItem = false;

    for(int i = 0; i < itemList.size(); ++i) {
        DigitizerTreeItem* item = dynamic_cast<DigitizerTreeItem*>(itemList.at(i));

        if(item == Q_NULLPTR){
            qDebug() << "Dynamic cast returned null. Returning early.";
            return;
        }

        if(item->text() == "Nasion" && !tNasion.empty()) {
            item->addData(tNasion, 0.002f, Qt::green);
            bFoundNasionlItem = true;
        }

        if(item->text() == "LAP" && !tLAP.empty()) {
            item->addData(tLAP, 0.002f, Qt::red);
            bFoundLAPItem = true;
        }

        if(item->text() == "RAP" && !tRAP.empty()) {
            item->addData(tRAP, 0.002f, Qt::blue);
            bFoundRAPItem = true;
        }

        if(item->text() == "HPI" && !tHpi.empty()) {
            item->addData(tHpi, 0.001f, Qt::darkRed);
            bFoundHPIItem = true;
        }

        if(item->text() == "EEG/ECG" && !tEeg.empty()) {
            item->addData(tEeg, 0.001f, Qt::cyan);
            bFoundEEGItem = true;
        }

        if(item->text() == "Extra" && !tExtra.empty()) {
            item->addData(tExtra, 0.001f, Qt::magenta);
            bFoundExtraItem = true;
        }
    }

    //If not existent yet create here
    if (!bFoundNasionlItem && !tNasion.empty()){
        //Create a cardinal digitizer item
        QList<QStandardItem*> itemListCardinal;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"Nasion");
        digitizerItem->addData(tNasion, 0.002f, Qt::green);
        itemListCardinal << digitizerItem;
        itemListCardinal << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListCardinal);
    }

    if (!bFoundLAPItem && !tLAP.empty()){
        //Create a cardinal digitizer item
        QList<QStandardItem*> itemListCardinal;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"LAP");
        digitizerItem->addData(tLAP, 0.002f, Qt::red);
        itemListCardinal << digitizerItem;
        itemListCardinal << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListCardinal);
    }

    if (!bFoundRAPItem && !tRAP.empty()){
        //Create a cardinal digitizer item
        QList<QStandardItem*> itemListCardinal;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"RAP");
        digitizerItem->addData(tRAP, 0.002f, Qt::blue);
        itemListCardinal << digitizerItem;
        itemListCardinal << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListCardinal);
    }

    if (!bFoundHPIItem && !tHpi.empty()){
        //Create a hpi digitizer item
        QList<QStandardItem*> itemListHPI;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"HPI");
        digitizerItem->addData(tHpi, 0.001f, Qt::darkRed);
        itemListHPI << digitizerItem;
        itemListHPI << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListHPI);
    }

    if (!bFoundEEGItem && !tEeg.empty()){
        //Create a eeg ecg digitizer item
        QList<QStandardItem*> itemListEEG;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"EEG/ECG");
        digitizerItem->addData(tEeg, 0.001f, Qt::cyan);
        itemListEEG << digitizerItem;
        itemListEEG << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListEEG);
    }

    if (!bFoundExtraItem && !tExtra.empty()){
        //Create a extra digitizer item
        QList<QStandardItem*> itemListExtra;
        DigitizerTreeItem* digitizerItem = new DigitizerTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::DigitizerItem,"Extra");
        digitizerItem->addData(tExtra, 0.001f, Qt::magenta);
        itemListExtra << digitizerItem;
        itemListExtra << new QStandardItem(digitizerItem->toolTip());
        this->appendRow(itemListExtra);
    }
}

//=============================================================================================================

void DigitizerSetTreeItem::setTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->setTransform(transform);
    }
}

//=============================================================================================================

void DigitizerSetTreeItem::setTransform(const FiffCoordTrans& transform,
                                        bool bApplyInverse)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->setTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void DigitizerSetTreeItem::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->applyTransform(transform);
    }
}

//=============================================================================================================

void DigitizerSetTreeItem::applyTransform(const FiffCoordTrans& transform,
                                          bool bApplyInverse)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->applyTransform(transform, bApplyInverse);
    }
}
