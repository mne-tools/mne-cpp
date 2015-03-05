//=============================================================================================================
/**
* @file     stcdatamodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2015
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
* @brief    Implementation of the StcDataModel class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stcdatamodel.h"
#include <mne/mne_sourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QColor>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StcDataModel::StcDataModel(QObject *parent)
: QAbstractTableModel(parent)
, m_pWorker(new StcDataWorker)
, m_bRTMode(true)
, m_bModelInit(false)
, m_bDataInit(false)
, m_bIntervallSet(false)
, m_dStcNormMax(10.0)
, m_dStcNorm(1)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");
    qRegisterMetaType<VectorXd>("VectorXd");
    qRegisterMetaType<Matrix3Xf>("Matrix3Xf");

    connect(m_pWorker.data(), &StcDataWorker::stcSample, this, &StcDataModel::setStcSample);

//    m_pWorker->setLoop(true);

}


//*************************************************************************************************************

StcDataModel::~StcDataModel()
{
}


//*************************************************************************************************************
//virtual functions
int StcDataModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(m_vecCurStc.rows() != 0)
        return m_vecCurStc.rows();
    else
        return 0;
}


//*************************************************************************************************************

int StcDataModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 7;
}


//*************************************************************************************************************

QVariant StcDataModel::data(const QModelIndex &index, int role) const
{
//    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
//        return QVariant();

    if (index.isValid()) {
        qint32 row = index.row();

        switch(index.column()) {
            case 0: { // index
                if(role == Qt::DisplayRole)
                    return QVariant(row);
                break;
            }
            case 1: { // vertex index
//                std::cout<<"StcDataModel::data() - LH - vertno.rows(): "<<m_forwardSolution.src[0].vertno.rows()<<std::endl;
//                std::cout<<"StcDataModel::data() - RH - vertno.rows(): "<<m_forwardSolution.src[1].vertno.rows()<<std::endl;

                if(m_bDataInit && role == StcDataModelRoles::GetIndexLH)
                    return QVariant(m_forwardSolution.src[0].vertno(row));
                    //return QVariant(m_qListLabels[row].label_id);//m_vertLabelIds(row));

                if(m_bDataInit && role == StcDataModelRoles::GetIndexRH)
                    return QVariant(m_forwardSolution.src[1].vertno(row));

                break;
            }
            case 2: // stc data
            {
                int numDipolesLH = m_forwardSolution.src[0].vertno.rows();
                int numDipolesRH = m_forwardSolution.src[1].vertno.rows();

                //std::cout<<"StcDataModel::data() - numDipolesLH: "<<numDipolesLH<<std::endl;
                //std::cout<<"StcDataModel::data() - numDipolesRH: "<<numDipolesRH<<std::endl;

                QVariant v;
                if(m_bDataInit && role == StcDataModelRoles::GetStcValLH) {
                    VectorXd valVec(numDipolesLH);

                    for(qint32 i = 0; i < valVec.rows(); ++i)
                        valVec(i) = m_vecCurStc(i);

                    v.setValue(valVec);
                }

                if(m_bDataInit && role == StcDataModelRoles::GetStcValRH) {
                    VectorXd valVec(numDipolesRH);

                    for(qint32 i = numDipolesLH; i < numDipolesLH+numDipolesRH; ++i)
                        valVec(i-numDipolesLH) = m_vecCurStc(i);

                    v.setValue(valVec);
                }

                return v;
            }
            case 3: // relative stc data
            {
                int numDipolesLH = m_forwardSolution.src[0].vertno.rows();
                int numDipolesRH = m_forwardSolution.src[1].vertno.rows();

                //std::cout<<"StcDataModel::data() - numDipolesLH: "<<numDipolesLH<<std::endl;
                //std::cout<<"StcDataModel::data() - numDipolesRH: "<<numDipolesRH<<std::endl;
                //std::cout<<"StcDataModel::data() - m_bDataInit: "<<m_bDataInit<<std::endl;

                QVariant v;
                if(m_bDataInit && role == StcDataModelRoles::GetRelStcValLH) {
                    VectorXd valVec(numDipolesLH);

                    for(qint32 i = 0; i < valVec.rows(); ++i)
                        valVec(i) = m_vecCurRelStc(i);

                    //std::cout<<"StcDataModel::data() - LH - valVec.rows(): "<<valVec.rows()<<std::endl;

                    v.setValue(valVec);
                }

                if(m_bDataInit && role == StcDataModelRoles::GetRelStcValRH) {
                    VectorXd valVec(numDipolesRH);

                    for(qint32 i = numDipolesLH; i < numDipolesLH+numDipolesRH; ++i)
                        valVec(i-numDipolesLH) = m_vecCurRelStc(i);

                    //std::cout<<"StcDataModel::data() - RH - valVec.rows(): "<<valVec.rows()<<std::endl;

                    v.setValue(valVec);
                }

                return v;

//                if(m_bDataInit && role == Qt::DisplayRole)
//                {
//                    //Cluster version
//                    QList<qint32> selVec;
//                    QVariant v;
//                    if(row < m_iLHSize)
//                        selVec = m_qMapLabelIdChannelLH.values(m_qListLabels[row].label_id);
//                    else
//                        selVec = m_qMapLabelIdChannelRH.values(m_qListLabels[row].label_id);

//                    VectorXd valVec(selVec.size());

//                    if(index.column() == 2) //stc data
//                        for(qint32 i = 0; i < selVec.size(); ++i)
//                            valVec(i) = m_vecCurStc(selVec[i]);
//                    else // relative stc data
//                        for(qint32 i = 0; i < selVec.size(); ++i)
//                            valVec(i) = m_vecCurRelStc(selVec[i]);

//                    v.setValue(valVec);
//                    return v;//m_vecCurStc(row));
//                }
                break;
            }
            case 4: { // Label
                if(role == Qt::DisplayRole)
                {
                    QVariant v;
                    v.setValue(m_qListLabels[row]);
                    return v;
                }
                break;
            }
            case 5: { // Color
                if(role == Qt::DisplayRole)
                    return QVariant(QColor(m_qListRGBAs[row](0),m_qListRGBAs[row](1),m_qListRGBAs[row](2),255));
                break;
            }
            case 6: { // Tri RRs
                if(role == Qt::DisplayRole)
                {
                    QVariant v;
                    v.setValue(m_qListTriRRs[row]);
                    return v;
                }
                break;
            }
        }
    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant StcDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)// && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0: //index column
                return QVariant("Index");
            case 1: //vertex column
                return QVariant("Vertex/Label ID");
            case 2: //stc data column
                return QVariant("STC");
            case 3: //realtive stc data column
                return QVariant("Relative STC");
            case 4: //roi name column
                return QVariant("Label");
            case 5: //roi color column
                return QVariant("Color");
            case 6: //roi Tri Coords
                return QVariant("Tri Coords");
        }
    }
    else if(orientation == Qt::Vertical) {
        QModelIndex mdlIdx = createIndex(section,0);
        switch(role) {
            case Qt::DisplayRole:
                return QVariant(data(mdlIdx).toString());
            }
    }

    return QVariant();
}


//*************************************************************************************************************

void StcDataModel::addData(const MNESourceEstimate &stc)
{
    //std::cout<<"START - StcDataModel::addData()"<<std::endl;

    if(!m_bModelInit || stc.isEmpty()) {
        std::cout<<"stc is empty or model was not correctly initialized"<<std::endl;
        return;
    }

    if(m_vertLabelIds.size() != stc.data.rows())
    {
        //TODO MAP data 416 to labels 150!!!!!!!!
        //ToDo Map the indices to the regions
        setVertLabelIDs(stc.vertices);

    }


    QList<VectorXd> data;
    for(qint32 i = 0; i < stc.data.cols(); ++i)
        data.append(stc.data.col(i));

//    if(!m_bIntervallSet)
//    {
//        int usec = floor(stc.tstep*1000000);
//        if(usec <= 0)
//            return;
//        m_pWorker->setInterval(usec);
//        m_bIntervallSet = true;
//    }

    m_pWorker->addData(data);

    m_bDataInit = true;

    //std::cout<<"END - StcDataModel::addData()"<<std::endl;
}


//*************************************************************************************************************

void StcDataModel::init(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, const QString &atlas, const MNEForwardSolution &forwardSolution)
{
    beginResetModel();
    m_forwardSolution = forwardSolution;
//    SurfaceSet(subject_id, hemi, surf, subjects_dir);
//    m_surfSet = SurfaceSet(subject_id, hemi, surf, subjects_dir);
//    m_annotationSet = AnnotationSet(subject_id, hemi, atlas, subjects_dir);
//    m_annotationSet.toLabels(m_surfSet, m_qListLabels, m_qListRGBAs);

//    float lhOffset = 0;
//    float rhOffset = 0;
//    // MIN MAX
//    for(qint32 h = 0; h < m_annotationSet.size(); ++h)
//    {
//        if(h == 0)
//        {
//            if(QString::compare(m_surfSet.surf(),"inflated") == 0)
//                lhOffset = m_surfSet[h].rr().col(0).maxCoeff(); // X

//            m_vecMinRR.setX(m_surfSet[h].rr().col(0).minCoeff()-lhOffset); // X
//            m_vecMinRR.setY(m_surfSet[h].rr().col(1).minCoeff()); // Y
//            m_vecMinRR.setZ(m_surfSet[h].rr().col(2).minCoeff()); // Z
//            m_vecMaxRR.setX(m_surfSet[h].rr().col(0).maxCoeff()-lhOffset); // X
//            m_vecMaxRR.setY(m_surfSet[h].rr().col(1).maxCoeff()); // Y
//            m_vecMaxRR.setZ(m_surfSet[h].rr().col(2).maxCoeff()); // Z
//        }
//        else
//        {
//            if(QString::compare(m_surfSet.surf(),"inflated") == 0)
//                rhOffset = m_surfSet[h].rr().col(0).maxCoeff(); // X

//            m_vecMinRR.setX(m_vecMinRR.x() < m_surfSet[h].rr().col(0).minCoeff()+rhOffset ? m_vecMinRR.x() : m_surfSet[h].rr().col(0).minCoeff()+rhOffset); // X
//            m_vecMinRR.setY(m_vecMinRR.y() < m_surfSet[h].rr().col(1).minCoeff() ? m_vecMinRR.y() : m_surfSet[h].rr().col(1).minCoeff()); // Y
//            m_vecMinRR.setZ(m_vecMinRR.z() < m_surfSet[h].rr().col(2).minCoeff() ? m_vecMinRR.z() : m_surfSet[h].rr().col(2).minCoeff()); // Z
//            m_vecMaxRR.setX(m_vecMaxRR.x() > m_surfSet[h].rr().col(0).maxCoeff()+rhOffset ? m_vecMaxRR.x() : m_surfSet[h].rr().col(0).maxCoeff()+rhOffset); // X
//            m_vecMaxRR.setY(m_vecMaxRR.y() > m_surfSet[h].rr().col(1).maxCoeff() ? m_vecMaxRR.y() : m_surfSet[h].rr().col(1).maxCoeff()); // Y
//            m_vecMaxRR.setZ(m_vecMaxRR.z() > m_surfSet[h].rr().col(2).maxCoeff() ? m_vecMaxRR.z() : m_surfSet[h].rr().col(2).maxCoeff()); // Z
//        }
//    }

//    QVector3D vecCenterRR;
//    vecCenterRR.setX((m_vecMinRR.x()+m_vecMaxRR.x())/2.0f);
//    vecCenterRR.setY((m_vecMinRR.y()+m_vecMaxRR.y())/2.0f);
//    vecCenterRR.setZ((m_vecMinRR.z()+m_vecMaxRR.z())/2.0f);

//    // Regions -> ToDo QtConcurrent
////    qDebug() << "Before ROI";
//    for(qint32 h = 0; h < m_annotationSet.size(); ++h)
//    {
//        MatrixX3i tris;
//        MatrixX3f rr = m_surfSet[h].rr();

//        //Centralize
//        if(QString::compare(m_surfSet.surf(),"inflated") == 0)
//        {
//            if(h == 0) //X
//                rr.col(0) = (rr.col(0).array() - lhOffset) - vecCenterRR.x();
//            else
//                rr.col(0) = (rr.col(0).array() + rhOffset) - vecCenterRR.x();
//        }
//        else
//            rr.col(0) = rr.col(0).array() - vecCenterRR.x(); // X

//        rr.col(1) = rr.col(1).array() - vecCenterRR.y(); // Y
//        rr.col(2) = rr.col(2).array() - vecCenterRR.z(); // Z


//        //
//        // Create each ROI
//        //
//        for(qint32 k = 0; k < m_qListLabels.size(); ++k)
//        {
//            //check if label hemi fits current hemi
//            if(m_qListLabels[k].hemi != h)
//                continue;

//            //Generate label tri information
//            tris = m_qListLabels[k].selectTris(m_surfSet[h]); //ToDO very slow -> QtConcurrent


//            Matrix3Xf triCoords(3,3*tris.rows());

//            for(qint32 i = 0; i < tris.rows(); ++i)
//            {
//                triCoords.col(i*3) = rr.row( tris(i,0) ).transpose();
//                triCoords.col(i*3+1) = rr.row( tris(i,1) ).transpose();
//                triCoords.col(i*3+2) = rr.row( tris(i,2) ).transpose();
//            }

//            m_qListTriRRs.append(triCoords);
//        }
//    }
//    qDebug() << "After ROI";

//    m_iLHSize = 0;
//    for(qint32 k = 0; k < m_qListLabels.size(); ++k)
//        if(m_qListLabels[k].hemi == 0)
//            ++m_iLHSize;

    m_bModelInit = true;

    endResetModel();

    //start the worker
    m_pWorker->start();
}


//*************************************************************************************************************

void StcDataModel::setAverage(qint32 samples)
{
    m_pWorker->setAverage(samples);
}


//*************************************************************************************************************

void StcDataModel::setLoop(bool looping)
{
    m_pWorker->setLoop(looping);
}


//*************************************************************************************************************

void StcDataModel::setNormalization(qint32 fraction)
{
    m_dStcNorm = (m_dStcNormMax/100.0) * (double)fraction;
}


//*************************************************************************************************************

void StcDataModel::setStcSample(const VectorXd &sample)
{
    //std::cout<<"StcDataModel::setStcSample()"<<std::endl;
    m_vecCurStc = sample;

    m_vecCurRelStc = sample/m_dStcNorm;

    //std::cout<<"StcDataModel::setStcSample() - m_vecCurStc.rows(): "<<m_vecCurStc.rows()<<std::endl;
    //std::cout<<"StcDataModel::setStcSample() - m_vecCurRelStc.rows(): "<<m_vecCurRelStc.rows()<<std::endl;

    //Update data content -> Bug in QTableView which updates the whole table http://qt-project.org/forums/viewthread/14723
    QModelIndex topLeft = this->index(0,2);
    QModelIndex bottomRight = this->index(this->rowCount()-1,3);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);

    //Alternative for Table View -> Do update per item instead
//    QModelIndex topLeft, bottomRight;
//    QVector<int> roles; roles << Qt::DisplayRole;
//    for(qint32 i = 0; i < m_qListLabels.size(); ++i)
//    {
//        topLeft = this->index(i,2);
//        bottomRight = this->index(i,2);
//        emit dataChanged(topLeft, bottomRight, roles);
//        topLeft = this->index(i,3);
//        bottomRight = this->index(i,3);
//        emit dataChanged(topLeft, bottomRight, roles);
//    }
}


//*************************************************************************************************************

void StcDataModel::setVertLabelIDs(const VectorXi &vertLabelIDs)
{
    QMap<qint32, qint32> t_qMapLabelIdChannel;
    for(qint32 i = 0; i < vertLabelIDs.size(); ++i)
        t_qMapLabelIdChannel.insertMulti(vertLabelIDs(i),i);

    QList<qint32> qListLastIdcs = t_qMapLabelIdChannel.values(vertLabelIDs(vertLabelIDs.size() - 1));

    qint32 lhIdx = 0;
    qint32 maxIdx = qListLastIdcs[0];
    qint32 minIdx = qListLastIdcs[0];

    for(qint32 i = 0; i < qListLastIdcs.size(); ++i)
    {
        if(maxIdx < qListLastIdcs[i])
            maxIdx = qListLastIdcs[i];
        if(minIdx > qListLastIdcs[i])
            minIdx = qListLastIdcs[i];
    }

    qint32 upperBound = maxIdx - (maxIdx*0.25);
    for(qint32 i = 0; i < qListLastIdcs.size(); ++i)
    {
        if(lhIdx < qListLastIdcs[i] && qListLastIdcs[i] < upperBound)
            lhIdx = qListLastIdcs[i];
    }

    m_qMapLabelIdChannelLH.clear();
    m_qMapLabelIdChannelRH.clear();


    QMap<qint32, qint32>::iterator it;
    for (it = t_qMapLabelIdChannel.begin(); it != t_qMapLabelIdChannel.end(); ++it)
    {
        if(it.value() <= lhIdx)
            m_qMapLabelIdChannelLH.insertMulti(it.key(),it.value());
        else
            m_qMapLabelIdChannelRH.insertMulti(it.key(),it.value());
    }

    m_vertLabelIds = vertLabelIDs;
}

