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

    void setSamplingInfo(float sps, int T, float dest_sps  = 100.0f);

    void addData(const QVector<VectorXd> &data);

    fiff_int_t getKind(qint32 row) const;

    fiff_int_t getUnit(qint32 row) const;

    fiff_int_t getCoil(qint32 row) const;

    inline qint32 getMaxSamples() const;

    inline const QMap<qint32,qint32>& getIdxSelMap() const;

    void selectRows(const QList<qint32> &selection);

    void resetSelection();

    inline qint32 numVLines() const;

    void toggleFreeze(const QModelIndex &);

    inline bool isFreezed() const;

signals:
    void newSelection(QList<qint32> selection);

private:
    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

    QMap<qint32,qint32> m_qMapIdxRowSelection;            /**< Selection mapping.*/

    //Fiff data structure
    QVector<VectorXd> m_dataCurrent;        /**< List that holds the current data*/
    QVector<VectorXd> m_dataLast;           /**< List that holds the last data */

    QVector<VectorXd> m_dataCurrentFreeze;        /**< List that holds the current data when freezed*/
    QVector<VectorXd> m_dataLastFreeze;           /**< List that holds the last data when freezed*/

    float m_fSps;               /**< Sampling rate */
    qint32 m_iT;                /**< Time window */
    qint32 m_iDownsampling;     /**< Down sampling factor */
    qint32 m_iMaxSamples;       /**< Max samples per window */
    qint32 m_iCurrentSample;    /**< Accurate Downsampling */

    bool m_bIsFreezed;       /**< Is freezed */
};


inline qint32 RealTimeMultiSampleArrayModel::getMaxSamples() const
{
    return m_iMaxSamples;
}


inline const QMap<qint32,qint32>& RealTimeMultiSampleArrayModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}


inline qint32 RealTimeMultiSampleArrayModel::numVLines() const
{
    return (m_iT - 1);
}


inline bool RealTimeMultiSampleArrayModel::isFreezed() const
{
    return m_bIsFreezed;
}

#endif // REALTIMEMULTISAMPLEARRAYMODEL_H
