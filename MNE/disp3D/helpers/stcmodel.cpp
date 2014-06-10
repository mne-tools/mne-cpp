#include "stcmodel.h"

#include <mne/mne_sourceestimate.h>

#include <QDebug>

#include <iostream>



using namespace MNELIB;


StcModel::StcModel(QObject *parent)
: QAbstractTableModel(parent)
, m_pThread(new QThread)
, m_pWorker(new StcWorker)
, m_bRTMode(true)
, m_bIsInit(false)
, m_iDownsampling(20)
, m_iCurrentSample(0)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");
    qRegisterMetaType<VectorXd>("VectorXd");

    m_pWorker->moveToThread(m_pThread.data());
    connect(m_pThread.data(), &QThread::started, m_pWorker.data(), &StcWorker::process);

    connect(m_pWorker.data(), &StcWorker::stcSample, this, &StcModel::setStcSample);


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
    return 4;
}


//*************************************************************************************************************

QVariant StcModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if (index.isValid()) {
        qint32 row = index.row();

        //******** first column (index) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(row);

        //******** second column (vertex) ********
        if(index.column() == 1)
            return QVariant(m_vertices(row));

        //******** third column (roi name) ********
        if(index.column() == 2)
            return QVariant(m_qListLabels[row].name);

        //******** fourth column (stc data) ********
        if(index.column()==3 && m_bIsInit)
            return QVariant(m_vecCurrentStc(row));

    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant StcModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: //index column
            return QVariant("Index");
        case 1: //vertex column
            return QVariant("Vertex");
        case 2: //roi name column
            return QVariant("ROI Name");
//            switch(role) {
//            case Qt::DisplayRole:
//                return QVariant("data plot");
//            case Qt::TextAlignmentRole:
//                return QVariant(Qt::AlignLeft);
//            }
        case 3: //stc data column
            return QVariant("STC");
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
    qDebug() << "addData currentThread" << QThread::currentThread();

    if(!m_bIsInit)
        return;

//    qDebug() << "m_vertices.size()" << m_vertices.size();

    if(m_vertices.size() != stc.data.rows())
        setVertices(stc.vertices);

    //Downsampling ->ToDo make this more accurate
    qint32 i;
    QList<VectorXd> data;
    for(i = m_iCurrentSample; i < stc.data.cols(); i += m_iDownsampling)
        data.append(stc.data.col(i));

    m_pWorker->addData(data);

//    qDebug() << "StcModel::addData" << stc.data.rows();
//    qDebug() << "MNESourceEstimate" << stc.vertices.size();
//    qDebug() << "m_vertices.size()" << m_vertices.size();
//    std::cout << "Vertices\n" << stc.vertices << std::endl;

    //store for next buffer
    m_iCurrentSample = i - stc.data.cols();

//    //ToDo separate worker thread? ToDo 2000 -> size of screen
//    if(m_dataCurrent.size() > m_iMaxSamples)
//    {
//        m_dataLast = m_dataCurrent.mid(0,m_iMaxSamples); // Store last data to keep as background in the display
//        m_dataCurrent.remove(0, m_iMaxSamples);
//    }

//    //Update data content
//    QModelIndex topLeft = this->index(0,1);
//    QModelIndex bottomRight = this->index(m_qListChInfo.size()-1,1);
//    QVector<int> roles; roles << Qt::DisplayRole;
//    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

void StcModel::init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet)
{
    beginResetModel();
    m_annotationSet = annotationSet;
    m_surfSet = surfSet;
    m_annotationSet.toLabels(m_surfSet, m_qListLabels, m_qListRGBAs);
    m_vecCurrentStc = VectorXd::Zero(m_qListLabels.size());
    endResetModel();
    m_bIsInit = true;
}


//*************************************************************************************************************

void StcModel::setStcSample(const VectorXd &sample)
{
    m_vecCurrentStc = sample;

    //Update data content
    QModelIndex topLeft = this->index(0,3);
    QModelIndex bottomRight = this->index(m_qListLabels.size()-1,3);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight);
}


//*************************************************************************************************************

void StcModel::setVertices(const VectorXi &vertnos)
{
    m_vertices = vertnos;
}

