//=============================================================================================================
/**
* @file     averagemodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     October, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the average model of the model/view framework of mne_browse_raw_qt application.
*
*/

#ifndef AVERAGEMODEL_H
#define AVERAGEMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/rawsettings.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractListModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{

//=============================================================================================================
/**
* DECLARE CLASS AverageModel
*/
class AverageModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AverageModel(QObject *parent = 0);
    AverageModel(QFile& qFile, QObject *parent);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
    *
    */
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool insertRows(int position, int span, const QModelIndex & parent = QModelIndex());
    bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());

    //=========================================================================================================
    /**
    * loadEvokedData loads the fiff evoked data file
    *
    * @param p_IODevice fiff data evoked file to read from
    */
    bool loadEvokedData(QFile& qFile);

    //=========================================================================================================
    /**
    * saveEvokedData saves the fiff evoked data file
    *
    * @param p_IODevice fiff data evoked file to save to
    */
    bool saveEvokedData(QFile& qFile);

    bool                        m_bFileloaded;          /**< true when a Fiff evoked file is loaded */

protected:
    FiffEvokedSet*              m_pEvokedDataSet;       /**< QList<FiffEvoked> that holds the evoked data sets which are to be organised and handled by this model */
    QSharedPointer<FiffIO>      m_pfiffIO;              /**< FiffIO objects, which holds all the information of the fiff data (excluding the samples!) */

    //=========================================================================================================
    /**
    * clearModel clears all model's members
    */
    void clearModel();

signals:
    //=========================================================================================================
    /**
    * fileLoaded is emitted whenever a file was (tried) to be loaded
    */
    void fileLoaded(bool);

public slots:

};

} // NAMESPACE

Q_DECLARE_METATYPE(Eigen::MatrixXd);
Q_DECLARE_METATYPE(FIFFLIB::FiffInfo);
Q_DECLARE_METATYPE(Eigen::RowVectorXf);

#endif // AVERAGEMODEL_H
