//=============================================================================================================
/**
* @file     realtimeevokedsetmodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeEvokedSetModel Class.
*
*/

#ifndef REALTIMEEVOKEDSETMODEL_H
#define REALTIMEEVOKEDSETMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <scMeas/realtimesamplearraychinfo.h>
#include <scMeas/realtimeevokedset.h>
#include <fiff/fiff_types.h>
#include <iostream>

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RealTimeEvokedSetModelRoles
{
    enum ItemRole{GetAverageData = Qt::UserRole + 1020};
}

typedef QPair<const double*,qint32> RowVectorPair;
typedef QPair<double, Eigen::RowVectorXd> AvrTypeRowVector;
typedef QPair<double, SCDISPLIB::RowVectorPair> AvrTypeRowVectorPair;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace SCDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

typedef Matrix<double,Dynamic,Dynamic,RowMajor> MatrixXdR;


//=============================================================================================================
/**
* DECLARE CLASS RealTimeEvokedSetModel
*
* @brief The RealTimeEvokedSetModel class implements the data access model for a real-time multi sample array data stream
*/
class RealTimeEvokedSetModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeEvokedSetModel> SPtr;              /**< Shared pointer type for RealTimeEvokedSetModel. */
    typedef QSharedPointer<const RealTimeEvokedSetModel> ConstSPtr;   /**< Const shared pointer type for RealTimeEvokedSetModel. */

    //=========================================================================================================
    /**
    * Constructs an real-time multi sample array table model for the given parent.
    *
    * @param[in] parent     parent of the table model
    */
    RealTimeEvokedSetModel(QObject *parent = 0);
    ~RealTimeEvokedSetModel();

    inline bool isInit() const;

    inline qint32 getNumSamples() const;

    //=========================================================================================================
    /**
    * Returns the number of rows under the given parent. When the parent is valid it means that rowCount is returning the number of children of parent.
    *
    * @param[in] parent     not used
    *
    * @return number of rows
    */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;

    //=========================================================================================================
    /**
    * Returns the number of columns for the children of the given parent.
    *
    * @param[in] parent     not used
    *
    * @return number of columns
    */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //=========================================================================================================
    /**
    * Data for the row and column and given display role
    *
    * @param [in] row       index row
    * @param [in] column    index column
    * @param [in] role      display role to access
    *
    * @return the accessed data
    */
    inline QVariant data(int row, int column, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
    * Returns the data stored under the given role for the item referred to by the index.
    *
    * @param[in] index      determines item location
    * @param[in] role       role to return
    *
    * @return accessed data
    */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
    * Returns the data for the given role and section in the header with the specified orientation.
    *
    * @param[in] section        For horizontal headers, the section number corresponds to the column number. Similarly, for vertical headers, the section number corresponds to the row number.
    * @param[in] orientation    Qt::Horizontal or Qt::Vertical
    * @param[in] role           role to show
    *
    * @return accessed eader data
    */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
    * Sets corresponding real-time evoked set
    *
    * @param [in] pRTESet      The real-time evoked set
    */
    void setRTESet(QSharedPointer<RealTimeEvokedSet> &pRTESet);

    //=========================================================================================================
    /**
    * Update stored data
    */
    void updateData();

    //=========================================================================================================
    /**
    * Returns the color of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return color of given channel number
    */
    QColor getColor(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns the kind of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return kind of given channel number
    */
    fiff_int_t getKind(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns the unit of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return unit of given channel number
    */
    fiff_int_t getUnit(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns the coil type of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return coil type of given channel number
    */
    fiff_int_t getCoil(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns a map which conatins the channel idx and its corresponding selection status
    *
    * @return the channel idx to selection status
    */
    inline const QMap<qint32,qint32>& getIdxSelMap() const;

    //=========================================================================================================
    /**
    * Returns current scaling
    *
    * @return the current scaling
    */
    inline const QMap< qint32,float >& getScaling() const;

    //=========================================================================================================
    /**
    * Returns the current number for the time spacers
    *
    * @return the current number for the time spacers
    */
    inline int getNumberOfTimeSpacers() const;

    //=========================================================================================================
    /**
    * Returns the current baseline information
    *
    * @return the current baseline information as a from to QPair
    */
    inline QPair<QVariant,QVariant> getBaselineInfo() const;

    //=========================================================================================================
    /**
    * Returns the current number of stored averages
    *
    * @return the current number of stored averages
    */
    inline int getNumAverages() const;

    //=========================================================================================================
    /**
    * Returns the number of pre-stimulus samples
    *
    * @return the number of pre-stimulus samples
    */
    inline qint32 getNumPreStimSamples() const;

    //=========================================================================================================
    /**
    * Returns the current sampling frequency
    *
    * @return the current sampling frequency
    */
    inline float getSamplingFrequency() const;

    //=========================================================================================================
    /**
    * Selects the given list of channel indeces and unselect all other channels
    *
    * @param[in] selection      channel index list to select
    */
    void selectRows(const QList<qint32> &selection);

    //=========================================================================================================
    /**
    * Resets the current selection (selects all channels)
    */
    void resetSelection();

    //=========================================================================================================
    /**
    * Returns the number of vertical lines (one per second)
    *
    * @return number of vertical lines
    */
    inline qint32 numVLines() const;

    //=========================================================================================================
    /**
    * Returns current freezing status
    *
    * @return the current freezing status
    */
    inline bool isFreezed() const;

    //=========================================================================================================
    /**
    * Set scaling channel scaling
    *
    * @param[in] p_qMapChScaling    Map of scaling factors
    */
    void setScaling(const QMap< qint32,float >& p_qMapChScaling);

    //=========================================================================================================
    /**
    * Update projections
    */
    void updateProjection();

    //=========================================================================================================
    /**
    * Update the compensator
    *
    * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind)
    */
    void updateCompensator(int to);

    //=========================================================================================================
    /**
    * Toggle freeze for all channels when a channel is double clicked
    */
    void toggleFreeze();

    //=========================================================================================================
    /**
    * Filter parameters changed
    *
    * @param[in] filterData    list of the currently active filter
    */
    void filterChanged(QList<FilterData> filterData);

    //=========================================================================================================
    /**
    * Sets the type of channel which are to be filtered
    *
    * @param[in] channelType    the channel type which is to be filtered (EEG, MEG, All)
    */
    void setFilterChannelType(QString channelType);

    //=========================================================================================================
    /**
    * Create list of channels which are to be filtered based on channel names
    *
    * @param[in] channelNames    the channel names which are to be filtered
    */
    void createFilterChannelList(QStringList channelNames);

private:
    //=========================================================================================================
    /**
    * Calculates the filtered version of the channels in m_matData
    */
    void filterChannelsConcurrently();

    QSharedPointer<RealTimeEvokedSet>   m_pRTESet;                      /**< The real-time evoked set measurement. */

    QMap<qint32,qint32>                 m_qMapIdxRowSelection;          /**< Selection mapping.*/
    QMap<qint32,float>                  m_qMapChScaling;                /**< Channel scaling map. */
    QMap<double, QPair<QColor, QPair<QString,bool> > >  m_qMapAverageInformation;             /**< Average colors and names. */

    QList<Eigen::MatrixXd>              m_matData;                      /**< List that holds the data*/
    QList<Eigen::MatrixXd>              m_matDataFreeze;                /**< List that holds the data when freezed*/
    QList<Eigen::MatrixXd>              m_matDataFiltered;              /**< The filtered data */
    QList<Eigen::MatrixXd>              m_matDataFilteredFreeze;        /**< The raw filtered data in freeze mode */
    QList<double>                       m_lAvrTypes;                    /**< The average types */

    Eigen::MatrixXd                     m_matProj;                      /**< SSP projector */
    Eigen::MatrixXd                     m_matComp;                      /**< Compensator */
    Eigen::SparseMatrix<double>         m_matSparseProjCompMult;        /**< The final sparse projection + compensator operator.*/
    Eigen::SparseMatrix<double>         m_matSparseProjMult;            /**< The final sparse SSP projector */
    Eigen::SparseMatrix<double>         m_matSparseCompMult;            /**< The final sparse compensator matrix */

    Eigen::RowVectorXi                  m_vecBadIdcs;                   /**< Idcs of bad channels */

    QPair<QVariant,QVariant>            m_pairBaseline;                 /**< Baseline information */

    bool                                m_bIsInit;                      /**< Init flag */
    bool                                m_bIsFreezed;                   /**< Display is freezed */
    bool                                m_bProjActivated;               /**< Doo projections flag */
    bool                                m_bCompActivated;               /**< Compensator activated */
    float                               m_fSps;                         /**< Sampling rate */
    qint32                              m_iMaxFilterLength;             /**< Max order of the current filters */

    QString                             m_sFilterChannelType;           /**< Kind of channel which is to be filtered */
    QList<FilterData>                   m_filterData;                   /**< List of currently active filters. */
    QStringList                         m_filterChannelList;            /**< List of channels which are to be filtered.*/
    QStringList                         m_visibleChannelList;           /**< List of currently visible channels in the view.*/

signals:
    //=========================================================================================================
    /**
    * Emmited when new selcetion was made
    *
    * @param [in] selection     list of all selected channels
    */
    void newSelection(QList<qint32> selection);

    //=========================================================================================================
    /**
    * Emmited when new average type has been received
    *
    * @param [in] qMapAverageColor     the average information map
    */
    void newAverageTypeReceived(QMap<double, QPair<QColor, QPair<QString,bool> > > qMapAverageColor);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


inline bool RealTimeEvokedSetModel::isInit() const
{
    return m_bIsInit;
}


//*************************************************************************************************************

inline qint32 RealTimeEvokedSetModel::getNumSamples() const
{
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return m_bIsInit ? iNumSamples : 0;
}


//*************************************************************************************************************

inline QVariant RealTimeEvokedSetModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

inline const QMap<qint32,qint32>& RealTimeEvokedSetModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}


//*************************************************************************************************************

inline qint32 RealTimeEvokedSetModel::numVLines() const
{
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return (qint32)(iNumSamples/m_fSps) - 1;
}


//*************************************************************************************************************

inline qint32 RealTimeEvokedSetModel::getNumPreStimSamples() const
{
    return m_pRTESet->getNumPreStimSamples();
}


//*************************************************************************************************************

inline float RealTimeEvokedSetModel::getSamplingFrequency() const
{
    return m_fSps;
}


//*************************************************************************************************************

inline bool RealTimeEvokedSetModel::isFreezed() const
{
    return m_bIsFreezed;
}


//*************************************************************************************************************

inline const QMap< qint32,float >& RealTimeEvokedSetModel::getScaling() const
{
    return m_qMapChScaling;
}


//*************************************************************************************************************

inline int RealTimeEvokedSetModel::getNumberOfTimeSpacers() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return floor((iNumSamples/m_fSps)*10);
}


//*************************************************************************************************************

inline QPair<QVariant,QVariant> RealTimeEvokedSetModel::getBaselineInfo() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    return m_pairBaseline;
}


//*************************************************************************************************************

inline int RealTimeEvokedSetModel::getNumAverages() const
{
    return m_matData.size();
}

} // NAMESPACE

#ifndef metatype_listrowvectorxd
#define metatype_listrowvectorxd
Q_DECLARE_METATYPE(SCDISPLIB::AvrTypeRowVector);    /**< Provides QT META type declaration of the Eigen::RowVectorXd type. For signal/slot usage.*/
#endif

#ifndef metatype_listrowvectorpair
#define metatype_listrowvectorpair
Q_DECLARE_METATYPE(SCDISPLIB::AvrTypeRowVectorPair);
#endif

#endif // REALTIMEEVOKEDSETMODEL_H
