//=============================================================================================================
/**
* @file     sensorsettreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    SensorSetTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorsettreeitem.h"
#include "sensorsurfacetreeitem.h"
#include "sensorpositiontreeitem.h"

#include <mne/mne_bem.h>
#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorSetTreeItem::SensorSetTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}


//*************************************************************************************************************

void SensorSetTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Sensor item");
}


//*************************************************************************************************************

void SensorSetTreeItem::addData(const MNEBem &tSensor, const QList<FiffChInfo>& lChInfo, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate sensor surfaces as childs
    for(int i = 0; i < tSensor.size(); ++i) {
        SensorSurfaceTreeItem* pSurfItem = new SensorSurfaceTreeItem(p3DEntityParent, Data3DTreeModelItemTypes::SensorSurfaceItem);
        pSurfItem->addData(tSensor[i]);

        QList<QStandardItem*> list;
        list << pSurfItem;
        list << new QStandardItem(pSurfItem->toolTip());
        this->appendRow(list);
    }

    //Sort MEG channel types
    QList<FiffChInfo> lChInfoGrad;
    QList<FiffChInfo> lChInfoMag;

    for(int i = 0; i < lChInfo.size(); ++i) {
        if(lChInfo.at(i).unit == FIFF_UNIT_T_M) {
            lChInfoGrad << lChInfo.at(i);
        } else if(lChInfo.at(i).unit == FIFF_UNIT_T) {
            lChInfoMag << lChInfo.at(i);
        }
    }

    //Add sensor locations as child items
    if(!lChInfoGrad.isEmpty()) {
        SensorPositionTreeItem* pSensorPosItem = new SensorPositionTreeItem(p3DEntityParent, Data3DTreeModelItemTypes::SensorPositionItem, "Grad");
        pSensorPosItem->addData(lChInfoGrad);

        QList<QStandardItem*> list;
        list << pSensorPosItem;
        list << new QStandardItem(pSensorPosItem->toolTip());
        this->appendRow(list);
    }

    if(!lChInfoMag.isEmpty()) {
        SensorPositionTreeItem* pSensorPosItem = new SensorPositionTreeItem(p3DEntityParent, Data3DTreeModelItemTypes::SensorPositionItem, "Mag");
        pSensorPosItem->addData(lChInfoMag);

        QList<QStandardItem*> list;
        list << pSensorPosItem;
        list << new QStandardItem(pSensorPosItem->toolTip());
        this->appendRow(list);
    }
}


//*************************************************************************************************************

void SensorSetTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i < this->rowCount(); ++i) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }
}



