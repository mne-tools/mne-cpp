//=============================================================================================================
/**
* @file     sensorpositiontreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lroenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     SensorPositionTreeItem class declaration.
*
*/

#ifndef SENSORPOSITIONTREEITEM_H
#define SENSORPOSITIONTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"


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

namespace FIFFLIB {
    class FiffChInfo;
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


//=============================================================================================================
/**
* SensorPositionTreeItem provides a tree item to visualize sensor position data.
*
* @brief SensorPositionTreeItem provides a tree item to visualize sensor position data.
*/
class DISP3DSHARED_EXPORT SensorPositionTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SensorPositionTreeItem> SPtr;             /**< Shared pointer type for SensorPositionTreeItem class. */
    typedef QSharedPointer<const SensorPositionTreeItem> ConstSPtr;  /**< Const shared pointer type for SensorPositionTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] p3DEntityParent    The parent 3D entity.
    * @param[in] iType              The type of the item. See types.h for declaration and definition.
    * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit SensorPositionTreeItem(Qt3DCore::QEntity* p3DEntityParent = 0,
                                    int iType = Data3DTreeModelItemTypes::SensorPositionItem,
                                    const QString& text = "Sensor Position");

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surface and annotation data to this item.
    *
    * @param[in] lChInfo            The channel information used to plot the MEG channels.
    * @param[in] sDataType          The data type: EEG, MEG.
    */
    void addData(const QList<FIFFLIB::FiffChInfo> &lChInfo, const QString &sDataType);

protected:
    //=========================================================================================================
    /**
    * Plots the sensors.
    *
    * @param[in] lChInfo            The channel information used to plot the MEG channels.
    * @param[in] sDataType          The data type: EEG, MEG.
    */
    void plotSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo, const QString &sDataType);

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();
};

} //NAMESPACE DISP3DLIB

#endif // SENSORPOSITIONTREEITEM_H
