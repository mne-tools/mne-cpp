 //=============================================================================================================
/**
 * @file     mnedatatreeitem.h
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief     MneDataTreeItem class declaration.
 *
 */

#ifndef DISP3DLIB_MNEDATATREEITEM_H
#define DISP3DLIB_MNEDATATREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstracttreeitem.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <Qt3DCore/QTransform>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEForwardSolution;
    class MNESourceEstimate;
}

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
}

namespace Qt3DCore {
    class QEntity;
    class QTransform;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class RtSourceDataController;
class AbstractMeshTreeItem;
class GpuInterpolationItem;

//=============================================================================================================
/**
 * MneDataTreeItem provides a generic item to hold information about real time source localization data to plot onto the brain surface.
 *
 * @brief Provides a generic brain tree item to hold real time data.
 */
class DISP3DSHARED_EXPORT MneDataTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<MneDataTreeItem> SPtr;             /**< Shared pointer type for MneDataTreeItem class. */
    typedef QSharedPointer<const MneDataTreeItem> ConstSPtr;  /**< Const shared pointer type for MneDataTreeItem class. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] iType      The type of the item. See types.h for declaration and definition.
     * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
     * @param[in] bUseGPU    Whether to use the GPU to visualize the data.
     */
    explicit MneDataTreeItem(int iType = Data3DTreeModelItemTypes::MNEDataItem,
                             const QString& text = "MNE",
                             bool bUseGPU = false);

    //=========================================================================================================
    /**
     * Default destructor
     */
    ~MneDataTreeItem();

    //=========================================================================================================
    /**
     * Initializes the rt data item with neccessary information for visualization computations.
     *
     * @param[in] tForwardSolution       The MNEForwardSolution.
     * @param[in] tSurfSet               The surface set holding the left and right hemisphere surfaces.
     * @param[in] tAnnotSet              The annotation set holding the left and right hemisphere annotations.
     * @param[in] p3DEntityParent        Pointer to the QEntity parent.
     */
    void initData(const MNELIB::MNEForwardSolution& tForwardSolution,
                  const FSLIB::SurfaceSet& tSurfSet,
                  const FSLIB::AnnotationSet& tAnnotSet,
                  Qt3DCore::QEntity* p3DEntityParent);

    //=========================================================================================================
    /**
     * Adds actual rt data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
     *
     * @param[in] tSourceEstimate    The MNESourceEstimate data.
     */
    void addData(const MNELIB::MNESourceEstimate& tSourceEstimate);

    //=========================================================================================================
    /**
     * Updates the rt data which is streamed by this item's worker thread item.
     *
     * @return                       Returns true if this item is initialized.
     */
    inline bool isDataInit() const;

    //=========================================================================================================
    /**
     * This function sets the loop flag.
     *
     * @param[in] state     Whether to loop the data or not.
     */
    void setLoopState(bool state);

    //=========================================================================================================
    /**
     * This function sets the data streaming.
     *
     * @param[in] state     Whether to stream the data to the display or not.
     */
    void setStreamingState(bool state);

    //=========================================================================================================
    /**
     * This function sets the time interval for streaming.
     *
     * @param[in] iMSec     The waiting time inbetween samples.
     */
    void setTimeInterval(int iMSec);

    //=========================================================================================================
    /**
     * This function sets the number of averages.
     *
     * @param[in] iNumberAverages     The new number of averages.
     */
    void setNumberAverages(int iNumberAverages);

    //=========================================================================================================
    /**
     * This function sets the color map type.
     *
     * @param[in] sColortable     The new colortable ("Hot Negative 1" etc.).
     */
    void setColormapType(const QString& sColormap);

    //=========================================================================================================
    /**
     * This function sets the visualization type.
     *
     * @param[in] sVisualizationType     The new visualization type ("Annotation based" etc.).
     */
    void setVisualizationType(const QString& sVisualizationType);

    //=========================================================================================================
    /**
     * This function set the threshold values.
     *
     * @param[in] vecThresholds     The new threshold values used for normalizing the data.
     */
    void setThresholds(const QVector3D& vecThresholds);

    //=========================================================================================================
    /**
     * This function sets the cancel distance used in distance calculations for the interpolation.
     * Distances higher than this are ignored, i.e. the respective coefficients are set to zero.
     *
     * @param[in] dCancelDist               The new cancel distance value in meters.
     */
    //virtual void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
     * This function sets the function that is used in the interpolation process.
     *
     * @param[in] sInterpolationFunction         Function that computes interpolation coefficients using the distance values.
     */
    virtual void setInterpolationFunction(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Set the sampling frequency.
     *
     * @param[in] dSFreq                 The new sampling frequency.
     */
    void setSFreq(const double dSFreq);

    //=========================================================================================================
    /**
     * Set the alpha value.
     *
     * @param[in] fAlpha    The new alpha value.
     */
    void setAlpha(float fAlpha);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void setTransform(const Qt3DCore::QTransform &transform);

    //=========================================================================================================
    /**
     * Sets the entity's transformation. This will clear the old transformation.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void setTransform(const FIFFLIB::FiffCoordTrans& transform, bool bApplyInverse = false);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     */
    virtual void applyTransform(const Qt3DCore::QTransform& transform);

    //=========================================================================================================
    /**
     * Applies a transformation o ntop of the present one.
     *
     * @param[in] transform     The new entity's transform.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    virtual void applyTransform(const FIFFLIB::FiffCoordTrans& transform, bool bApplyInverse = false);

protected:
    //=========================================================================================================
    /**
     * AbstractTreeItem functions
     */
    void initItem();

    //=========================================================================================================
    /**
     * This function gets called whenever the check/actiation state of the rt data worker changed.
     *
     * @param[in] checkState     The check state of the worker.
     */
    void onCheckStateWorkerChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
     * Set the new interpolation matrix.
     *
     * @param[in] pMatInterpolationMatrixLeftHemi          The new interpolation matrix for the left hemisphere.
     */
    virtual void onNewInterpolationMatrixLeftAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeftHemi);

