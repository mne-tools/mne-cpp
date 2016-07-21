//=============================================================================================================
/**
* @file     digitizersettreeitem.h
* @author   Your name <yourname@yourdomain>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief     DigitizerSetTreeItem class declaration.
*
*/

#ifndef DIGITIZERSETTREEITEM
#define DIGITIZERSETTREEITEM

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "../../helpers/abstracttreeitem.h"

#include "../../helpers/types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//Put all Qt includes here


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//Put all Eigen includes here


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffDigPoint;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//PLEAS DO NOT USE ""using namespace"" IN HEADER FILES

//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//Put all used forward declarations of the DISP3DLIB namespace here


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/
class DISP3DNEWSHARED_EXPORT DigitizerSetTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<DigitizerSetTreeItem> SPtr;            /**< Shared pointer type for DigitizerSetTreeItem. */
    typedef QSharedPointer<const DigitizerSetTreeItem> ConstSPtr; /**< Const shared pointer type for DigitizerSetTreeItem. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit DigitizerSetTreeItem(int iType = Data3DTreeModelItemTypes::SourceSpaceItem, const QString& text = "Source space");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~DigitizerSetTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds digitizer data to this item.
    *
    * @param[in] tDigitizer         The digitizer data.
    * @param[in] parent             The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer, Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
    * Call this function whenever you want to change the visibilty of the 3D rendered content.
    *
    * @param[in] state     The visiblity flag.
    */
    void setVisible(bool state);

private:
    //=========================================================================================================
    /**
    * Call this function whenever the surface color was changed.
    *
    * @param[in] color        The new surface color.
    */
    void onSurfaceColorChanged(const QColor &color);

    //=========================================================================================================
    /**
    * Call this function whenever the check box of this item was checked.
    *
    * @param[in] checkState        The current checkstate.
    */
    virtual void onCheckStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * Creates a QByteArray of colors for given color for the input vertices.
    *
    * @param[in] vertices       The vertices information.
    * @param[in] color          The vertex color information.
    */
    QByteArray createVertColor(const Eigen::MatrixXf& vertices, const QColor& color = QColor(100,100,100)) const;

    Qt3DCore::QEntity*      m_pParentEntity;                            /**< The parent 3D entity. */
    Renderable3DEntity*     m_pRenderable3DEntity;                      /**< The renderable 3D entity. */

    QObjectList             m_lChildren;

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the origin of the vertex color (from curvature, from annotation) changed.
    *
    * @param[in] arrayVertColor      The new vertex colors.
    */
    void colorInfoOriginChanged(const QByteArray& arrayVertColor);

};

} // NAMESPACE DISP3DLIB

#endif // DIGITIZERSETTREEITEM
