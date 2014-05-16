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

    void setSamplingInfo(float sps, float T, float dest_sps  = 100.0f);

    void addData(const QVector<VectorXd> &data);

private:
    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

    //Fiff data structure
    QVector<VectorXd> m_dataCurrent;        /**< List that holds the fiff matrix data <n_channels x n_samples> */
    QVector<VectorXd> m_dataLast;        /**< List that holds the fiff matrix data <n_channels x n_samples> */

    float m_fSps;           /**< Sampling rate */
    float m_fT;             /**< Time window */
    qint32 m_iDownsampling; /**< Down sampling factor */
    qint32 m_iMaxSamples;   /**< Max samples per window */
    qint32 m_iCurrentSample; /**< Accurate Downsampling */
};

#endif // REALTIMEMULTISAMPLEARRAYMODEL_H
