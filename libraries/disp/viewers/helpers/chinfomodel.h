//=============================================================================================================
/**
* @file     chinfomodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     November, 2014
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
* @brief    This class represents the channel info model of the model/view framework of mne_browse application.
*
*/

#ifndef CHINFOMODEL_H
#define CHINFOMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QSharedPointer>
#include <QPointF>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
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

class MNEOperator;

namespace ChInfoModelRoles
{
    enum ItemRole{GetOrigChName = Qt::UserRole + 1009,
                  GetMappedLayoutChName = Qt::UserRole + 1010,
                  GetChNumber = Qt::UserRole + 1011,
                  GetChKind = Qt::UserRole + 1012,
                  GetMEGType = Qt::UserRole + 1013,
                  GetChUnit = Qt::UserRole + 1014,
                  GetChAlias = Qt::UserRole + 1015,
                  GetChPosition = Qt::UserRole + 1016,
                  GetChDigitizer = Qt::UserRole + 1017,
                  GetChActiveFilter = Qt::UserRole + 1018,
                  GetChCoilType = Qt::UserRole + 1019,
                  GetIsBad = Qt::UserRole + 1020};
}


//=============================================================================================================
/**
* DECLARE CLASS ChInfoModel
*/
class DISPSHARED_EXPORT ChInfoModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    typedef QSharedPointer<ChInfoModel> SPtr;              /**< Shared pointer type for ChInfoModel. */
    typedef QSharedPointer<const ChInfoModel> ConstSPtr;   /**< Const shared pointer type for ChInfoModel. */

    ChInfoModel(QSharedPointer<FIFFLIB::FiffInfo>& pFiffInfo, QObject *parent = 0);
    ChInfoModel(QObject *parent = 0);

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
    * Updates the fiff info
    *
    * @param fiffInfo fiff info variabel.
    */
    void fiffInfoChanged(QSharedPointer<FIFFLIB::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
    * Updates the fiff info
    *
    * @param assignedOperators the filter operators which are currently active.
    */
    void assignedOperatorsChanged(const QMap<int,QSharedPointer<MNEOperator> > &assignedOperators);

    //=========================================================================================================
    /**
    * Updates the layout map
    *
    * @param layoutMap the layout map with the 2D positions.
    */
    void layoutChanged(const QMap<QString,QPointF> &layoutMap);

    //=========================================================================================================
    /**
    * Updates the layout map
    *
    * @return the current mapped channel list
    */
    const QStringList & getMappedChannelsList();

    //=========================================================================================================
    /**
    * Returns the model index for the given input channel fro mthe original channel list
    *
    * @param chName the channel name for which the model index is needed.
    * @return the index number. if channel was not found in the data this functions returns -1
    */
    int getIndexFromOrigChName(QString chName);

    //=========================================================================================================
    /**
    * Returns the model index for the given input channel fro mthe mapped channel list
    *
    * @param chName the channel name for which the model index is needed.
    * @return the index number. if channel was not found in the data this functions returns -1
    */
    int getIndexFromMappedChName(QString chName);

    //=========================================================================================================
    /**
    * Returns bad channel list.
    *
    * @return the bad channel list.
    */
    QStringList getBadChannelList();

protected:
    //=========================================================================================================
    /**
    * clearModel clears all model's members
    */
    void clearModel();

    //=========================================================================================================
    /**
    * Maps the currently loaded channels to the loaded layout file
    */
    void mapLayoutToChannels();

    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;            /**< The fiff info of the currently loaded fiff file. */

    QMap<int,QSharedPointer<MNEOperator> >      m_assignedOperators;    /**< Map of MNEOperator types to channels.*/
    QMap<QString,QPointF>                       m_layoutMap;            /**< The current layout map with a position for all MEG and EEG channels. */

    QStringList                 m_aliasNames;           /**< list of given channel aliases. */
    QStringList                 m_mappedLayoutChNames;  /**< list of the mapped layout channel names. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever channels where mapped to a layout
    *
    */
    void channelsMappedToLayout(const QStringList &mappedLayoutChNames);
};

} // NAMESPACE DISPLIB

#endif // CHINFOMODEL_H
