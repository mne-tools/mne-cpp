//=============================================================================================================
/**
* @file     brain.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    Brain class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brain.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Brain::Brain(Qt3DCore::QEntity *parent)
: Qt3DCore::QEntity(parent)
, m_pBrainTreeModel(new BrainTreeModel(this))
{
}


//*************************************************************************************************************

Brain::~Brain()
{
}


//*************************************************************************************************************

bool Brain::addData(const QString & text, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet)
{
    return m_pBrainTreeModel->addData(text, tSurfaceSet, tAnnotationSet, this);
}


//*************************************************************************************************************

bool Brain::addData(const QString & text, const Surface &tSurface, const Annotation &tAnnotation)
{
    return m_pBrainTreeModel->addData(text, tSurface, tAnnotation, this);
}


//*************************************************************************************************************

bool Brain::addData(const QString & text, const MNESourceEstimate & tSourceEstimate, const MNEForwardSolution & tForwardSolution)
{
    return m_pBrainTreeModel->addData(text, tSourceEstimate, tForwardSolution);
}


//*************************************************************************************************************

BrainTreeModel* Brain::getBrainTreeModel()
{
    return m_pBrainTreeModel;
}


//*************************************************************************************************************

void Brain::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    qDebug()<<"Brain::dataChanged";

    QList<QStandardItem*> itemList = m_pBrainTreeModel->findItems("Surface", Qt::MatchContains);

    for(int i = 0; i<itemList.size(); i++)
        qDebug()<<itemList.at(i)->text();

//    int stcDataRoleLH = StcDataModelRoles::GetSmoothedStcValLH;
//    int stcDataRoleRH = StcDataModelRoles::GetSmoothedStcValRH;

//    //check wether realtive stc data column (3) has changed
//    if(topLeft.column() > 3 || bottomRight.column() < 3) {
//        std::cout<<"Brain::dataChanged() - stc data did not change"<<std::endl;
//        return;
//    }

//    //Get LH activation and transform to index/color map
//    const StcDataModel* model = static_cast<const StcDataModel*>(topLeft.model());
//    VectorXd currentActivationLH = model->data(0,4,stcDataRoleLH).value<VectorXd>();

//    MatrixX3f matVertexActivationColorLH = m_qmDefaultVertexColorLH;

//    std::cout<<"BrainSurface::dataChanged() - currentActivationLH.rows(): "<<currentActivationLH.rows()<<std::endl;

//    for(qint32 i = 0; i < currentActivationLH.rows(); ++i) {
//        qint32 iVal = currentActivationLH(i) * 20;

//        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

//        //std::cout<<(int)iVal<<std::endl;

//        QRgb qRgb;
////        qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);
//        qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);
////        qRgb = ColorMap::valueToHot((float)iVal/255.0);

//        int vertexIndex = i;
//        if(stcDataRoleLH == StcDataModelRoles::GetRelStcValLH || stcDataRoleLH == StcDataModelRoles::GetStcValLH)
//            vertexIndex = model->data(i,1,StcDataModelRoles::GetIndexLH).toInt();

////        std::cout<<"BrainSurface::dataChanged() - vertexIndex: "<<vertexIndex<<std::endl;
////        std::cout<<"BrainSurface::dataChanged() - qRgb: "<<QColor(qRgb).redF()<<" "<<QColor(qRgb).greenF()<<" "<<QColor(qRgb).blueF()<<std::endl;

//        if(iVal>150) {
//            QColor rgb = QColor(qRgb);
//            matVertexActivationColorLH(i, 0) = rgb.red();
//            matVertexActivationColorLH(i, 1) = rgb.green();
//            matVertexActivationColorLH(i, 2) = rgb.blue();
//        }
//    }

//    //Get RH activation and transform to index/color map
//    VectorXd currentActivationRH = model->data(0,4,stcDataRoleRH).value<VectorXd>();

//    MatrixX3f  matVertexActivationColorRH = m_qmDefaultVertexColorRH;

//    for(qint32 i = 0; i < currentActivationRH.rows(); ++i) {
//        qint32 iVal = currentActivationRH(i) * 20;

//        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

//        QRgb qRgb;
////        qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);
//        qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);
////        qRgb = ColorMap::valueToHot((float)iVal/255.0);

//        int vertexIndex = i;
//        if(stcDataRoleLH == StcDataModelRoles::GetRelStcValRH || stcDataRoleLH == StcDataModelRoles::GetStcValRH)
//            vertexIndex = model->data(i,1,StcDataModelRoles::GetIndexRH).toInt();

//        if(iVal>150) {
//            QColor rgb = QColor(qRgb);
//            matVertexActivationColorRH(i, 0) = rgb.red();
//            matVertexActivationColorRH(i, 1) = rgb.green();
//            matVertexActivationColorRH(i, 2) = rgb.blue();
//        }
//    }

//    updateActivation();

//    Q_UNUSED(roles);

    //std::cout<<"END - BrainSurface::dataChanged()"<<std::endl;
}

