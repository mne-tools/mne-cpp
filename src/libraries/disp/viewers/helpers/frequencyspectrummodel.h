//=============================================================================================================
/**
 * @file     frequencyspectrummodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the FrequencySpectrumModel Class.
 *
 */

#ifndef FREQUENCYSPECTRUMMODEL_H
#define FREQUENCYSPECTRUMMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
/**
 * DECLARE CLASS FrequencySpectrumModel
 *
 * @brief The FrequencySpectrumModel class implements the data access model for a real-time multi sample array data stream
 */
class DISPSHARED_EXPORT FrequencySpectrumModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<FrequencySpectrumModel> SPtr;              /**< Shared pointer type for FrequencySpectrumModel. */
    typedef QSharedPointer<const FrequencySpectrumModel> ConstSPtr;   /**< Const shared pointer type for FrequencySpectrumModel. */

    //=========================================================================================================
    /**
     * Constructs an real-time multi sample array table model for the given parent.
     *
     * @param[in] parent     parent of the table model.
     */
    FrequencySpectrumModel(QObject *parent = 0);

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
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Sets corresponding fiff info
     *
     * @param[in] inf       The corresponding fiff information object.
     */
    void setInfo(QSharedPointer<FIFFLIB::FiffInfo> &info);

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
    void addData(const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
     * Returns the fiff info
     *
     * @return the fiff info.
     */
    inline QSharedPointer<FIFFLIB::FiffInfo> getInfo() const;

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
    void newSelection(QList<qint32> selection);

private:
    QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;  /**< Fiff Information.*/

    QMap<qint32,qint32>     m_qMapIdxRowSelection;  /**< Selection mapping.*/

    Eigen::RowVectorXd      m_vecFreqScale;         /**< Frequency scale. */
    Eigen::RowVectorXd      m_vecFreqScaleBound;    /**< Frequency scaled to boundaries. */
    Eigen::MatrixXd         m_dataCurrent;          /**< List that holds the current data*/
    Eigen::MatrixXd         m_dataCurrentFreeze;    /**< List that holds the current data when freezed*/

    float       m_fSps;                 /**< Sampling rate. */
    qint32      m_iT;                   /**< Time window. */
    qint32      m_iLowerFrqIdx;         /**< Upper frequency plotting boundary. */
    qint32      m_iUpperFrqIdx;         /**< Lower frequency plotting boundary. */
    qint8       m_iScaleType;           /**< The display scale type. */
    bool        m_bIsFreezed;           /**< Display is freezed. */
    bool        m_bInitialized;         /**< If it's initailized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QSharedPointer<FIFFLIB::FiffInfo> FrequencySpectrumModel::getInfo() const
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

inline const QMap<qint32,qint32>& FrequencySpectrumModel::getIdxSelMap() const
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
#ifndef metatype_rowvectorxd
#define metatype_rowvectorxd
Q_DECLARE_METATYPE(Eigen::RowVectorXd);    /**< Provides QT META type declaration of the Eigen::RowVectorXd type. For signal/slot usage.*/
#endif
#endif

#endif // FREQUENCYSPECTRUMMODEL_H
