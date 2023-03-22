//=============================================================================================================
/**
 * @file     rtfiffrawviewmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of the RtFiffRawViewModel Class.
 *
 */

#ifndef RTFIFFRAWVIEWMODEL_H
#define RTFIFFRAWVIEWMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_proj.h>

#include <rtprocessing/helpers/filterkernel.h>

#include <events/eventmanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QSharedPointer>
#include <QColor>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

typedef QPair<const double*,qint32> RowVectorPair;
typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> MatrixXdR;

//=============================================================================================================
/**
 * DECLARE CLASS RtFiffRawViewModel
 *
 * @brief The RtFiffRawViewModel class implements the data access model for a real-time multi sample array data stream
 */
class DISPSHARED_EXPORT RtFiffRawViewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtFiffRawViewModel> SPtr;              /**< Shared pointer type for RtFiffRawViewModel. */
    typedef QSharedPointer<const RtFiffRawViewModel> ConstSPtr;   /**< Const shared pointer type for RtFiffRawViewModel. */

    //=========================================================================================================
    /**
     * Constructs an real-time multi sample array table model for the given parent.
     *
     * @param[in] parent     parent of the table model.
     */
    RtFiffRawViewModel(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destructs RtFiffRawViewModel and stops the use of Shared Memory within the Event Manager.
     *
     */

    ~RtFiffRawViewModel();
    //=========================================================================================================
    /**
     * Returns the number of rows under the given parent. When the parent is valid it means that rowCount is returning the number of children of parent.
     *
     * @param[in] parent     not used.
     *
     * @return number of rows.
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;

    //=========================================================================================================
    /**
     * Returns the number of columns for the children of the given parent.
     *
     * @param[in] parent     not used.
     *
     * @return number of columns.
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the item referred to by the index.
     *
     * @param[in] index      determines item location.
     * @param[in] role       role to return.
     *
     * @return accessed data.
     */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Returns the data for the given role and section in the header with the specified orientation.
     *
     * @param[in] section        For horizontal headers, the section number corresponds to the column number. Similarly, for vertical headers, the section number corresponds to the row number.
     * @param[in] orientation    Qt::Horizontal or Qt::Vertical.
     * @param[in] role           role to show.
     *
     * @return accessed eader data.
     */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Sets corresponding fiff information
     *
     * @param[in] p_pFiffInfo   The corresponding fiff information.
     */
    void setFiffInfo(QSharedPointer<FIFFLIB::FiffInfo>& p_pFiffInfo);

    //=========================================================================================================
    /**
     * Sets the sampling information and calculates the resulting downsampling factor between actual sps and desired sps
     *
     * @param[in] sps        Samples per second of incomming data.
     * @param[in] T          Time window length to display.
     * @param[in] bSetZero   If data should be set to zero. Default is false.
     */
    void setSamplingInfo(float sps, int T, bool bSetZero = false);

    //=========================================================================================================
    /**
     * Returns the last received data block.
     *
     * @return the last data block.
     */
    Eigen::MatrixXd getLastBlock();

    //=========================================================================================================
    /**
     * Adds multiple time points (QVector) for a channel set (VectorXd)
     *
     * @param[in] data       data to add (Time points of channel samples).
     */
    void addData(const QList<Eigen::MatrixXd> &data);

    //=========================================================================================================
    /**
     * Returns the kind of a given channel number
     *
     * @param[in] row    row number which correspodns to a given channel.
     *
     * @return kind of given channel number.
     */
    FIFFLIB::fiff_int_t getKind(qint32 row) const;

    //=========================================================================================================
    /**
     * Returns the unit of a given channel number
     *
     * @param[in] row    row number which correspodns to a given channel.
     *
     * @return unit of given channel number.
     */
    FIFFLIB::fiff_int_t getUnit(qint32 row) const;

    //=========================================================================================================
    /**
     * Returns the coil type of a given channel number
     *
     * @param[in] row    row number which correspodns to a given channel.
     *
     * @return coil type of given channel number.
     */
    FIFFLIB::fiff_int_t getCoil(qint32 row) const;

    //=========================================================================================================
    /**
     * Returns the maximal number of samples of the downsampled data to display
     *
     * @return the maximal number of samples.
     */
    inline qint32 getMaxSamples() const;

    //=========================================================================================================
    /**
     * Returns the current sample index which represents the index which the next incoming data will be stored at in the data
     *
     * @return the current sample index.
     */
    inline qint32 getCurrentSampleIndex() const;

    //=========================================================================================================
    /**
     * Returns the first value of the last complete data display block
     *
     * @param[in] row    row for which the first value is to be returned.
     *
     * @return the first value of the last complete data display block.
     */
    inline double getLastBlockFirstValue(int row) const;

    //=========================================================================================================
    /**
     * Returns a map which conatins the channel idx and its corresponding selection status
     *
     * @return the channel idx to selection status.
     */
    inline const QMap<qint32,qint32>& getIdxSelMap() const;

    //=========================================================================================================
    /**
     * Selects the given list of channel indeces and unselect all other channels
     *
     * @param[in] selection      channel index list to select.
     */
    void selectRows(const QList<qint32> &selection);

    //=========================================================================================================
    /**
     * Hides the given list of channel
     *
     * @param[in] selection      channel index list to select.
     */
    void hideRows(const QList<qint32> &selection);

    //=========================================================================================================
    /**
     * Resets the current selection (selects all channels)
     */
    void resetSelection();

    //=========================================================================================================
    /**
     * Toggle freeze for all channels when a channel is double clicked
     *
     * @param[in] index     of the channel which has been double clicked.
     */
    void toggleFreeze(const QModelIndex &index);

    //=========================================================================================================
    /**
     * Set scaling channel scaling
     *
     * @param[in] p_qMapChScaling    Map of scaling factors.
     */
    void setScaling(const QMap< qint32,float >& p_qMapChScaling);

    //=========================================================================================================
    /**
     * Update the SSP projection
     *
     * @param[in] projs    The new projectors.
     */
    void updateProjection(const QList<FIFFLIB::FiffProj>& projs);

    //=========================================================================================================
    /**
     * Update the compensator
     *
     * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind).
     */
    void updateCompensator(int to);

    //=========================================================================================================
    /**
     * Update the SPHARA operator
     *
     * @param[in] state            The current state of teh SPHARA tool.
     */
    void updateSpharaActivation(bool state);

    //=========================================================================================================
    /**
     * Update the SPHARA operator
     *
     * @param[in] sSystemType            The current acquisition system type (VectorView, BabyMEG, EEG).
     * @param[in] nBaseFctsFirst         The new number of basis function to use for the first SPHARA operator.
     * @param[in] nBaseFctsSecond        The new number of basis function to use for the second SPHARA operator.
     */
    void updateSpharaOptions(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond);

    //=========================================================================================================
    /**
     * Set new filter parameters
     *
     * @param[in] filterData    list of the new filter.
     */
    void setFilter(QList<RTPROCESSINGLIB::FilterKernel> filterData);

    //=========================================================================================================
    /**
     * Set filter activation
     *
     * @param[in] state    filter on/off flag.
     */
    void setFilterActive(bool state);

    //=========================================================================================================
    /**
     * Set the background color
     *
     * @param[in] color    The background color.
     */
    void setBackgroundColor(const QColor& color);

    //=========================================================================================================
    /**
     * Sets the type of channel which are to be filtered
     *
     * @param[in] channelType    the channel type which is to be filtered (EEG, MEG, All).
     */
    void setFilterChannelType(const QString &channelType);

    //=========================================================================================================
    /**
     * Create list of channels which are to be filtered based on channel names
     *
     * @param[in] channelNames    the channel names which are to be filtered.
     */
    void createFilterChannelList(QStringList channelNames);

    //=========================================================================================================
    /**
     * markChBad marks the selected channels as bad/good in m_chInfolist
     *
     * @param[in] chlist index that is selected for marking.
     * @param[in] status, status=1 -> mark as bad, status=0 -> mark as good.
     */
    void markChBad(QModelIndex ch, bool status);

    //=========================================================================================================
    /**
     * markChBad marks the selected channels as bad/good in m_chInfolist
     *
     * @param[in] chlist is the list of indices that are selected for marking.
     * @param[in] status, status=1 -> mark as bad, status=0 -> mark as good.
     */
    void markChBad(QModelIndexList chlist, bool status);

    //=========================================================================================================
    /**
     * markChBad marks the selected channels as bad/good in m_chInfolist
     *
     * @param[in] colorMap       color for each trigger channel.
     * @param[in] activ          real time trigger detection active.
     * @param[in] triggerCh      current trigger channel to scan.
     * @param[in] threshold      threshold for the detection process.
     */
    void triggerInfoChanged(const QMap<double, QColor>& colorMap, bool active, QString triggerCh, double threshold);

    //=========================================================================================================
    /**
     * distanceTimeSpacerChanged changes the distance of the time spacers
     *
     * @param[in] value the new distance for the time spacers.
     */
    void distanceTimeSpacerChanged(int value);

    //=========================================================================================================
    /**
     * resetTriggerCounter resets the trigger counter
     */
    void resetTriggerCounter();

    //=========================================================================================================
    /**
     * Returns the number of vertical lines (one per second)
     *
     * @return number of vertical lines.
     */
    inline qint32 numVLines() const;

    //=========================================================================================================
    /**
     * Returns current freezing status
     *
     * @return the current freezing status.
     */
    inline bool isFreezed() const;

    //=========================================================================================================
    /**
     * Returns current scaling
     *
     * @return the current scaling.
     */
    inline const QMap< qint32,float >& getScaling() const;

    //=========================================================================================================
    /**
     * Returns current detected trigger flanks
     *
     * @return the current detected trigger flanks.
     */
    inline QList<QPair<int,double> > getDetectedTriggers() const;

    //=========================================================================================================
    /**
     * Returns old detected trigger flanks
     *
     * @return the old detected trigger flanks.
     */
    inline QList<QPair<int, double> > getDetectedTriggersOld() const;

    //=========================================================================================================
    /**
     * Returns current trigger color
     *
     * @return the current trigger color map fpr each detected type.
     */
    inline QMap<double, QColor> getTriggerColor() const;

    //=========================================================================================================
    /**
     * Returns the current number for the time spacers
     *
     * @return the current number for the time spacers.
     */
    inline int getNumberOfTimeSpacers() const;

    //=========================================================================================================
    /**
     * Returns the current trigger threshold
     *
     * @return the current trigger threshold.
     */
    inline double getTriggerThreshold() const;

    //=========================================================================================================
    /**
     * Returns the current trigger name
     *
     * @return the current trigger name.
     */
    inline QString getTriggerName() const;

    //=========================================================================================================
    /**
     * Returns the current trigger channel index
     *
     * @return the current trigger channel index.
     */
    inline int getCurrentTriggerIndex() const;

    //=========================================================================================================
    /**
     * Returns whether trigger detection is active or not
     *
     * @return whether trigger detection is active or not.
     */
    inline bool triggerDetectionActive() const;

    //=========================================================================================================
    /**
     * Returns the current overlap add delay
     *
     * @return the current overlap add delay.
     */
    inline int getCurrentOverlapAddDelay() const;

    //=========================================================================================================
    /**
     * Get offset of first drawn sample in the window
     *
     * @return sample offset of window
     */
    inline int getFirstSampleOffset() const;

    //=========================================================================================================
    /**
     * Get maximum range of respective channel type. range value in FiffChInfo does not seem to contain a reasonable value
     *
     * @param [in] Row of the model
     * @return the max value of the y axis for the channel
     */
    double getMaxValueFromRawViewModel(int row) const;

    //=========================================================================================================
    /**
     * Adds event based on input parameters
     *
     * @param[in]iSample    Sample of the new event.
     */
    void addEvent(int iSample);

    //=========================================================================================================
    /**
     * Returns events between two samples
     *
     * @param[in] iBegin    Lower bound for sample (inclusive).
     * @param[in] iEnd      Upper bound for sample (exclusive).
     *
     * @return  Pointer to a vector of events.
     */
    std::unique_ptr<std::vector<EVENTSLIB::Event> > getEventsToDisplay(int iBegin, int iEnd) const;

private:
    //=========================================================================================================
    /**
     * Init the SPHARA method.
     */
    void initSphara();

    static void doFilterPerChannelRTMSA(QPair<QList<RTPROCESSINGLIB::FilterKernel>,QPair<int,Eigen::RowVectorXd> > &channelDataTime);

    //=========================================================================================================
    /**
     * Calculates the filtered version of the channels in m_matDataRaw
     */
    void filterDataBlock();

    //=========================================================================================================
    /**
     * Calculates the filtered version of the raw input data
     *
     * @param[in] data          data which is to be filtered.
     * @param[in] iDataIndex    current position in the global data matrix.
     */
    void filterDataBlock(const Eigen::MatrixXd &data, int iDataIndex);

    //=========================================================================================================
    /**
     * Clears the model
     */
    void clearModel();

    bool                                m_bProjActivated;                           /**< Projections activated. */
    bool                                m_bCompActivated;                           /**< Compensator activated. */
    bool                                m_bSpharaActivated;                         /**< Sphara activated. */
    bool                                m_bIsFreezed;                               /**< Display is freezed. */
    bool                                m_bDrawFilterFront;                         /**< Flag whether to plot/write the delayed frontal part of the filtered signal. This flag is necessary to get rid of nasty signal jumps when changing the filter parameters. */
    bool                                m_bPerformFiltering;                        /**< Flag whether to activate/deactivate filtering. */
    bool                                m_bTriggerDetectionActive;                  /**< Trigger detection activation state. */
    float                               m_fSps;                                     /**< Sampling rate. */
    double                              m_dTriggerThreshold;                        /**< Trigger detection threshold. */
    qint32                              m_iT;                                       /**< Time window. */
    qint32                              m_iDownsampling;                            /**< Down sampling factor. */
    qint32                              m_iMaxSamples;                              /**< Max samples per window. */
    qint32                              m_iCurrentSample;                           /**< Current sample which holds the current position in the data matrix. */
    qint32                              m_iCurrentStartingSample;                   /**< Accumulates cumulative starting sample position when m_iCurrentSample resets to 0 */
    qint32                              m_iCurrentSampleFreeze;                     /**< Current sample which holds the current position in the data matrix when freezing tool is active. */
    qint32                              m_iMaxFilterLength;                         /**< Max order of the current filters. */
    qint32                              m_iCurrentBlockSize;                        /**< Current block size. */
    qint32                              m_iResidual;                                /**< Current amount of samples which were to size. */
    int                                 m_iCurrentTriggerChIndex;                   /**< The index of the current trigger channel. */
    int                                 m_iDistanceTimerSpacer;                     /**< The distance for the horizontal time spacers in the view in ms. */
    int                                 m_iDetectedTriggers;                        /**< Detected triggers since the last reset. */

    QString                             m_sCurrentTriggerCh;                        /**< Current trigger channel which is beeing scanned. */
    QString                             m_sFilterChannelType;                       /**< Kind of channel which is to be filtered. */

    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                                /**< Fiff info. */

    Eigen::RowVectorXi                  m_vecBadIdcs;                               /**< Idcs of bad channels. */
    Eigen::VectorXd                     m_vecLastBlockFirstValuesFiltered;          /**< The first value of the last complete filtered data display block. */
    Eigen::VectorXd                     m_vecLastBlockFirstValuesRaw;               /**< The first value of the last complete raw data display block. */

    MatrixXdR                           m_matDataRaw;                               /**< The raw data. */
    MatrixXdR                           m_matDataFiltered;                          /**< The filtered data. */
    MatrixXdR                           m_matDataRawFreeze;                         /**< The raw data in freeze mode. */
    MatrixXdR                           m_matDataFilteredFreeze;                    /**< The raw filtered data in freeze mode. */
    Eigen::MatrixXd                     m_matOverlap;                               /**< Last overlap block for the back. */

    Eigen::VectorXi                     m_vecIndicesFirstVV;                        /**< The indices of the channels to pick for the first SPHARA operator in case of a VectorView system.*/
    Eigen::VectorXi                     m_vecIndicesSecondVV;                       /**< The indices of the channels to pick for the second SPHARA operator in case of a VectorView system.*/
    Eigen::VectorXi                     m_vecIndicesFirstBabyMEG;                   /**< The indices of the channels to pick for the first SPHARA operator in case of a BabyMEG system.*/
    Eigen::VectorXi                     m_vecIndicesSecondBabyMEG;                  /**< The indices of the channels to pick for the second SPHARA operator in case of a BabyMEG system.*/
    Eigen::VectorXi                     m_vecIndicesFirstEEG;                       /**< The indices of the channels to pick for the second SPHARA operator in case of an EEG system.*/

    Eigen::SparseMatrix<double>         m_matSparseSpharaMult;                      /**< The final sparse SPHARA operator .*/
    Eigen::SparseMatrix<double>         m_matSparseProjCompMult;                    /**< The final sparse projection + compensator operator.*/
    Eigen::SparseMatrix<double>         m_matSparseProjMult;                        /**< The final sparse SSP projector. */
    Eigen::SparseMatrix<double>         m_matSparseCompMult;                        /**< The final sparse compensator matrix. */

    Eigen::MatrixXd                     m_matProj;                                  /**< SSP projector. */
    Eigen::MatrixXd                     m_matComp;                                  /**< Compensator. */

    Eigen::MatrixXd                     m_matSpharaVVGradLoaded;                    /**< The loaded VectorView gradiometer basis functions.*/
    Eigen::MatrixXd                     m_matSpharaVVMagLoaded;                     /**< The loaded VectorView magnetometer basis functions.*/
    Eigen::MatrixXd                     m_matSpharaBabyMEGInnerLoaded;              /**< The loaded babyMEG inner layer basis functions.*/
    Eigen::MatrixXd                     m_matSpharaBabyMEGOuterLoaded;              /**< The loaded babyMEG outer layer basis functions.*/
    Eigen::MatrixXd                     m_matSpharaEEGLoaded;                       /**< The loaded EEG basis functions.*/

    QMap<double, QColor>                m_qMapTriggerColor;                         /**< Current colors for all trigger channels. */
    QMap<int,QList<QPair<int,double> > >m_qMapDetectedTrigger;                      /**< Detected trigger for each trigger channel. */
    QList<int>                          m_lTriggerChannelIndices;                   /**< List of all trigger channel indices. */
    QMap<int,QList<QPair<int,double> > >m_qMapDetectedTriggerFreeze;                /**< Detected trigger for each trigger channel while display is freezed. */
    QMap<int,QList<QPair<int,double> > >m_qMapDetectedTriggerOld;                   /**< Old detected trigger for each trigger channel. */
    QMap<int,QList<QPair<int,double> > >m_qMapDetectedTriggerOldFreeze;             /**< Old detected trigger for each trigger channel while display is freezed. */
    QMap<qint32,float>                  m_qMapChScaling;                            /**< Channel scaling map. */
    QList<RTPROCESSINGLIB::FilterKernel>m_filterKernel;                             /**< List of currently active filters. */
    QStringList                         m_filterChannelList;                        /**< List of channels which are to be filtered.*/
    QStringList                         m_visibleChannelList;                       /**< List of currently visible channels in the view.*/
    QMap<qint32,qint32>                 m_qMapIdxRowSelection;                      /**< Selection mapping.*/

    QColor                              m_colBackground;                            /**< The background color.*/

    static EVENTSLIB::EventManager      m_EventManager;                             /**< Database for storing events and using shared memory. Shared between all instances */

signals:
    //=========================================================================================================
    /**
     * Emmited when new selcetion was made
     *
     * @param[in] selection     list of all selected channels.
     */
    void newSelection(const QList<qint32>& selection);

    //=========================================================================================================
    /**
     * Emmited when the window size/max number of samples changed
     *
     * @param[in] windowSize     number of samples in the window.
     */
    void windowSizeChanged(int windowSize);

    //=========================================================================================================
    /**
     * Emmited when trigger detection was performed
     */
    void triggerDetected(int numberDetectedTriggers, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 RtFiffRawViewModel::getMaxSamples() const
{
    return m_iMaxSamples;
}

//=============================================================================================================

inline qint32 RtFiffRawViewModel::getCurrentSampleIndex() const
{
    if(m_bIsFreezed) {
        return m_iCurrentSampleFreeze;
    }

    if(!m_filterKernel.isEmpty() && m_bPerformFiltering) {
        return m_iCurrentSample-m_iMaxFilterLength/2;
    }

    return m_iCurrentSample;
}

//=============================================================================================================

inline double RtFiffRawViewModel::getLastBlockFirstValue(int row) const
{
    if(row>m_vecLastBlockFirstValuesFiltered.rows() || row>m_vecLastBlockFirstValuesRaw.rows())
        return 0;

    if(!m_filterKernel.isEmpty())
        return m_vecLastBlockFirstValuesFiltered[row];

    return m_vecLastBlockFirstValuesRaw[row];
}

//=============================================================================================================

inline const QMap<qint32,qint32>& RtFiffRawViewModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}

//=============================================================================================================

inline qint32 RtFiffRawViewModel::numVLines() const
{
    return (m_iT - 1);
}

//=============================================================================================================

inline bool RtFiffRawViewModel::isFreezed() const
{
    return m_bIsFreezed;
}

//=============================================================================================================

inline const QMap<qint32,float>& RtFiffRawViewModel::getScaling() const
{
    return m_qMapChScaling;
}

//=============================================================================================================

inline QMap<double, QColor> RtFiffRawViewModel::getTriggerColor() const
{
    if(m_bTriggerDetectionActive) {
        return m_qMapTriggerColor;
    }

    QMap<double, QColor> map;
    return map;
}

//=============================================================================================================

inline QList<QPair<int,double> >  RtFiffRawViewModel::getDetectedTriggers() const
{
    QList<QPair<int,double> > triggerIndices;

    if(m_bIsFreezed)
        return m_qMapDetectedTriggerFreeze[m_iCurrentTriggerChIndex];

    if(m_bTriggerDetectionActive) {
        return m_qMapDetectedTrigger[m_iCurrentTriggerChIndex];
    }
    else
        return triggerIndices;
}

//=============================================================================================================

inline QList<QPair<int,double> > RtFiffRawViewModel::getDetectedTriggersOld() const
{
    QList<QPair<int,double> > triggerIndices;

    if(m_bIsFreezed)
        return m_qMapDetectedTriggerOldFreeze[m_iCurrentTriggerChIndex];

    if(m_bTriggerDetectionActive) {
        return m_qMapDetectedTriggerOld[m_iCurrentTriggerChIndex];
    }
    else
        return triggerIndices;
}

//=============================================================================================================

inline int RtFiffRawViewModel::getNumberOfTimeSpacers() const
{
    //qDebug()<<((m_iT*1000)/m_iDistanceTimerSpacer)-1;
    return ((1000)/m_iDistanceTimerSpacer)-1;
}

//=============================================================================================================

inline double RtFiffRawViewModel::getTriggerThreshold() const
{
    return m_dTriggerThreshold;
}

//=============================================================================================================

inline QString RtFiffRawViewModel::getTriggerName() const
{
    return m_sCurrentTriggerCh;
}

//=============================================================================================================

inline int RtFiffRawViewModel::getCurrentTriggerIndex() const
{
    return m_iCurrentTriggerChIndex;
}

//=============================================================================================================

inline bool RtFiffRawViewModel::triggerDetectionActive() const
{
    return m_bTriggerDetectionActive;
}

//=============================================================================================================

inline int RtFiffRawViewModel::getCurrentOverlapAddDelay() const
{
    if(!m_filterKernel.isEmpty())
        return m_iMaxFilterLength/2;
    else
        return 0;
}

//=============================================================================================================

inline int RtFiffRawViewModel::getFirstSampleOffset() const
{
    return m_iCurrentStartingSample;
}
} // NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#ifndef metatype_rowvectorpair
#define metatype_rowvectorpair
Q_DECLARE_METATYPE(DISPLIB::RowVectorPair);
#endif
#endif

#endif // RTFIFFRAWVIEWMODEL_H
