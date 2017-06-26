//=============================================================================================================
/**
* @file     subjecttreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
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
* @brief     SubjectTreeItem class declaration.
*
*/

#ifndef SUBJECTTREEITEM_H
#define SUBJECTTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"
#include "../measurement/measurementtreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


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

class MeasurementTreeItem;


//=============================================================================================================
/**
* SubjectTreeItem provides a generic tree item to hold subject data (brain surface, annotation, BEM) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic SubjectTreeItem.
*/
class DISP3DNEWSHARED_EXPORT SubjectTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SubjectTreeItem> SPtr;             /**< Shared pointer type for SubjectTreeItem class. */
    typedef QSharedPointer<const SubjectTreeItem> ConstSPtr;  /**< Const shared pointer type for SubjectTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit SubjectTreeItem(int iType = Data3DTreeModelItemTypes::SubjectItem, const QString& text = "");

    //=========================================================================================================
    /**
    * Connects measurement items and their data (i.e. MNE source data) to already loaded MRI data
    *
    * @param[in] pMeasurementItem       The measurement item which is to be connected.
    */
    void connectMeasurementToMriItems(MeasurementTreeItem* pMeasurementItem);

    //=========================================================================================================
    /**
    * Connects measurement items and their data (i.e. MNE source data) to already loaded head BEM data
    *
    * @param[in] pMeasurementItem       The measurement item which is to be connected.
    */
    void connectEEGMeasurementToBemHeadItems(MeasurementTreeItem* pMeasurementItem);

    //=========================================================================================================
    /**
    * Connects measurement items and their data (i.e. MNE source data) to already loaded sensor BEM data
    *
    * @param[in] pMeasurementItem       The measurement item which is to be connected.
    */
    void connectMEGMeasurementToSensorItems(MeasurementTreeItem* pMeasurementItem);

protected:
    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();

};

} //NAMESPACE DISP3DLIB

#endif // SUBJECTTREEITEM_H
