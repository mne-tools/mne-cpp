//=============================================================================================================
/**
 * @file     digitizertreeitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief     DigitizerTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_DIGITIZERTREEITEM_H
#define DISP3DLIB_DIGITIZERTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffDigPoint;
}

namespace Qt3DCore {
    class QEntity;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class GeometryMultiplier;

//=============================================================================================================
/**
 * DigitizerTreeItem provides a generic tree item to hold and visualize digitizer data.
 *
 * @brief DigitizerTreeItem provides a generic tree item to hold and visualize digitizer data.
 */
class DISP3DSHARED_EXPORT DigitizerTreeItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<DigitizerTreeItem> SPtr;             /**< Shared pointer type for DigitizerTreeItem class. */
    typedef QSharedPointer<const DigitizerTreeItem> ConstSPtr;  /**< Const shared pointer type for DigitizerTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType              The type of the item. See types.h for declaration and definition.
     * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit DigitizerTreeItem(Qt3DCore::QEntity* p3DEntityParent = 0,
                               int iType = Data3DTreeModelItemTypes::DigitizerItem,
                               const QString& text = "Digitizer");

    //=========================================================================================================
    /**
     * Adds FreeSurfer data based on surface and annotation data to this item.
     *
     * @param[in] tDigitizer         The digitizer data.
     * @param[in] tSphereRadius      The radius of the visualized digitizer sphere.
     * @param[in] tSphereColor       The color of the visualized digitizer.
     */
    void addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer,
                                    const float tSphereRadius,
                                    const QColor &tSphereColor);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    QPointer<GeometryMultiplier>    m_pSphereMesh;
};
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_DIGITIZERTREEITEM_H
