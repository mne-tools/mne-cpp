//=============================================================================================================
/**
* @file     gpusensordatatreeitem.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
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
* @brief    GpuSensorDataTreeItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpusensordatatreeitem.h"
#include "../common/gpuinterpolationitem.h"
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

using namespace DISP3DLIB;
using namespace Eigen;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GpuSensorDataTreeItem::GpuSensorDataTreeItem(int iType, const QString &text)
: SensorDataTreeItem(iType,text)
{
    connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newInterpolationMatrixAvailable,
                    this, &GpuSensorDataTreeItem::onNewInterpolationMatrixAvailable);

    connect(m_pSensorRtDataWorkController.data(), &RtSensorDataController::newRtRawDataAvailable,
            this, &GpuSensorDataTreeItem::onNewRtRawData);

    m_pSensorRtDataWorkController->setStreamSmoothedData(false);
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::initInterpolationItem(const MatrixX3f &matVertices,
                                                  const MatrixX3f &matNormals,
                                                  const MatrixX3i &matTriangles,
                                                  Qt3DCore::QEntity *p3DEntityParent)
{
    //create new Tree Item
    if(!m_pInterpolationItem)
    {
        m_pInterpolationItem = new GpuInterpolationItem(p3DEntityParent,
                                                        Data3DTreeModelItemTypes::GpuInterpolationItem,
                                                        QStringLiteral("3D Plot"));
        m_pInterpolationItem->initData(matVertices,
                                       matNormals,
                                       matTriangles);

        QList<QStandardItem*> list;
        list << m_pInterpolationItem;
        list << new QStandardItem(m_pInterpolationItem->toolTip());
        this->appendRow(list);
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onNewInterpolationMatrixAvailable(const Eigen::SparseMatrix<float> &matInterpolationMatrix)
{
    if(m_pInterpolationItem)
    {
        m_pInterpolationItem->setInterpolationMatrix(matInterpolationMatrix);
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onNewRtRawData(const VectorXd &vecDataVector)
{
    if(m_pInterpolationItem)
    {
        m_pInterpolationItem->addNewRtData(vecDataVector.cast<float>());
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onColormapTypeChanged(const QVariant &sColormapType)
{
    if(sColormapType.canConvert<QString>())
    {
        if(m_pInterpolationItem)
        {
            m_pInterpolationItem->setColormapType(sColormapType.toString());
        }
    }
}


//*************************************************************************************************************

void GpuSensorDataTreeItem::onDataThresholdChanged(const QVariant &vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>())
    {
        if(m_pInterpolationItem)
        {
            m_pInterpolationItem->setNormalization(vecThresholds.value<QVector3D>());
        }
    }
}
