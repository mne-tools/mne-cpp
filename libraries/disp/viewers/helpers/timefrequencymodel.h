//=============================================================================================================
/**
 * @file     timefrequencymodel.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the TimeFrequencyModel Class.
 *
 */

#ifndef TIMEFREQUENCYMODEL_H
#define TIMEFREQUENCYMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLinearGradient>
#include <QAbstractTableModel>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

class DISPSHARED_EXPORT TimeFrequencyModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    TimeFrequencyModel();

    TimeFrequencyModel(std::vector<Eigen::MatrixXd>& spectr);

    void setSpectr(std::vector<Eigen::MatrixXd>& spectr);

    void setFiffInfo(const FIFFLIB::FiffInfo& info);

    float getSamplingFrequency();

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
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;

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
    QVariant data(int row,
                  int column,
                  int role = Qt::DisplayRole) const;

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
    virtual QVariant headerData(int section,
                                Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;


private:
//    QSharedPointer<DISPLIB::ChannelInfoModel>                   m_pChannelInfoModel;

//    QLinearGradient         m_Gradient;

    std::vector<Eigen::MatrixXd> m_vSpectr;

    FIFFLIB::FiffInfo                    m_Info;

};
}//namespace

#ifndef metatype_matrixXd
#define metatype_matrixXd
Q_DECLARE_METATYPE(Eigen::MatrixXd);
#endif

#endif // TIMEFREQUENCYMODEL_H
