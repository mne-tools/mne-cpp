#ifndef REALTIMEMULTISAMPLEARRAYMODEL_H
#define REALTIMEMULTISAMPLEARRAYMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <xMeas/realtimesamplearraychinfo.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;
using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArrayModel
*/
class RealTimeMultiSampleArrayModel : public QAbstractTableModel
{
    Q_OBJECT

    friend class RealTimeMultiSampleArrayDelegate;
public:
    RealTimeMultiSampleArrayModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void setChannelInfo(QList<RealTimeSampleArrayChInfo> &chInfo);

private:
    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

    //Fiff data structure
    QVector<VectorXd> m_data;        /**< List that holds the fiff matrix data <n_channels x n_samples> */


};

#endif // REALTIMEMULTISAMPLEARRAYMODEL_H
