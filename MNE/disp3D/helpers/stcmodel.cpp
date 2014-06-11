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
, m_iDownsampling(20)
, m_iCurrentSample(0)
, m_dStcNormMax(5.0)
, m_dStcNorm(1.0)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");
    qRegisterMetaType<VectorXd>("VectorXd");

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
    return 6;
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
            case 4: { // roi name
                if(role == Qt::DisplayRole)
                    return QVariant(m_qListLabels[row].name);
                break;
            }
            case 5: { // roi color
                if(role == Qt::DisplayRole)
                    return QVariant(QColor(m_qListRGBAs[row](0),m_qListRGBAs[row](1),m_qListRGBAs[row](2),255));
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
                return QVariant("ROI Name");
            case 5: //roi color column
                return QVariant("ROI Color");
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
    for(i = m_iCurrentSample; i < stc.data.cols(); i += m_iDownsampling)
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

    //Update data content
    QModelIndex topLeft = this->index(0,2);
    QModelIndex bottomRight = this->index(m_qListLabels.size()-1,2);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight);
}


//*************************************************************************************************************

void StcModel::setVertices(const VectorXi &vertnos)
{
    m_vertices = vertnos;
}

