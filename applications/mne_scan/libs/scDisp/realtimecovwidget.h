//=============================================================================================================
/**
* @file     realtimecovwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeCovWidget Class.
*
*/

#ifndef REALTIMECOVWIDGET_H
#define REALTIMECOVWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;
class QVBoxLayout;
class QLabel;

namespace FIFFLIB {
    class FiffInfo;
}

namespace SCMEASLIB {
    class RealTimeCov;
}

namespace DISPLIB {
    class ModalitySelectionView;
    class ImageSc;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS RealTimeCovWidget
*
* @brief The RealTimeCovWidget class provides a real-time covariance display.
*/
class SCDISPSHARED_EXPORT RealTimeCovWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeCovWidget which is a child of parent.
    *
    * @param [in] pRTC          pointer to real-time evoked measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeCovWidget(QSharedPointer<SCMEASLIB::RealTimeCov> pRTC,
                      QSharedPointer<QTime> &pTime,
                      QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeCovWidget.
    */
    ~RealTimeCovWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Is called when new data are available.
    */
    virtual void getData();

    //=========================================================================================================
    /**
    * Initialise the RealTimeCovWidget.
    */
    virtual void init();

protected:
    //=========================================================================================================
    /**
    * Show modality view.
    */
    void showModalitySelectionWidget();

    //=========================================================================================================
    /**
    * Show modality view.
    */
    void onNewModalitySelection(const QMap<QString, bool>& modalityMap);

    QSharedPointer<DISPLIB::ModalitySelectionView>   m_pModalitySelectionWidget;    /**< Modality selection widget */

    QSharedPointer<SCMEASLIB::RealTimeCov>  m_pRTC;                                 /**< The real-time covariance measurement. */

    QPointer<QAction>                       m_pActionSelectModality;                /**< Modality selection action */
    QPointer<QVBoxLayout>                   m_pRtcLayout;                           /**< Widget layout */
    QPointer<QLabel>                        m_pLabelInit;                           /**< Initialization label */

    bool                                    m_bInitialized;                         /**< Is Initialized */

    QSharedPointer<FIFFLIB::FiffInfo>       m_pFiffInfo;                            /**< The Fiff Info. */

    QMap<QString, bool>                     m_modalityMap;                          /**< Map of different modalities. */

    QPointer<DISPLIB::ImageSc>              m_pImageSc;                             /**< The covariance colormap */

    QList<qint32>                           m_qListSelChannel;                      /**< The channel list generated from the selected modalities */
};

} // NAMESPACE

#endif // REALTIMECOVWIDGET_H
