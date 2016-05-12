//=============================================================================================================
/**
* @file     brainrtsourcelocdatatreeitem.h
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
* @brief     BrainRTSourceLocDataTreeItem class declaration.
*
*/

#ifndef BRAINRTSOURCELOCDATATREEITEM_H
#define BRAINRTSOURCELOCDATATREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "../../helpers/abstracttreeitem.h"
#include "../../helpers/types.h"
#include "../../rt/rtSourceLoc/rtsourcelocdataworker.h"

#include "braintreemetaitem.h"

#include "fiff/fiff_types.h"

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
* BrainRTSourceLocDataTreeItem provides a generic item to hold information about real time source localization data to plot onto the brain surface.
*
* @brief Provides a generic brain tree item to hold real time data.
*/
class DISP3DNEWSHARED_EXPORT BrainRTSourceLocDataTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<BrainRTSourceLocDataTreeItem> SPtr;             /**< Shared pointer type for BrainRTSourceLocDataTreeItem class. */
    typedef QSharedPointer<const BrainRTSourceLocDataTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainRTSourceLocDataTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit BrainRTSourceLocDataTreeItem(int iType = BrainTreeModelItemTypes::RTSourceLocDataItem, const QString& text = "RT Source Loc Data");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainRTSourceLocDataTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Initializes the rt data item with neccessary information for visualization computations.
    *
    * @param[in] tForwardSolution       The MNEForwardSolution.
    * @param[in] hemi                   The hemispehre of this brain rt data item. This information is important in order to cut out the wanted source estimations from the MNESourceEstimate
    * @param[in] arraySurfaceVertColor  The vertex colors for the surface where the data is to be plotted on.
    * @param[in] vecLabelIds            The label ids for each surface vertex index.
    * @param[in] lLabels                The label list.
    *
    * @return                           Returns true if successful.
    */
    bool init(const MNELIB::MNEForwardSolution& tForwardSolution, const QByteArray &arraySurfaceVertColor, int iHemi, const Eigen::VectorXi& vecLabelIds = FIFFLIB::defaultVectorXi, const QList<FSLIB::Label>& lLabels = QList<FSLIB::Label>());

    //=========================================================================================================
    /**
    * Adds actual rt data which is streamed by this item's worker thread item. In order for this function to worker, you must call init(...) beforehand.
    *
    * @param[in] tSourceEstimate    The MNESourceEstimate data.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const MNELIB::MNESourceEstimate& tSourceEstimate);

    //=========================================================================================================
    /**
    * Updates the rt data which is streamed by this item's worker thread item.
    *
    * @return                       Returns true if this item is initialized.
    */
    inline bool isInit() const;

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
    void setStreamingActive(bool state);

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
    * This function sets the colortable type.
    *
    * @param[in] sColortable     The new colortable ("Hot Negative 1" etc.).
    */
    void setColortable(const QString& sColortable);

    //=========================================================================================================
    /**
    * This function sets the visualization type.
    *
    * @param[in] sVisualizationType     The new visualization type ("Annotation based" etc.).
    */
    void setVisualizationType(const QString& sVisualizationType);

    //=========================================================================================================
    /**
    * This function set the normalization value.
    *
    * @param[in] dNormalization     The new normalization value.
    */
    void setNormalization(double dNormalization);

public slots:
    //=========================================================================================================
    /**
    * This slot gets called whenever the origin of the surface vertex color (curvature, annoation, etc.) changed.
    *
    * @param[in] arrayVertColor     The new vertex colors.
    */
    void onColorInfoOriginChanged(const QByteArray& arrayVertColor);

private slots:
    //=========================================================================================================
    /**
    * This slot gets called whenever the check/actiation state of the rt data worker changed.
    *
    * @param[in] checkState     The check state of the worker.
    */
    void onCheckStateWorkerChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * This slot gets called whenever this item receives new color values for each estimated source.
    *
    * @param[in] sourceColorSamples     The color values for each estimated source.
    */
    void onNewRtData(const QByteArray& sourceColorSamples);

    //=========================================================================================================
    /**
    * This slot gets called whenever the used colormap type changed.
    *
    * @param[in] sColormapType     The name of the new colormap type.
    */
    void onColormapTypeChanged(const QString& sColormapType);

    //=========================================================================================================
    /**
    * This slot gets called whenever the time interval in between the streamed samples changed.
    *
    * @param[in] iMSec     The new time in milliseconds waited in between each streamed sample.
    */
    void onTimeIntervalChanged(int iMSec);

    //=========================================================================================================
    /**
    * This slot gets called whenever the normaization value changed. The normalization value is used to normalize the estimated source activation.
    *
    * @param[in] iMSec     The new time normalization value.
    */
    void onDataNormalizationValueChanged(double dValue);

    //=========================================================================================================
    /**
    * This slot gets called whenever the preferred visualization type changes (single vertex, smoothing, annotation based). This functions translates from QString to m_iVisualizationType.
    *
    * @param[in] sVisType     The new visualization type.
    */
    void onVisualizationTypeChanged(const QString& sVisType);

    //=========================================================================================================
    /**
    * This slot gets called whenever the check/actiation state of the looped streaming state changed.
    *
    * @param[in] checkState     The check state of the looped streaming state.
    */
    void onCheckStateLoopedStateChanged(const Qt::CheckState& checkState);

    //=========================================================================================================
    /**
    * This slot gets called whenever the number of averages of the streamed samples changed.
    *
    * @param[in] iNumAvr     The new number of averages.
    */
    void onNumberAveragesChanged(int iNumAvr);

private:
    bool                        m_bIsInit;                      /**< The init flag. */

    RtSourceLocDataWorker*      m_pSourceLocRtDataWorker;       /**< The source data worker. This worker streams the rt data to this item.*/

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever you want to provide newly generated colors from the stream rt data.
    *
    * @param[in] sourceColorSamples     The color values for each estimated source.
    */
    void rtVertColorChanged(const QByteArray& sourceColorSamples);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool BrainRTSourceLocDataTreeItem::isInit() const
{
    return m_bIsInit;
}

} //NAMESPACE DISP3DLIB

#endif // BRAINRTSOURCELOCDATATREEITEM_H
