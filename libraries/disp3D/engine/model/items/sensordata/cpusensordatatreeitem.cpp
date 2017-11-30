//=============================================================================================================
/**
* @file     cpusensordatatreeitem.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    CpuSensorDataTreeItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cpusensordatatreeitem.h"
#include "cpuinterpolationitem.h"
#include "../../workers/rtSensorData/rtsensordatacontroller.h"
#include <mne/mne_bem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISP3DLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CpuSensorDataTreeItem::CpuSensorDataTreeItem(int iType, const QString &text)
: SensorDataTreeItem(iType, text)
{
    connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newRtSmoothedDataAvailable,
            this, &CpuSensorDataTreeItem::onNewRtSmoothedData);
}


//*************************************************************************************************************

void CpuSensorDataTreeItem::initInterpolationItem(const MNEBemSurface &bemSurface,
                                                  Qt3DCore::QEntity* p3DEntityParent)
{
    //create new Tree Item
    if(!m_pInterpolationItem)
    {
        m_pInterpolationItem = new CpuInterpolationItem(p3DEntityParent,
                                                        Data3DTreeModelItemTypes::CpuInterpolationItem,
                                                        QStringLiteral("3D Plot"));
        m_pInterpolationItem->initData(bemSurface);

        QList<QStandardItem*> list;
        list << m_pInterpolationItem;
        list << new QStandardItem(m_pInterpolationItem->toolTip());
        this->appendRow(list);
    }
}


//*************************************************************************************************************

void CpuSensorDataTreeItem::onNewRtSmoothedData(const MatrixX3f &matColorMatrix)
{
    if(m_pInterpolationItem)
    {
        QVariant data;
        data.setValue(matColorMatrix);
        m_pInterpolationItem->setVertColor(data);
    }
}
