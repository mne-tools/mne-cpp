//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file evokedsetmodel.h
 * @since 2022
 * @date  March 2026
 * @brief QAbstractTableModel wrapping a FiffEvokedSet so views can render per-condition averages without copying data.
 *
 * EvokedSetModel exposes one row per FIFF channel and one column per
 * condition; each cell yields the underlying time-series view
 * (plus colour / visibility) needed by @ref ButterflyView and
 * @ref AverageLayoutView. It also propagates baseline-correction,
 * SSP-projection and bad-channel updates so the visualisations stay
 * in sync with the latest pre-processing settings.
 */

#ifndef EVOKEDSETMODEL_H
#define EVOKEDSETMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <dsp/filterkernel.h>
#include <fiff/fiff_types.h>

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
    class FiffEvokedSet;
    class FiffProj;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

namespace EvokedSetModelRoles {
    enum ItemRole{GetAverageData = Qt::UserRole + 1020};
}

//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> MatrixXdR;
typedef QPair<const double*,qint32> RowVectorPair;
typedef QPair<QString, Eigen::RowVectorXd> AvrTypeRowVector;
typedef QPair<QString, DISPLIB::RowVectorPair> AvrTypeRowVectorPair;

//=============================================================================================================
/**
 * @brief QAbstractTableModel wrapping a FiffEvokedSet (one row per channel, one column per condition).
 *
 * Propagates baseline-correction, SSP-projection and bad-channel
 * updates so connected views (@ref ButterflyView,
 * @ref AverageLayoutView) stay in sync with the latest
 * pre-processing settings.
 */
