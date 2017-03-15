//=============================================================================================================
/**
* @file     networktreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     NetworkTreeItem class declaration.
*
*/

#ifndef NETWORKTREEITEM_H
#define NETWORKTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

#include "../common/abstracttreeitem.h"
#include "../common/types.h"

#include <connectivity/network/network.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEForwardSolution;
}

namespace FSLIB {
    class Surface;
}

namespace Qt3DCore {
    class QEntity;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Renderable3DEntity;
class MetaTreeItem;


//=============================================================================================================
/**
* NetworkTreeItem provides a generic item to hold information about real time connectivity data to plot onto the brain surface.
*
* @brief Provides a generic brain tree item to hold real time data.
*/
class DISP3DNEWSHARED_EXPORT NetworkTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<NetworkTreeItem> SPtr;             /**< Shared pointer type for NetworkTreeItem class. */
    typedef QSharedPointer<const NetworkTreeItem> ConstSPtr;  /**< Const shared pointer type for NetworkTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit NetworkTreeItem(int iType = Data3DTreeModelItemTypes::RTConnectivityDataItem, const QString& text = "Connectivity Data");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~NetworkTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Initializes the rt connectivity data item with neccessary information for visualization computations.
    *
    * @param[in] parent                 The Qt3D entity parent of the new item.
    */
    void initData(Qt3DCore::QEntity *parent);

    //=========================================================================================================
    /**
    * Adds actual rt connectivity data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
    *
    * @param[in] tNetworkData       The new connectivity data.
    */
    void addData(const CONNECTIVITYLIB::Network& tNetworkData);

    //=========================================================================================================
    /**
    * Updates the rt connectivity data which is streamed by this item's worker thread item.
    *
    * @return                       Returns true if this item is initialized.
    */
    inline bool isDataInit() const;

private:
    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();

    //=========================================================================================================
    /**
    * Call this function whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * Call this function whenever you want to change the visibilty of the 3D rendered content.
    *
    * @param[in] state     The visiblity flag.
    */
    void setVisible(bool state);

    //=========================================================================================================
    /**
    * This function gets called whenever the network threshold changes.
    *
    * @param[in] vecThresholds     The new threshold values used for threshold the network.
    */
    void onNetworkThresholdChanged(const QVector3D& vecThresholds);

    //=========================================================================================================
    /**
    * Call this function whenever you want to calculate the indices/tris for a network.
    *
    * @param[in] tNetworkData     The network data.
    * @param[in] vecThreshold     The threshold data.
    */
    void plotNetwork(const CONNECTIVITYLIB::Network& tNetworkData, const QVector3D& vecThreshold);

    bool                                        m_bDataIsInit;                  /**< The data init flag. */
    bool                                        m_bNodesPlotted;                /**< Flag whether nodes were plotted. */

    MetaTreeItem*                               m_pItemNetworkThreshold;        /**< The item to access the threshold values. */

    QPointer<Renderable3DEntity>                m_pRenderable3DEntity;          /**< The renderable 3D entity. */

    QList<QPointer<Renderable3DEntity> >        m_lNodes;                       /**< The currently displayed node points as 3D spheres. */

signals:

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool NetworkTreeItem::isDataInit() const
{
    return m_bDataIsInit;
}

} //NAMESPACE DISP3DLIB

#ifndef metatype_networksptr
#define metatype_networksptr
Q_DECLARE_METATYPE(CONNECTIVITYLIB::Network);
#endif

#endif // NETWORKTREEITEM_H
