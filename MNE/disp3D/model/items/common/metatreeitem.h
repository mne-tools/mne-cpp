//=============================================================================================================
/**
* @file     metatreeitem.h
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
* @brief     MetaTreeItem class declaration.
*
*/

#ifndef METATREEITEM_H
#define METATREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../disp3D_global.h"
#include "abstracttreeitem.h"


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


//=============================================================================================================
/**
* MetaTreeItem provides a generic brain tree item to hold meta information about other brain tree items.
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT MetaTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<MetaTreeItem> SPtr;             /**< Shared pointer type for MetaTreeItem class. */
    typedef QSharedPointer<const MetaTreeItem> ConstSPtr;  /**< Const shared pointer type for MetaTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit MetaTreeItem(int iType = MetaTreeItemTypes::UnknownItem, const QString& text = "");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~MetaTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the color of the curvature data changed.
    */
    void curvColorsChanged();

    //=========================================================================================================
    /**
    * Emit this signal whenever the time interval of the data streaming changed.
    *
    * @param[in] iMSec     The time interval in mSecs.
    */
    void rtDataTimeIntervalChanged(int iMSec);

    //=========================================================================================================
    /**
    * Emit this signal whenever the normalization value of the data streaming changed.
    *
    * @param[in] vecThresholds     The new threshold values used for normalizing the data.
    */
    void rtDataNormalizationValueChanged(const QVector3D& vecThresholds);

    //=========================================================================================================
    /**
    * Emit this signal whenever the colormap type of the data streaming changed.
    *
    * @param[in] sColormapType     The new colormap type.
    */
    void rtDataColormapTypeChanged(const QString& sColormapType);

    //=========================================================================================================
    /**
    * Emit this signal whenever the visualization type of the data streaming changed (single vertex, smoothing, annotation based).
    *
    * @param[in] sVisualizationType     The new visualization type.
    */
    void rtDataVisualizationTypeChanged(const QString& sVisualizationType);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface color changed.
    *
    * @param[in] color     The new surface color.
    */
    void surfaceColorChanged(const QColor& color);

    //=========================================================================================================
    /**
    * Emit this signal whenever the number of averages of the data streaming changed.
    *
    * @param[in] iMSec     The new number of averages.
    */
    void rtDataNumberAveragesChanged(int iNumAvr);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface alpha value has changed.
    *
    * @param[in] fAlpha     The new alpha value.
    */
    void surfaceAlphaChanged(float fAlpha);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface inner tesselation value has changed.
    *
    * @param[in] fTessInner     The new inner tesselation value.
    */
    void surfaceTessInnerChanged(float fTessInner);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface outer tesselation value has changed.
    *
    * @param[in] fTessOuter     The new outer tesselation value.
    */
    void surfaceTessOuterChanged(float fTessOuter);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface triangle scale value has changed.
    *
    * @param[in] fTriangleScale     The triangle scale value.
    */
    void surfaceTriangleScaleChanged(float fTriangleScale);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface translation x value changed has changed.
    *
    * @param[in] xTrans     The new translation x value.
    */
    void surfaceTranslationXChanged(float xTrans);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface translation y value changed has changed.
    *
    * @param[in] yTrans     The new translation y value.
    */
    void surfaceTranslationYChanged(float yTrans);

    //=========================================================================================================
    /**
    * Emit this signal whenever the surface translation z value changed has changed.
    *
    * @param[in] zTrans     The new translation z value.
    */
    void surfaceTranslationZChanged(float zTrans);

    //=========================================================================================================
    /**
    * Emit this signal whenever the network threshold changed.
    *
    * @param[in] vecThresholds     The new threshold values used for thresholding the data.
    */
    void networkThresholdChanged(const QVector3D& vecThresholds);
};

} //NAMESPACE DISP3DLIB

#endif // METATREEITEM_H
