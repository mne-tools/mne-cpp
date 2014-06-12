#include "stcmodel.h"

#include <mne/mne_sourceestimate.h>

#include <QDebug>
#include <QColor>

#include <iostream>



using namespace MNELIB;


StcModel::StcModel(QObject *parent)
: QAbstractTableModel(parent)
, m_pThread(new QThread)
, m_pWorker(new StcWorker)
, m_bRTMode(true)
, m_bIsInit(false)
, m_bIntervallSet(false)
, m_iDownSampling(20)
, m_iCurrentSample(0)
, m_dStcNormMax(10.0)
, m_dStcNorm(1.0)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");
    qRegisterMetaType<VectorXd>("VectorXd");
    qRegisterMetaType<Matrix3Xf>("Matrix3Xf");

    m_pWorker->moveToThread(m_pThread.data());
    connect(m_pThread.data(), &QThread::started, m_pWorker.data(), &StcWorker::process);
    connect(m_pWorker.data(), &StcWorker::stcSample, this, &StcModel::setStcSample);

    m_pWorker->setLoop(true);

    m_pThread->start();

}


//*************************************************************************************************************
//virtual functions
int StcModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qListLabels.empty())
        return m_qListLabels.size();
    else
        return 0;
}


//*************************************************************************************************************

int StcModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 7;
}


//*************************************************************************************************************

