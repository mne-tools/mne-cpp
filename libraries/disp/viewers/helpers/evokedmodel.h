//=============================================================================================================
/**
* @file     evokedmodel.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the EvokedModel Class.
*
*/

#ifndef EVOKEDMODEL_H
#define EVOKEDMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffEvoked;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

namespace EvokedModelRoles
{
    enum ItemRole{GetAverageData = Qt::UserRole + 1020};
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

typedef Eigen::Matrix<double,Dynamic,Dynamic,RowMajor> MatrixXdR;
typedef QPair<const double*,qint32> RowVectorPair;
typedef QPair<double, Eigen::RowVectorXd> AvrTypeRowVector;
typedef QPair<double, SCDISPLIB::RowVectorPair> AvrTypeRowVectorPair;


//=============================================================================================================
/**
* DECLARE CLASS EvokedModel
*
* @brief The EvokedModel class implements the data access model for a evoked data structure
*/
class EvokedModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<EvokedModel> SPtr;              /**< Shared pointer type for EvokedModel. */
    typedef QSharedPointer<const EvokedModel> ConstSPtr;   /**< Const shared pointer type for EvokedModel. */

    //=========================================================================================================
    /**
    * Constructs an real-time multi sample array table model for the given parent.
    *
    * @param[in] parent     parent of the table model
    */
    EvokedModel(QObject *parent = 0);
    ~EvokedModel();

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
    * Sets corresponding evoked data
    *
    * @param [in] pEvoked      The real-time evoked
    */
    void setRTE(QSharedPointer<FIFFLIB::FiffEvoked> &pEvoked);

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
    FIFFLIB::fiff_int_t getKind(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns the unit of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return unit of given channel number
    */
    FIFFLIB::fiff_int_t getUnit(qint32 row) const;

    //=========================================================================================================
    /**
    * Returns the coil type of a given channel number
    *
    * @param[in] row    row number which correspodns to a given channel
    *
    * @return coil type of given channel number
    */
    FIFFLIB::fiff_int_t getCoil(qint32 row) const;

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

signals:
    //=========================================================================================================
    /**
    * Emmited when new selcetion was made
    *
    * @param [in] selection     list of all selected channels
    */
    void newSelection(QList<qint32> selection);

private:
    //=========================================================================================================
    /**
    * Calculates the filtered version of the channels in m_matData
    */
    void filterChannelsConcurrently();

    QSharedPointer<FIFFLIB::FiffEvoked> m_pEvoked;                      /**< The evoked measurement. */

    QMap<qint32,qint32>                 m_qMapIdxRowSelection;          /**< Selection mapping.*/
    QMap<qint32,float>                  m_qMapChScaling;                /**< Channel scaling map. */

    Eigen::MatrixXd                     m_matData;                      /**< List that holds the data*/
    Eigen::MatrixXd                     m_matDataFreeze;                /**< List that holds the data when freezed*/

    Eigen::MatrixXd                     m_matProj;                      /**< SSP projector */
    Eigen::MatrixXd                     m_matComp;                      /**< Compensator */

    Eigen::MatrixXd                     m_matDataFiltered;              /**< The filtered data */
    Eigen::MatrixXd                     m_matDataFilteredFreeze;        /**< The raw filtered data in freeze mode */

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
    QList<UTILSLIB::FilterData>         m_filterData;                   /**< List of currently active filters. */
    QStringList                         m_filterChannelList;            /**< List of channels which are to be filtered.*/
    QStringList                         m_visibleChannelList;           /**< List of currently visible channels in the view.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


inline bool EvokedModel::isInit() const
{
    return m_bIsInit;
}


//*************************************************************************************************************

inline qint32 EvokedModel::getNumSamples() const
{
    return m_bIsInit ? m_matData.cols() : 0;
}


//*************************************************************************************************************

inline QVariant EvokedModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

inline const QMap<qint32,qint32>& EvokedModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}


//*************************************************************************************************************

inline qint32 EvokedModel::numVLines() const
{
    return (qint32)(m_matData.cols()/m_fSps) - 1;
}


//*************************************************************************************************************

inline qint32 EvokedModel::getNumPreStimSamples() const
{
    return m_pRTE->getNumPreStimSamples();
}


//*************************************************************************************************************

inline float EvokedModel::getSamplingFrequency() const
{
    return m_fSps;
}


//*************************************************************************************************************

inline bool EvokedModel::isFreezed() const
{
    return m_bIsFreezed;
}


//*************************************************************************************************************

inline const QMap< qint32,float >& EvokedModel::getScaling() const
{
    return m_qMapChScaling;
}


//*************************************************************************************************************

inline int EvokedModel::getNumberOfTimeSpacers() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    return floor((m_matData.cols()/m_fSps)*10);
}


//*************************************************************************************************************

inline QPair<QVariant,QVariant> EvokedModel::getBaselineInfo() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    return m_pairBaseline;
}

} // NAMESPACE

#ifndef metatype_rowvectorxd
#define metatype_rowvectorxd
Q_DECLARE_METATYPE(Eigen::RowVectorXd);    /**< Provides QT META type declaration of the Eigen::RowVectorXd type. For signal/slot usage.*/
#endif

#ifndef metatype_rowvectorpair
#define metatype_rowvectorpair
Q_DECLARE_METATYPE(SCDISPLIB::RowVectorPair);
#endif

#endif // EVOKEDMODEL_H
