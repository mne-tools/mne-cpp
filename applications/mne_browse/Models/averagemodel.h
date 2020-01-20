//=============================================================================================================
/**
 * @file     averagemodel.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    This class represents the average model of the model/view framework of mne_browse application.
 *
 */

#ifndef AVERAGEMODEL_H
#define AVERAGEMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/rawsettings.h"
#include "../Utils/types.h"


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
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// Forward Declarations
//=============================================================================================================

namespace FIFFLIB
{
    class FiffIO;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS AverageModel
 */
class AverageModel : public QAbstractTableModel
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
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual bool insertRows(int position, int span, const QModelIndex & parent = QModelIndex());
    virtual bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());

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

    //=========================================================================================================
    /**
     * getFiffInfo returns the fiff info
     *
     * @param FiffInfo of the evoked file
     */
    const FiffInfo getFiffInfo();

    bool                        m_bFileloaded;          /**< true when a Fiff evoked file is loaded. */

protected:
    FiffEvokedSet::SPtr         m_pEvokedDataSet;       /**< QList<FiffEvoked> that holds the evoked data sets which are to be organised and handled by this model. */
    QSharedPointer<FiffIO>      m_pfiffIO;              /**< FiffIO objects, which holds all the information of the fiff data (excluding the samples!). */

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
};

} // NAMESPACE



#endif // AVERAGEMODEL_H
