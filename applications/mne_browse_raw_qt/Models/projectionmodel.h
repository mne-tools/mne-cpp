//=============================================================================================================
/**
* @file     projectionmodel.h
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
* @brief    This class represents the projection model of the model/view framework of mne_browse_raw_qt application.
*
*/

#ifndef PROJECTIONMODEL_H
#define PROJECTIONMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

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
* DECLARE CLASS ProjectionModel
*/
class ProjectionModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ProjectionModel(QObject *parent = 0);
    ProjectionModel(QObject *parent, QFile& qFile);
    ProjectionModel(QObject *parent, QList<FiffProj>& dataProjs);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
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
    * loadProjections loads projections from a fif file
    *
    * @param qFile fiff data file containing the projections
    */
    bool loadProjections(QFile& qFile);

    //=========================================================================================================
    /**
    * saveProjections saves projections to a fif file
    *
    * @param qFile fiff data file containing the projections
    */
    bool saveProjections(QFile& qFile);

    //=========================================================================================================
    /**
    * addProjections adds projections to the data
    *
    * @param dataProjs fiff list with already loaded projectors.
    */
    void addProjections(const QList<FiffProj>& dataProjs);

    //=========================================================================================================
    /**
    * addProjections adds projections to the data
    *
    * @param fiffInfo fiff info with already loaded projectors.
    */
    void addProjections(const FiffInfo &fiffInfo);

    bool                        m_bFileloaded;          /**< true when a Fiff evoked file is loaded. */

    //=========================================================================================================
    /**
    * clearModel clears all model's members
    */
    void clearModel();

protected:
    QList<FiffProj>             m_dataProjs;            /**< current projector data. */

signals:
    //=========================================================================================================
    /**
    * fileLoaded is emitted whenever a file was (tried) to be loaded
    */
    void fileLoaded(bool);
};

} // NAMESPACE



#endif // PROJECTIONMODEL_H