class DISPSHARED_EXPORT EvokedSetModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<EvokedSetModel> SPtr;              /**< Shared pointer type for EvokedSetModel. */
    typedef QSharedPointer<const EvokedSetModel> ConstSPtr;   /**< Const shared pointer type for EvokedSetModel. */

    //=========================================================================================================
    /**
     * Constructs an real-time multi sample array table model for the given parent.
     *
     * @param[in] parent     parent of the table model.
     */
    EvokedSetModel(QObject *parent = 0);

    ~EvokedSetModel();

    //=========================================================================================================
    /**
     * Returns whether this class is initalized.
     *
     * @return Flag specifying whether this class is initalized.
     */
    bool isInit() const;

    //=========================================================================================================
    /**
     * Returns the number of samples.
     *
     * @return The number of samples.
     */
    qint32 getNumSamples() const;

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
     * Data for the row and column and given display role
     *
     * @param[in] row       index row.
     * @param[in] column    index column.
     * @param[in] role      display role to access.
     *
     * @return the accessed data.
     */
    QVariant data(int row,
                  int column,
                  int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the item referred to by the index.
     *
     * @param[in] index      determines item location.
     * @param[in] role       role to return.
     *
     * @return accessed data.
     */
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;

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
    virtual QVariant headerData(int section,
                                Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    void init();

    //=========================================================================================================
    /**
     * Sets corresponding evoked set
     *
     * @param[in] pEvokedSet      The evoked set.
     */
    void setEvokedSet(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet);

    //=========================================================================================================
    QSharedPointer<FIFFLIB::FiffEvokedSet> getEvokedSet();

    //=========================================================================================================
    /**
     * Update stored data
     */
    void updateData();

    //=========================================================================================================
    /**
     * Get the current average colors
     *
     * @return Pointer to the current average colors.
     */
    QSharedPointer<QMap<QString, QColor> > getAverageColor() const;

    //=========================================================================================================
    /**
     * Get the current average activations
     *
     * @return Pointer to the current average activations.
     */
    QSharedPointer<QMap<QString, bool> > getAverageActivation() const;

    //=========================================================================================================
    /**
     * Set the average colors
     *
     * @param[in] qMapAverageColor      Pointer to the new average colors.
     */
    void setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
     * Set the average activations
     *
     * @param[in] qMapAverageActivation      Pointer to the new average activations.
     */
    void setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);

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
     * Returns true or fals whether the provided channel is bad.
     *
     * @param[in] row    row number which correspodns to a given channel.
     *
     * @return Returns true or fals whether the provided channel is bad.
     */
    bool getIsChannelBad(qint32 row) const;

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
     * Returns a map which conatins the channel idx and its corresponding selection status
     *
     * @return the channel idx to selection status.
     */
    const QMap<qint32,qint32>& getIdxSelMap() const;

    //=========================================================================================================
    /**
     * Returns the current number for the time spacers
     *
     * @return the current number for the time spacers.
     */
    int getNumberOfTimeSpacers() const;

    //=========================================================================================================
    /**
     * Returns the current baseline information
     *
     * @return the current baseline information as a from to QPair.
     */
    QPair<QVariant,QVariant> getBaselineInfo() const;

    //=========================================================================================================
    /**
     * Returns the current number of stored averages
     *
     * @return the current number of stored averages.
     */
    int getNumAverages() const;

    //=========================================================================================================
    /**
     * Returns the number of pre-stimulus samples
     *
     * @return the number of pre-stimulus samples.
     */
    qint32 getNumPreStimSamples() const;

    //=========================================================================================================
    /**
     * Returns the current sampling frequency
     *
     * @return the current sampling frequency.
     */
    float getSamplingFrequency() const;

    //=========================================================================================================
    /**
     * Selects the given list of channel indeces and unselect all other channels
     *
     * @param[in] selection      channel index list to select.
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
     * @return number of vertical lines.
     */
    qint32 numVLines() const;

    //=========================================================================================================
    /**
     * Returns current freezing status
     *
     * @return the current freezing status.
     */
    bool isFreezed() const;

    //=========================================================================================================
    /**
     * Update projections
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
     * Toggle freeze for all channels when a channel is double clicked
     */
    void toggleFreeze();

private:
    QSharedPointer<FIFFLIB::FiffEvokedSet>  m_pEvokedSet;                   /**< The evoked set measurement. */

    QMap<qint32,qint32>                     m_qMapIdxRowSelection;          /**< Selection mapping.*/
    QSharedPointer<QMap<QString, QColor> >  m_qMapAverageColor;             /**< Average colors. */
    QSharedPointer<QMap<QString, bool> >    m_qMapAverageActivation;        /**< Average activation status. */
    QSharedPointer<QMap<QString, QColor> >  m_qMapAverageColorOld;          /**< Average colors. */
    QSharedPointer<QMap<QString, bool> >    m_qMapAverageActivationOld;     /**< Average activation status. */

    QList<Eigen::MatrixXd>                  m_matData;                      /**< List that holds the data*/
    QList<Eigen::MatrixXd>                  m_matDataFreeze;                /**< List that holds the data when freezed*/
    QStringList                             m_lAvrTypes;                    /**< The average types. */

    Eigen::MatrixXd                         m_matProj;                      /**< SSP projector. */
    Eigen::MatrixXd                         m_matComp;                      /**< Compensator. */
    Eigen::SparseMatrix<double>             m_matSparseProjCompMult;        /**< The final sparse projection + compensator operator.*/
    Eigen::SparseMatrix<double>             m_matSparseProjMult;            /**< The final sparse SSP projector. */
    Eigen::SparseMatrix<double>             m_matSparseCompMult;            /**< The final sparse compensator matrix. */

    Eigen::RowVectorXi                      m_vecBadIdcs;                   /**< Idcs of bad channels. */

    QPair<QVariant,QVariant>                m_pairBaseline;                 /**< Baseline information. */

    bool                                    m_bIsInit;                      /**< Init flag. */
    bool                                    m_bIsFreezed;                   /**< Display is freezed. */
    bool                                    m_bProjActivated;               /**< Doo projections flag. */
    bool                                    m_bCompActivated;               /**< Compensator activated. */
    float                                   m_fSps;                         /**< Sampling rate. */

signals:
    //=========================================================================================================
    /**
     * Emmited when new selcetion was made
     *
     * @param[in] selection     list of all selected channels.
     */
    void newSelection(QList<qint32> selection);

    //=========================================================================================================
    /**
     * Emmited when new average color is available
     *
     * @param[in] qMapAverageColor     the average color map.
     */
    void newAverageColorMap(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
     * Emmited when new average activation is available
     *
     * @param[in] qMapAverageActivation     the average activation map.
     */
    void newAverageActivationMap(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#ifndef metatype_listrowvectorxd
#define metatype_listrowvectorxd
Q_DECLARE_METATYPE(DISPLIB::AvrTypeRowVector);    /**< Provides QT META type declaration of the Eigen::RowVectorXd type. For signal/slot usage.*/
#endif

#ifndef metatype_listrowvectorpair
#define metatype_listrowvectorpair
Q_DECLARE_METATYPE(DISPLIB::AvrTypeRowVectorPair);
#endif

#endif // EVOKEDSETMODEL_H
