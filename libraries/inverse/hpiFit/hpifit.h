//=============================================================================================================
/**
 * @file     hpifit.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    HPIFit class declaration.
 *
 */

#ifndef HPIFIT_H
#define HPIFIT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "fiff/fiff_ch_info.h"

#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FWDLIB{
    class FwdCoil;
    class FwdCoilSet;
}

namespace FIFFLIB{
    class FiffInfo;
    class FiffCoordTrans;
    class FiffDigPointSet;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

/**
 * The strucut specifing the coil parameters.
 */
struct CoilParam {
    Eigen::MatrixXd pos;
    Eigen::MatrixXd mom;
    Eigen::VectorXd dpfiterror;
    Eigen::VectorXd dpfitnumitr;
};

/**
 * The struct specifing all data needed to perform coil-wise fitting.
 */
struct HpiFitResult {
    FIFFLIB::FiffDigPointSet    fittedCoils;
    FIFFLIB::FiffCoordTrans     devHeadTrans;
    QVector<double>             errorDistances;
    Eigen::VectorXd             GoF;
    QString                     sFilePathDigitzers;
    bool                        bIsLargeHeadMovement;
    float                       fHeadMovementDistance;
    float                       fHeadMovementAngle;
};

/**
 * The strucut specifing the sensor parameters.
 */
struct SensorSet {
    Eigen::MatrixXd r0;
    Eigen::MatrixXd rmag;
    Eigen::MatrixXd cosmag;
    Eigen::MatrixXd tra;
    Eigen::RowVectorXd w;
    int ncoils;
    int np;
};

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * HPI Fit algorithms.
 *
 * @brief HPI Fit algorithms.
 */
class INVERSESHARED_EXPORT HPIFit
{

public:
    typedef QSharedPointer<HPIFit> SPtr;             /**< Shared pointer type for HPIFit. */
    typedef QSharedPointer<const HPIFit> ConstSPtr;  /**< Const shared pointer type for HPIFit. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] pFiffInfo        Associated Fiff Information.
     * @param[in] bDoFastFit       Do the fast fit by fitting to the more basic Model.
     */
    explicit HPIFit(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                    bool bDoFastFit = false);

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in]   t_mat                      Data to estimate the HPI positions from.
     * @param[in]   t_matProjectors            The projectors to apply. Bad channels are still included.
     * @param[out]   transDevHead               The final dev head transformation matrix.
     * @param[in]   vecFreqs                   The frequencies for each coil.
     * @param[out]   vecError                   The HPI estimation Error in mm for each fitted HPI coil.
     * @param[out]   vecGoF                     The goodness of fit for each fitted HPI coil.
     * @param[out]   fittedPointSet             The final fitted positions in form of a digitizer set.
     * @param[in]   pFiffInfo                  Associated Fiff Information.
     * @param[in]   bDoDebug                   Print debug info to cmd line and write debug info to file.
     * @param[in]   sHPIResourceDir            The path to the debug file which is to be written.
     * @param[in]   iMaxIterations             The maximum allowed number of iterations used to fit the dipoles. Default is 500.
     * @param[in]   fAbortError                The error which will lead to aborting the dipole fitting process. Default is 1e-9.
     */
    void fitHPI(const Eigen::MatrixXd& t_mat,
                const Eigen::MatrixXd& t_matProjectors,
                FIFFLIB::FiffCoordTrans &transDevHead,
                const QVector<int>& vecFreqs,
                QVector<double>& vecError,
                Eigen::VectorXd& vecGoF,
                FIFFLIB::FiffDigPointSet& fittedPointSet,
                QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                bool bDoDebug = false,
                const QString& sHPIResourceDir = QString("./HPIFittingDebug"),
                int iMaxIterations = 500,
                float fAbortError = 1e-9);

