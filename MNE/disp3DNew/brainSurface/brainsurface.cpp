//=============================================================================================================
/**
* @file     brainsurface.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
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
* @brief    Implementation of BrainSurface.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurface.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurface::BrainSurface(QEntity *parent)
: RenderableEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
, m_ColorSulci(QColor(50.0, 50.0, 50.0, 255.0))
, m_ColorGyri(QColor(200.0, 200.0, 200.0, 255.0))
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QEntity *parent)
: RenderableEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
, m_ColorSulci(QColor(50.0, 50.0, 50.0, 255.0))
, m_ColorGyri(QColor(100.0, 100.0, 100.0, 255.0))
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QEntity *parent)
: RenderableEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_AnnotationSet(subject_id, hemi, atlas, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
, m_ColorSulci(QColor(50.0, 50.0, 50.0, 255.0))
, m_ColorGyri(QColor(200.0, 200.0, 200.0, 255.0))
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString& p_sFile, QEntity *parent)
: RenderableEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
, m_ColorSulci(QColor(50.0, 50.0, 50.0, 255.0))
, m_ColorGyri(QColor(200.0, 200.0, 200.0, 255.0))
{
    Surface t_Surf(p_sFile);
    m_SurfaceSet.insert(t_Surf);

    init();
}


//*************************************************************************************************************

BrainSurface::~BrainSurface()
{

}


//*************************************************************************************************************

void BrainSurface::updateActivation()
{
    //std::cout<<"START - BrainSurface::updateActivation()"<<std::endl;

    // LEFT HEMISPHERE
    //Find brain mesh as component
//    int indexSurfaceMeshLeftHemi;
//    QComponentList componentsListLeftHemi = m_pLeftHemisphere->components();
//    for(int i = 0; i<componentsListLeftHemi.size(); i++)
//        if(componentsListLeftHemi.at(i)->objectName() == "m_pSurfaceMesh")
//            indexSurfaceMeshLeftHemi = i;

//    //Remove mesh component
//    m_pLeftHemisphere->removeComponent(componentsListLeftHemi.at(indexSurfaceMeshLeftHemi));

//    //Create new mesh component
//    if(m_pLeftHemisphere)
//        delete m_pLeftHemisphere;

//    m_pLeftHemisphere = new BrainHemisphere(m_SurfaceSet[0], m_qmVertexActivationColorLH, this);

//    BrainSurfaceMesh *meshLeftHemi = (BrainSurfaceMesh*)componentsListLeftHemi.at(indexSurfaceMeshLeftHemi);

    if(m_pLeftHemisphere)
        m_pLeftHemisphere->updateActivation(m_qmVertexActivationColorLH);

    // RIGHT HEMISPHERE
    //Find brain mesh as component
//    int indexSurfaceMeshRightHemi;
//    QComponentList componentsListRightHemi = m_pRightHemisphere->components();
//    for(int i = 0; i<componentsListRightHemi.size(); i++)
//        if(componentsListRightHemi.at(i)->objectName() == "m_pSurfaceMesh")
//            indexSurfaceMeshRightHemi = i;

//    //Remove mesh component
//    m_pRightHemisphere->removeComponent(componentsListRightHemi.at(indexSurfaceMeshRightHemi));

//    //Create new mesh component
//    if(m_pRightHemisphere)
//        delete m_pRightHemisphere;

//    m_pRightHemisphere = new BrainHemisphere(m_SurfaceSet[1], m_qmVertexActivationColorRH, this);

//    BrainSurfaceMesh *meshRightHemi = (BrainSurfaceMesh*)componentsListRightHemi.at(indexSurfaceMeshRightHemi);

    if(m_pRightHemisphere)
        m_pRightHemisphere->updateActivation(m_qmVertexActivationColorRH);

    //std::cout<<"END - BrainSurface::updateActivation()"<<std::endl;
}


//*************************************************************************************************************

void BrainSurface::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    //std::cout<<"START - BrainSurface::dataChanged()"<<std::endl;

    //std::cout<<"BrainSurface::dataChanged() - topLeft.column(): "<<topLeft.column()<<std::endl;
    //std::cout<<"BrainSurface::dataChanged() - bottomRight.column(): "<<bottomRight.column()<<std::endl;

    int stcDataRoleLH = StcDataModelRoles::GetSmoothedStcValLH;
    int stcDataRoleRH = StcDataModelRoles::GetSmoothedStcValRH;

    //check wether realtive stc data column (3) has changed
    if(topLeft.column() > 3 || bottomRight.column() < 3) {
        std::cout<<"BrainSurface::dataChanged() - stc data did not change"<<std::endl;
        return;
    }

    //Get LH activation and transform to index/color map
    VectorXd currentActivationLH = m_pStcDataModel->data(0,4,stcDataRoleLH).value<VectorXd>();

    m_qmVertexActivationColorLH = m_qmDefaultVertexColorLH;

    std::cout<<"BrainSurface::dataChanged() - currentActivationLH.rows(): "<<currentActivationLH.rows()<<std::endl;

    for(qint32 i = 0; i < currentActivationLH.rows(); ++i) {
        qint32 iVal = currentActivationLH(i) * 20;

        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

        //std::cout<<(int)iVal<<std::endl;

        QRgb qRgb;
//        qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);
        qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);
//        qRgb = ColorMap::valueToHot((float)iVal/255.0);

        int vertexIndex = i;
        if(stcDataRoleLH == StcDataModelRoles::GetRelStcValLH || stcDataRoleLH == StcDataModelRoles::GetStcValLH)
            vertexIndex = m_pStcDataModel->data(i,1,StcDataModelRoles::GetIndexLH).toInt();

//        std::cout<<"BrainSurface::dataChanged() - vertexIndex: "<<vertexIndex<<std::endl;
//        std::cout<<"BrainSurface::dataChanged() - qRgb: "<<QColor(qRgb).redF()<<" "<<QColor(qRgb).greenF()<<" "<<QColor(qRgb).blueF()<<std::endl;

        if(iVal>0)
            m_qmVertexActivationColorLH[vertexIndex] = QColor(qRgb);
    }

    //Get RH activation and transform to index/color map
    VectorXd currentActivationRH = m_pStcDataModel->data(0,4,stcDataRoleRH).value<VectorXd>();

    m_qmVertexActivationColorRH = m_qmDefaultVertexColorRH;

    for(qint32 i = 0; i < currentActivationRH.rows(); ++i) {
        qint32 iVal = currentActivationRH(i) * 20;

        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

        QRgb qRgb;
//        qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);
        qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);
//        qRgb = ColorMap::valueToHot((float)iVal/255.0);

        int vertexIndex = i;
        if(stcDataRoleLH == StcDataModelRoles::GetRelStcValRH || stcDataRoleLH == StcDataModelRoles::GetStcValRH)
            vertexIndex = m_pStcDataModel->data(i,1,StcDataModelRoles::GetIndexRH).toInt();

        if(iVal>0)
            m_qmVertexActivationColorRH[vertexIndex] = QColor(qRgb);
    }

    updateActivation();

    Q_UNUSED(roles);

    //std::cout<<"END - BrainSurface::dataChanged()"<<std::endl;
}


//*************************************************************************************************************

void BrainSurface::setModel(StcDataModel::SPtr model)
{
    m_pStcDataModel = model;

    //connect stc data model with surface coloring
    connect(m_pStcDataModel.data(), &StcDataModel::dataChanged, this, &BrainSurface::dataChanged);
}


//*************************************************************************************************************

void BrainSurface::init()
{
    //Init sulci gyri colors for LH and RH
    m_qmDefaultVertexColorLH.clear();
    for(int i = 0; i<m_SurfaceSet[0].rr().rows() ; i++)
        if(m_SurfaceSet[0].curv()[i] >= 0)
            m_qmDefaultVertexColorLH.insertMulti(i, m_ColorSulci); //Sulci
        else
            m_qmDefaultVertexColorLH.insertMulti(i, m_ColorGyri); //Gyri

    m_qmDefaultVertexColorRH.clear();
    for(int i = 0; i<m_SurfaceSet[1].rr().rows() ; i++)
        if(m_SurfaceSet[1].curv()[i] >= 0)
            m_qmDefaultVertexColorRH.insertMulti(i, m_ColorSulci); //Sulci
        else
            m_qmDefaultVertexColorRH.insertMulti(i, m_ColorGyri); //Gyri

    m_qmVertexActivationColorLH = m_qmDefaultVertexColorLH;
    m_qmVertexActivationColorRH = m_qmDefaultVertexColorRH;

    //Create hemispheres and add as childs / 0 -> lh, 1 -> rh
    for(int i = 0; i<m_SurfaceSet.size(); i++) {
        if(m_SurfaceSet[i].hemi() == 0)
            m_pLeftHemisphere = new BrainHemisphere(m_SurfaceSet[i], m_qmVertexActivationColorLH, this);

        if(m_SurfaceSet[i].hemi() == 1)
            m_pRightHemisphere = new BrainHemisphere(m_SurfaceSet[i], m_qmVertexActivationColorRH, this);
    }

    // Calculate bounding box
    calcBoundingBox();

    // Brain initial surface Transform
    int scale = 300;
    this->scaleTransform()->setScale(scale);

    //Brain initial rotation
    this->rotateTransform()->setAxis(QVector3D(1,0,0));
    this->rotateTransform()->setAngleDeg(-90);
}


//*************************************************************************************************************

void BrainSurface::calcBoundingBox()
{
    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();

    QVector3D min(it.value().rr().col(0).minCoeff(), it.value().rr().col(1).minCoeff(), it.value().rr().col(2).minCoeff());
    QVector3D max(it.value().rr().col(0).maxCoeff(), it.value().rr().col(1).maxCoeff(), it.value().rr().col(2).maxCoeff());

    for (it = m_SurfaceSet.data().begin()+1; it != m_SurfaceSet.data().end(); ++it)
    {
        for(qint32 i = 0; i < 3; ++i)
        {
            min[i] = min[i] > it.value().rr().col(i).minCoeff() ? it.value().rr().col(i).minCoeff() : min[i];
            max[i] = max[i] < it.value().rr().col(i).maxCoeff() ? it.value().rr().col(i).maxCoeff() : max[i];
        }
    }
    m_vecBoundingBoxMin = min;
    m_vecBoundingBoxMax = max;

    for(qint32 i = 0; i < 3; ++i)
        m_vecBoundingBoxCenter[i] = (m_vecBoundingBoxMin[i]+m_vecBoundingBoxMax[i])/2.0f;
}