QVariant StcModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if (index.isValid()) {
        qint32 row = index.row();

        switch(index.column()) {
            case 0: { // index
                if(role == Qt::DisplayRole)
                    return QVariant(row);
                break;
            }
            case 1: { // vertex
                if(role == Qt::DisplayRole)
                    return QVariant(m_vertices(row));
                break;
            }
            case 2: { // stc data
                if(m_bIsInit && role == Qt::DisplayRole)
                    return QVariant(m_vecCurStc(row));
                break;
            }
            case 3: { // relative stc data
                if(m_bIsInit && role == Qt::DisplayRole)
                    return QVariant(m_vecCurRelStc(row));
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

QVariant StcModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)// && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0: //index column
                return QVariant("Index");
            case 1: //vertex column
                return QVariant("Vertex");
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

void StcModel::addData(const MNESourceEstimate &stc)
{
//    qDebug() << "addData currentThread" << QThread::currentThread();

    if(!m_bIsInit)
        return;

//    qDebug() << "m_vertices.size()" << m_vertices.size();

    if(m_vertices.size() != stc.data.rows())
        setVertices(stc.vertices);

    //Downsampling ->ToDo make this more accurate
    qint32 i;
    QList<VectorXd> data;
    for(i = m_iCurrentSample; i < stc.data.cols(); i += m_iDownSampling)
        data.append(stc.data.col(i));

    m_pWorker->addData(data);

    if(!m_bIntervallSet)
    {
        int usec = floor(stc.tstep*1000000);
        m_pWorker->setInterval(usec);
        m_bIntervallSet = true;
    }

    //store for next buffer
    m_iCurrentSample = i - stc.data.cols();
}


//*************************************************************************************************************

void StcModel::init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet)
{
    beginResetModel();
    m_annotationSet = annotationSet;
    m_surfSet = surfSet;
    m_annotationSet.toLabels(m_surfSet, m_qListLabels, m_qListRGBAs);

    // MIN MAX
    for(qint32 h = 0; h < m_annotationSet.size(); ++h)
    {
        if(h == 0)
        {
            m_vecMinRR.setX(m_surfSet[h].rr().col(0).minCoeff()); // X
            m_vecMinRR.setY(m_surfSet[h].rr().col(1).minCoeff()); // Y
            m_vecMinRR.setZ(m_surfSet[h].rr().col(2).minCoeff()); // Z
            m_vecMaxRR.setX(m_surfSet[h].rr().col(0).maxCoeff()); // X
            m_vecMaxRR.setY(m_surfSet[h].rr().col(1).maxCoeff()); // Y
            m_vecMaxRR.setZ(m_surfSet[h].rr().col(2).maxCoeff()); // Z
        }
        else
        {
            m_vecMinRR.setX(m_vecMinRR.x() < m_surfSet[h].rr().col(0).minCoeff() ? m_vecMinRR.x() : m_surfSet[h].rr().col(0).minCoeff()); // X
            m_vecMinRR.setY(m_vecMinRR.y() < m_surfSet[h].rr().col(1).minCoeff() ? m_vecMinRR.y() : m_surfSet[h].rr().col(1).minCoeff()); // Y
            m_vecMinRR.setZ(m_vecMinRR.z() < m_surfSet[h].rr().col(2).minCoeff() ? m_vecMinRR.z() : m_surfSet[h].rr().col(2).minCoeff()); // Z
            m_vecMaxRR.setX(m_vecMaxRR.x() > m_surfSet[h].rr().col(0).maxCoeff() ? m_vecMaxRR.x() : m_surfSet[h].rr().col(0).maxCoeff()); // X
            m_vecMaxRR.setY(m_vecMaxRR.y() > m_surfSet[h].rr().col(1).maxCoeff() ? m_vecMaxRR.y() : m_surfSet[h].rr().col(1).maxCoeff()); // Y
            m_vecMaxRR.setZ(m_vecMaxRR.z() > m_surfSet[h].rr().col(2).maxCoeff() ? m_vecMaxRR.z() : m_surfSet[h].rr().col(2).maxCoeff()); // Z
        }
    }

    QVector3D vecCenterRR;
    vecCenterRR.setX((m_vecMinRR.x()+m_vecMaxRR.x())/2.0f);
    vecCenterRR.setY((m_vecMinRR.y()+m_vecMaxRR.y())/2.0f);
    vecCenterRR.setZ((m_vecMinRR.z()+m_vecMaxRR.z())/2.0f);

    // Regions
    for(qint32 h = 0; h < m_annotationSet.size(); ++h)
    {
        MatrixX3i tris;
        MatrixX3f rr = m_surfSet[h].rr();

        //Centralize
        rr.col(0) = rr.col(0).array() - vecCenterRR.x();
        rr.col(1) = rr.col(1).array() - vecCenterRR.y();
        rr.col(2) = rr.col(2).array() - vecCenterRR.z();

        //
        // Create each ROI
        //
        for(qint32 k = 0; k < m_qListLabels.size(); ++k)
        {
            //check if label hemi fits current hemi
            if(m_qListLabels[k].hemi != h)
                continue;

            //Ggenerate label tri information
            tris = m_qListLabels[k].selectTris(m_surfSet[h]);


            Matrix3Xf triCoords(3,3*tris.rows());

            for(qint32 i = 0; i < tris.rows(); ++i)
            {
                triCoords.col(i*3) = rr.row( tris(i,0) ).transpose();
                triCoords.col(i*3+1) = rr.row( tris(i,1) ).transpose();
                triCoords.col(i*3+2) = rr.row( tris(i,2) ).transpose();
            }

            m_qListTriRRs.append(triCoords);
        }
    }



    m_vecCurStc = VectorXd::Zero(m_qListLabels.size());
    m_vecCurRelStc = VectorXd::Zero(m_qListLabels.size());;
    endResetModel();
    m_bIsInit = true;
}


//*************************************************************************************************************

void StcModel::setAverage(qint32 samples)
{
    m_pWorker->setAverage(samples);
}


//*************************************************************************************************************

void StcModel::setNormalization(qint32 fraction)
{
    m_dStcNorm = (m_dStcNormMax/100.0) * (double)fraction;
}


//*************************************************************************************************************

void StcModel::setStcSample(const VectorXd &sample)
{
    m_vecCurStc = sample;

    m_vecCurRelStc = sample/m_dStcNorm;

    //Update data content -> Bug in QTableView which updates the whole table http://qt-project.org/forums/viewthread/14723
    QModelIndex topLeft = this->index(0,2);
    QModelIndex bottomRight = this->index(m_qListLabels.size()-1,3);
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

void StcModel::setVertices(const VectorXi &vertnos)
{
    m_vertices = vertnos;
}

