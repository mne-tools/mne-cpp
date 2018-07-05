//=============================================================================================================
/**
* @file     hpiview.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    HpiView class declaration.
*
*/

#ifndef HPIVIEW_H
#define HPIVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3d_global.h"

#include <fiff/fiff_coord_trans.h>

#include <realtime/rtProcessing/rthpis.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class HpiViewWidget;
}

namespace FIFFLIB {
    class FiffDigPointSet;
    class FiffInfo;
    class FiffDigPoint;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class View3D;
class Data3DTreeModel;
class BemTreeItem;
class DigitizerSetTreeItem;


//=============================================================================================================
/**
* The HpiView class provides a QDialog for the HPI controls.
*
* @brief The HpiView class provides a QDialog for the HPI controls.
*/
class DISP3DSHARED_EXPORT HpiView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a HpiView object.
    *
    * @param[in] pFiffInfo      The FiffInfo.
    * @param[in] parent         The parent widget.
    */
    HpiView(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo, QWidget *parent = 0);
    ~HpiView();

    //=========================================================================================================
    /**
    * Set the data needed for fitting.
    *
    * @param[in] matData  The data matrix
    */
    void setData(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
    * Get GOF per coil in mm.
    *
    * @return   The GOF vector
    */
    QVector<double> getGOF();

    //=========================================================================================================
    /**
    * Returns if last fit was ok.
    *
    * @return   True if last fit was ok.
    */
    bool wasLastFitOk();

protected:
    virtual void closeEvent( QCloseEvent * event );

    //=========================================================================================================
    /**
    * Update the projectors for SSP and Comps.
    */
    void updateProjections();

    //=========================================================================================================
    /**
    * Returns true if any digitizers were loaded that correspond to HPI coils.
    *
    * @return true  If any digitizers were loaded that correspond to HPI coils, false otherwise.
    */
    bool hpiLoaded();

    //=========================================================================================================
    /**
    * Read Polhemus data from fif file.
    */
    QList<FIFFLIB::FiffDigPoint> readPolhemusDig(const QString& fileName);

    void alignFiducials(const QString& fileNameDigData);

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    *
    * @param[in] fitResult  The fit result coming from the rt HPI class.
    */
    void onNewFittingResultAvailable(REALTIMELIB::FittingResult fitResult);

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    */
    void onBtnLoadPolhemusFile();

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    */
    void onFreqsChanged();

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    */
    void onDoContinousHPI();

    //=========================================================================================================
    /**
    * The max distance value for continous HPI fitting changed.
    */
    void onContinousHPIMaxDistChanged();

    //=========================================================================================================
    /**
    * Toggle SSP's and Comp's.
    */
    void onSSPCompUsageChanged();

    //=========================================================================================================
    /**
    * Perform a single HPI fit.
    */
    void onBtnDoSingleFit();

    //=========================================================================================================
    /**
    * Updates the error related labels.
    */
    void updateErrorLabels();

    //=========================================================================================================
    /**
    * Updates the transformation related labels.
    */
    void updateTransLabels();

    //=========================================================================================================
    /**
    * Store the last fit which was ok.
    *
    * @param[in] devHeadTrans   The device to head transformation matrix to be stored.
    * @param[in] fittedCoils    The fitted coils to be stored.
    */
    void storeResults(const FIFFLIB::FiffCoordTrans& devHeadTrans, const FIFFLIB::FiffDigPointSet& fittedCoils);

    //=========================================================================================================
    /**
    * Updates the digitizer and head models in the 3D view based on the current head/device transformation.
    */
    void update3DView();

//    //=========================================================================================================
//    /**
//    * Align the MEG fiducials to the MRI fiducials.
//    */
//    int alignFiducials(digitizerData head_dig,
//                       digitizerData mri_dig);

    Ui::HpiViewWidget*                          ui;                     /**< The HPI dialog. */

    QVector<int>                                m_vCoilFreqs;           /**< Vector contains the HPI coil frequencies. */
    QVector<double>                             m_vGof;                 /**< The goodness of fit in mm for each fitted HPI coil. */

    double                                      m_dMeanErrorDist;       /**< The error distances, averaged over all coil errors. */
    qint16                                      m_iNubmerBadChannels;   /**< The number of bad channels.*/

    bool                                        m_bUseSSP;              /**< Use SSP's.*/
    bool                                        m_bUseComp;             /**< Use Comps's.*/
    bool                                        m_bLastFitGood;         /**< Flag specifying if last fit was ok or not.*/

    Eigen::SparseMatrix<double>                 m_sparseMatCals;        /**< Sparse calibration matrix.*/
    Eigen::MatrixXd                             m_matValue;             /**< The current data block.*/
    Eigen::MatrixXd                             m_matProjectors;        /**< Holds the matrix with the SSP and compensator projectors.*/
    Eigen::MatrixXd                             m_matCompProjectors;    /**< Holds the matrix with the SSP and compensator projectors.*/

    QSharedPointer<DISP3DLIB::View3D>           m_pView3D;              /**< The 3D view. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>  m_pData3DModel;         /**< The Disp3D model. */
    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;            /**< The FiffInfo. */
    QSharedPointer<REALTIMELIB::RtHPIS>         m_pRtHPI;               /**< The real-time HPI object. */

    QPointer<DISP3DLIB::BemTreeItem>            m_pBemHeadKid;          /**< The BEM head model for a kid. */
    QPointer<DISP3DLIB::BemTreeItem>            m_pBemHeadAdult;        /**< The BEM head model for an adult. */

    QPointer<DISP3DLIB::DigitizerSetTreeItem>   m_pTrackedDigitizer;    /**< The 3D item pointing to the tracked digitizers. */

    double                                      m_dMaxHPIFitError;      /**< The maximum HPI fitting error allowed.*/

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user toggled the do HPI check box.
    *
    * @param[in] state    Whether to do continous HPI.
    */
    void continousHPIToggled(bool state);
};

} //NAMESPACE
#endif // HPIVIEW_H
