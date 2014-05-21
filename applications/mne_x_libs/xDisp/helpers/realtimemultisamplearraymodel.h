#ifndef REALTIMEMULTISAMPLEARRAYMODEL_H
#define REALTIMEMULTISAMPLEARRAYMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <xMeas/realtimesamplearraychinfo.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;
using namespace FIFFLIB;
using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArrayModel
*/
class RealTimeMultiSampleArrayModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    RealTimeMultiSampleArrayModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void setChannelInfo(QList<RealTimeSampleArrayChInfo> &chInfo);

    void setSamplingInfo(float sps, float T, float dest_sps  = 100.0f);

    void addData(const QVector<VectorXd> &data);

    fiff_int_t getKind(qint32 row) const;

    fiff_int_t getUnit(qint32 row) const;

    inline qint32 getMaxSamples() const;

    inline const QMap<qint32,qint32>& getIdxSelMap() const;

    void selectRows(const QVector<qint32> &selection);

    void resetSelection();


private:
    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

    QMap<qint32,qint32> m_qMapIdxRowSelection;            /**< Selection mapping.*/

    //Fiff data structure
    QVector<VectorXd> m_dataCurrent;        /**< List that holds the current data*/
    QVector<VectorXd> m_dataLast;           /**< List that holds the last data */

    float m_fSps;           /**< Sampling rate */
    float m_fT;             /**< Time window */
    qint32 m_iDownsampling; /**< Down sampling factor */
    qint32 m_iMaxSamples;   /**< Max samples per window */
    qint32 m_iCurrentSample; /**< Accurate Downsampling */
};


inline qint32 RealTimeMultiSampleArrayModel::getMaxSamples() const
{
    return m_iMaxSamples;
}


inline const QMap<qint32,qint32>& RealTimeMultiSampleArrayModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}



#endif // REALTIMEMULTISAMPLEARRAYMODEL_H
