//=============================================================================================================
/**
* @file     brainrtdata.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     BrainRTDataTreeItem class declaration.
*
*/

#ifndef BRAINRTDATATREEITEM_H
#define BRAINRTDATATREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3DNew_global.h"

#include "../../helpers/abstracttreeitem.h"
#include "../../helpers/types.h"

#include "mne/mne_sourceestimate.h"
#include "mne/mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* BrainRTDataTreeItem provides a generic item to hold information about real time data to plot onto the brain surface.
*
* @brief Provides a generic brain tree item to hold real time data.
*/
class DISP3DNEWSHARED_EXPORT BrainRTDataTreeItem : public AbstractTreeItem
{

public:
    typedef QSharedPointer<BrainRTDataTreeItem> SPtr;             /**< Shared pointer type for BrainRTDataTreeItem class. */
    typedef QSharedPointer<const BrainRTDataTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainRTDataTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    */
    explicit BrainRTDataTreeItem(const int &iType = BrainTreeModelItemTypes::UnknownItem, const QString &text = "RT Data");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainRTDataTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant &value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on annotation information to this model.
    *
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] tAnnotation        FreeSurfer annotation.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const MNESourceEstimate & tSourceEstimate, const MNEForwardSolution & tForwardSolution, const QString & hemi = "Unknown");

    bool updateData(const MNESourceEstimate & tSourceEstimate);

    inline bool isInit() const;

private:
    bool        m_bInit;
    QString     m_sHemi;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool BrainRTDataTreeItem::isInit() const
{
    return m_bInit;
}

} //NAMESPACE DISP3DNEWLIB

#endif // BRAINRTDATATREEITEM_H
