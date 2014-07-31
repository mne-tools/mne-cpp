//=============================================================================================================
/**
* @file     noiseestimationmodel.h
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
* @brief    Declaration of the NoiseEstimationModel Class.
*
*/

#ifndef NOISEESTIMATIONMODEL_H
#define NOISEESTIMATIONMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_info.h>


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
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS NoiseEstimationModel
*
* @brief The NoiseEstimationModel class implements the data access model for a real-time multi sample array data stream
*/
class NoiseEstimationModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs an real-time multi sample array table model for the given parent.
    *
    * @param[in] parent     parent of the table model
    */
    NoiseEstimationModel(QObject *parent = 0);

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
    * Sets corresponding fiff info
    *
    * @param [in] inf       The corresponding fiff information object
    */
    void setInfo(FiffInfo::SPtr &info);

    //=========================================================================================================
    /**
    * Adds the frequency estimation
    *
    * @param[in] data   the frequency estimation
    */
    void addData(const MatrixXd &data);

    //=========================================================================================================
    /**
    * Returns the fiff info
    *
    * @return the fiff info
    */
    inline FiffInfo::SPtr getInfo() const;

    //=========================================================================================================
    /**
    * Returns the frequency scale of the x axis
    *
    * @return the frequency scale of the x axis
    */
    inline RowVectorXd getFreqScale() const;

    //=========================================================================================================
    /**
    * Returns the number of stems
    *
    * @return the number of stems
    */
    inline qint32 getNumStems() const;

    //=========================================================================================================
    /**
    * Returns a map which conatins the channel idx and its corresponding selection status
    *
    * @return the channel idx to selection status
    */
    inline const QMap<qint32,qint32>& getIdxSelMap() const;

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
    * Toggle freeze for all channels when a channel is double clicked
    *
    * @param [in] index     of the channel which has been double clicked
    */
    void toggleFreeze(const QModelIndex &index);

    //=========================================================================================================
    /**
    * Returns current freezing status
    *
    * @return the current freezing status
    */
    inline bool isFreezed() const;

signals:
    //=========================================================================================================
    /**
    * Emmited when new selcetion was made
    *
    * @param [in] selection     list of all selected channels
    */
    void newSelection(QList<qint32> selection);

private:
    FiffInfo::SPtr m_pFiffInfo; /**< Fiff Information.*/

    QMap<qint32,qint32> m_qMapIdxRowSelection;  /**< Selection mapping.*/

    RowVectorXd m_vecFreqScale;                 /**< Frequency scale */

    //Fiff data structure
    MatrixXd m_dataCurrent;         /**< List that holds the current data*/

    MatrixXd m_dataCurrentFreeze;   /**< List that holds the current data when freezed*/

    float m_fSps;               /**< Sampling rate */
    qint32 m_iT;                /**< Time window */

    bool m_bIsFreezed;          /**< Display is freezed */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

FiffInfo::SPtr NoiseEstimationModel::getInfo() const
{
    return m_pFiffInfo;
}


//*************************************************************************************************************

RowVectorXd NoiseEstimationModel::getFreqScale() const
{
    return m_vecFreqScale;
}


//*************************************************************************************************************

inline qint32 NoiseEstimationModel::getNumStems() const
{
    return m_dataCurrent.cols();
}


//*************************************************************************************************************

inline const QMap<qint32,qint32>& NoiseEstimationModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}


//*************************************************************************************************************

inline bool NoiseEstimationModel::isFreezed() const
{
    return m_bIsFreezed;
}

} // NAMESPACE

#ifndef metatype_rowvectorxd
#define metatype_rowvectorxd
Q_DECLARE_METATYPE(Eigen::RowVectorXd);    /**< Provides QT META type declaration of the Eigen::RowVectorXd type. For signal/slot usage.*/
#endif

#endif // NOISEESTIMATIONMODEL_H