    //=========================================================================================================
    /**
     * assign frequencies to correct position
     *
     * @param[in]   t_mat              Data to estimate the HPI positions from.
     * @param[in]   t_matProjectors    The projectors to apply. Bad channels are still included.
     * @param[in]   transDevHead       The final dev head transformation matrix.
     * @param[in]   vecFreqs           The frequencies for each coil in unknown order.
     * @param[out]   vecFreqs           The frequencies for each coil in correct order.
     * @param[in]   vecError           The HPI estimation Error in mm for each fitted HPI coil.
     * @param[in]   vecGoF             The goodness of fit for each fitted HPI coil.
     * @param[in]   fittedPointSet     The final fitted positions in form of a digitizer set.
     * @param[in]   pFiffInfo          Associated Fiff Information.
     */
    void findOrder(const Eigen::MatrixXd& t_mat,
                   const Eigen::MatrixXd& t_matProjectors,
                   FIFFLIB::FiffCoordTrans &transDevHead,
                   QVector<int>& vecFreqs,
                   QVector<double> &vecError,
                   Eigen::VectorXd& vecGoF,
                   FIFFLIB::FiffDigPointSet& fittedPointSet,
                   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Store results from dev_Head_t as quaternions in position matrix. The format is the same as you
     * get from Neuromag's MaxFilter.
     *
     *
     * @param[in]  fTime           The corresponding time in the measurement for the fit.
     * @param[in]  pFiffInfo       The FiffInfo file from the measurement.
     * @param[out]  matPosition     The matrix to store the results.
     * @param[in]  vecGoF          The goodness of fit for each coil.
     * @param[in]  vecError        The Hpi estimation Error per coil.
     *
     * ToDo: get estimated movement velocity and store it in channel 9
     */
    static void storeHeadPosition(float fTime,
                                  const Eigen::MatrixXf& transDevHead,
                                  Eigen::MatrixXd& matPosition,
                                  const Eigen::VectorXd& vecGoF,
                                  const QVector<double>& vecError);
protected:
    //=========================================================================================================
    /**
     * Fits dipoles for the given coils and a given data set.
     *
     * @param[in] CoilParam         The coil parameters.
     * @param[in] sensors           The sensor information.
     * @param[in] matData           The data which used to fit the coils.
     * @param[in] iNumCoils         The number of coils.
     * @param[in] t_matProjectors   The projectors to apply. Bad channels are still included.
     * @param[in] iMaxIterations    The maximum allowed number of iterations used to fit the dipoles. Default is 500.
     * @param[in] fAbortError       The error which will lead to aborting the dipole fitting process. Default is 1e-9.
     *
     * @return Returns the coil parameters.
     */
    CoilParam dipfit(struct CoilParam coil,
                     const SensorSet& sensors,
                     const Eigen::MatrixXd &matData,
                     int iNumCoils,
                     const Eigen::MatrixXd &t_matProjectors,
                     int iMaxIterations,
                     float fAbortError);

    //=========================================================================================================
    /**
     * Computes the transformation matrix between two sets of 3D points.
     *
     * @param[in] matNH    The first set of input 3D points (row-wise order).
     * @param[in] matBT    The second set of input 3D points (row-wise order).
     *
     * @return Returns the transformation matrix.
     */
    Eigen::Matrix4d computeTransformation(Eigen::MatrixXd matNH,
                                          Eigen::MatrixXd matBT);

    //=========================================================================================================
    /**
     * Read from FwdCoilSet and store into sensors struct.
     * Can be deleted as soon as FwdCoilSet is refactored to QList and EigenMatrix.
     *
     * @param[in] sensors       The struct to save sensor information.
     * @param[in] coils         The coilset to read the sensor information from.
     *
     */
    void createSensorSet(SensorSet& sensors,
                         QSharedPointer<FWDLIB::FwdCoilSet> coils);

    SensorSet                m_sensors;            /**< sensor struct that contains information about all sensors. */

private:
    //=========================================================================================================
    /**
     * Update FwdCoilSet and store into sensors struct.
     *
     */
    void updateSensor();

    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     */
    void updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    QList<FIFFLIB::FiffChInfo>   m_lChannels;             /**< Channellist with bads excluded. */
    QVector<int>                 m_vecInnerind;           /**< index of inner channels . */
    QList<QString>               m_lBads;                 /**< contains bad channels . */

    //=========================================================================================================
    /**
     * Update the model of sinoids for the hpi data
     *
     * @param[in] iSamF             The sample frequency.
     * @param[in] iSamLoc           The minimum samples required to localize numLoc times in a second.
     * @param[in] iLineF            The line frequency.
     * @param[in] vecFreqs          The frequencies for each coil in unknown order.
     *
     * @return The updated model.
     */
    void updateModel(const int iSamF,
                     const int iSamLoc,
                     const int iLineF,
                     const QVector<int>& vecFreqs);

    Eigen::MatrixXd     m_matModel;         /**< The model that contains the sines/cosines for the hpi fit*/
    bool                m_bDoFastFit;       /**< Do fast fit. */
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilTemplate;    /**< */
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilMeg;         /**< */
    QVector<int>        m_vecFreqs;         /**< The frequencies for each coil in unknown order. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#ifndef metatype_HpiFitResult
#define metatype_HpiFitResult
Q_DECLARE_METATYPE(INVERSELIB::HpiFitResult)
#endif

#endif // HPIFIT_H