    //=========================================================================================================
    /**
     * Set the new interpolation matrix.
     *
     * @param[in] pMatInterpolationMatrixRightHemi          The new interpolation matrix for the right hemisphere.
     */
    virtual void onNewInterpolationMatrixRightAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRightHemi);

    //=========================================================================================================
    /**
     * This function gets called whenever this item receives sensor values for each estimated source.
     *
     * @param[in] vecDataVectorLeftHemi          The new streamed raw data for the left hemispehre.
     * @param[in] vecDataVectorRightHemi         The new streamed raw data for the right hemispehre.
     */
    void virtual onNewRtRawData(const Eigen::VectorXd &vecDataVectorLeftHemi,
                                const Eigen::VectorXd &vecDataVectorRightHemi);

    //=========================================================================================================
    /**
     * This function gets called whenever this item receives new color values for each estimated source.
     *
     * @param[in] matColorMatrixLeftHemi          The new streamed interpolated raw data in form of RGB colors per vertex for the left hemisphere.
     * @param[in] matColorMatrixRightHemi         The new streamed interpolated raw data in form of RGB colors per vertex for the right hemisphere.
     */
    void onNewRtSmoothedDataAvailable(const Eigen::MatrixX4f &matColorMatrixLeftHemi,
                                      const Eigen::MatrixX4f &matColorMatrixRightHemi);

    //=========================================================================================================
    /**
     * This function gets called whenever the used colormap type changed.
     *
     * @param[in] sColormapType     The name of the new colormap type.
     */
    void onColormapTypeChanged(const QVariant& sColormapType);

    //=========================================================================================================
    /**
     * This function gets called whenever the time interval in between the streamed samples changed.
     *
     * @param[in] iMSec     The new time in milliseconds waited in between each streamed sample.
     */
    void onTimeIntervalChanged(const QVariant &iMSec);

    //=========================================================================================================
    /**
     * This function gets called whenever the normaization value changed. The normalization value is used to normalize the estimated source activation.
     *
     * @param[in] vecThresholds     The new threshold values used for normalizing the data.
     */
    void onDataThresholdChanged(const QVariant &vecThresholds);

    //=========================================================================================================
    /**
     * This function gets called whenever the preferred visualization type changes (single vertex, smoothing, annotation based). This functions translates from QString to m_iVisualizationType.
     *
     * @param[in] sVisType     The new visualization type.
     */
    void onVisualizationTypeChanged(const QVariant& sVisType);

    //=========================================================================================================
    /**
     * This function gets called whenever the check/actiation state of the looped streaming state changed.
     *
     * @param[in] checkState     The check state of the looped streaming state.
     */
    void onCheckStateLoopedStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
     * This function gets called whenever the number of averages of the streamed samples changed.
     *
     * @param[in] iNumAvr     The new number of averages.
     */
    void onNumberAveragesChanged(const QVariant& iNumAvr);

    //=========================================================================================================
    /**
     * This function gets called whenever the cancel distance of the interpolation changed.
     *
     * @param[in] dCancelDist     The new cancel distance.
     */
    //virtual void onCancelDistanceChanged(const QVariant& dCancelDist);

    //=========================================================================================================
    /**
     * This function gets called whenever the function of the interpolation changed.
     *
     * @param[in] sInterpolationFunction     The new function name.
     */
    virtual void onInterpolationFunctionChanged(const QVariant& sInterpolationFunction);

    bool                                m_bIsDataInit;                      /**< The init flag. */
    bool                                m_bUseGPU;                          /**< The use GPU flag. */

    QPointer<RtSourceDataController>    m_pRtSourceDataController;          /**< The source data worker. This worker streams the rt data to this item.*/

    QPointer<AbstractMeshTreeItem>      m_pInterpolationItemLeftCPU;        /**< This item manages all 3d rendering and calculations for the left hemisphere. */
    QPointer<GpuInterpolationItem>      m_pInterpolationItemLeftGPU;        /**< This item manages all 3d rendering and calculations for the left hemisphere. */
    QPointer<AbstractMeshTreeItem>      m_pInterpolationItemRightCPU;       /**< This item manages all 3d rendering and calculations for the right hemisphere. */
    QPointer<GpuInterpolationItem>      m_pInterpolationItemRightGPU;       /**< This item manages all 3d rendering and calculations for the right hemisphere. */

signals:
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MneDataTreeItem::isDataInit() const
{
    return m_bIsDataInit;
}
} //NAMESPACE DISP3DLIB

#endif // DISP3DLIB_MNEESTIMATTREEITEM_H
