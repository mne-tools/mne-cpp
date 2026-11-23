//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     frequencyspectrummodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.0
 * @date     May 2014
 * @brief    QAbstractTableModel storing per-channel FFT magnitudes for @ref mne::disp::SpectrumView.
 *
 * FrequencySpectrumModel maintains an @c Eigen::MatrixXd of channels
 * × frequency bins together with the active @c FiffInfo. New blocks
 * of FFT data are pushed through @c addData() which appends or
 * averages frames and emits @c dataChanged so the connected view
 * repaints the affected rows.
 */

#ifndef FREQUENCYSPECTRUMMODEL_H
#define FREQUENCYSPECTRUMMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include <mne/fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief QAbstractTableModel storing per-channel FFT magnitudes for @ref SpectrumView.
 *
 * Maintains an @c Eigen::MatrixXd of channels × frequency bins;
 * @c addData() appends or averages new FFT frames and emits
 * @c dataChanged so the view repaints the affected rows.
 */
class DISPSHARED_EXPORT FrequencySpectrumModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an real-time multi sample array table model for the given parent.
     *
     * @param[in] parent     parent of the table model.
     */
    FrequencySpectrumModel(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Returns the number of rows under the given parent. When the parent is valid it means that rowCount is returning the number of children of parent.
     *
     * @param[in] parent     not used.
     *
     * @return number of rows.
     */
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns the number of columns for the children of the given parent.
     *
     * @param[in] parent     not used.
     *
     * @return number of columns.
     */
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the item referred to by the index.
     *
     * @param[in] index      determines item location.
     * @param[in] role       role to return.
     *
     * @return accessed data.
     */
    virtual QVariant data(const QModelIndex& index,
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
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Sets corresponding fiff info
     *
     * @param[in] info       The corresponding fiff information object.
     */
    void setInfo(std::shared_ptr<mne::FiffInfo>& info);

    //=========================================================================================================
    /**
     * Sets Scale type
     *
     * @param[in] ScaleType       The corresponding scale type.
     */
    void setScaleType(qint8 ScaleType);

    //=========================================================================================================
    /**
     * Adds the frequency estimation
     *
     * @param[in] data   the frequency estimation.
     */
    void addData(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Returns the fiff info
     *
     * @return the fiff info.
     */
    inline std::shared_ptr<mne::FiffInfo> getInfo() const;

    //=========================================================================================================
    /**
     * Returns the frequency scale of the x axis
     *
     * @return the frequency scale of the x axis.
     */
    inline Eigen::RowVectorXd getFreqScale() const;

    //=========================================================================================================
    /**
     * Returns the frequency scale scaled to boundaries of the x axis
     *
     * @return the frequency scale of the x axis.
     */
    inline Eigen::RowVectorXd getFreqScaleBound() const;

    //=========================================================================================================
    /**
     * Returns the number of stems
     *
     * @return the number of stems.
     */
    inline qint32 getNumStems() const;

    //=========================================================================================================
    /**
     * Returns a map which conatins the channel idx and its corresponding selection status
     *
     * @return the channel idx to selection status.
     */
    inline const std::map<int, int>& getIdxSelMap() const;

    //=========================================================================================================
    /**
     * Selects the given list of channel indeces and unselect all other channels
     *
     * @param[in] selection      channel index list to select.
     */
    void selectRows(const std::vector<int>& selection);

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
    void toggleFreeze(const QModelIndex& index);

    //=========================================================================================================
    /**
     * Returns current freezing status
     *
     * @return the current freezing status.
     */
    inline bool isFreezed() const;

    //=========================================================================================================
    /**
     * Set plotting boundaries
     *
     * @param[in] fLowerFrqBound     Lower frequency boudnary.
     * @param[in] fUpperFrqBound     Upper frequency boudnary.
     */
    void setBoundaries(float fLowerFrqBound, float fUpperFrqBound);

    //=========================================================================================================
    /**
     * Returns the lower frequency boundary
     *
     * @return the lower frequency boundary.
     */
    inline qint32 getLowerFrqBound() const;

    //=========================================================================================================
    /**
     * Returns the upper frequency boundary
     *
     * @return the upper frequency boundary.
     */
    inline qint32 getUpperFrqBound() const;

signals:
    //=========================================================================================================
    /**
     * Emmited when new selcetion was made
     *
     * @param[in] selection     list of all selected channels.
     */
    void newSelection(const std::vector<int>& selection);

private:
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< Fiff Information.*/

    std::map<int, int> m_qMapIdxRowSelection; /**< Selection mapping.*/

    Eigen::RowVectorXd m_vecFreqScale;      /**< Frequency scale. */
    Eigen::RowVectorXd m_vecFreqScaleBound; /**< Frequency scaled to boundaries. */
    Eigen::MatrixXd m_dataCurrent;          /**< List that holds the current data*/
    Eigen::MatrixXd m_dataCurrentFreeze;    /**< List that holds the current data when freezed*/

    float m_fSps;          /**< Sampling rate. */
    qint32 m_iT;           /**< Time window. */
    qint32 m_iLowerFrqIdx; /**< Upper frequency plotting boundary. */
    qint32 m_iUpperFrqIdx; /**< Lower frequency plotting boundary. */
    qint8 m_iScaleType;    /**< The display scale type. */
    bool m_bIsFreezed;     /**< Display is freezed. */
    bool m_bInitialized;   /**< If it's initailized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

std::shared_ptr<mne::FiffInfo> FrequencySpectrumModel::getInfo() const
{
    return m_pFiffInfo;
}

//=============================================================================================================

Eigen::RowVectorXd FrequencySpectrumModel::getFreqScale() const
{
    return m_vecFreqScale;
}

//=============================================================================================================

Eigen::RowVectorXd FrequencySpectrumModel::getFreqScaleBound() const
{
    return m_vecFreqScaleBound;
}

//=============================================================================================================

inline qint32 FrequencySpectrumModel::getNumStems() const
{
    return m_dataCurrent.cols();
}

//=============================================================================================================

inline const std::map<int, int>& FrequencySpectrumModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}

//=============================================================================================================

inline bool FrequencySpectrumModel::isFreezed() const
{
    return m_bIsFreezed;
}

//=============================================================================================================

inline qint32 FrequencySpectrumModel::getLowerFrqBound() const
{
    return m_iLowerFrqIdx;
}

//=============================================================================================================

inline qint32 FrequencySpectrumModel::getUpperFrqBound() const
{
    return m_iUpperFrqIdx;
}
} // NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#endif

#endif // FREQUENCYSPECTRUMMODEL_H
