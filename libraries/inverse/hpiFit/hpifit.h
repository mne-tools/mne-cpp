//=============================================================================================================
/**
 * @file     hpifit.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <doerfelruben@aol.com>;
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
#include "sensorset.h"
#include "signalmodel.h"
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
    class SensorSet;
    class SignalModel;
    class HpiModelParameters;
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

    CoilParam(int iNumCoils)
        : pos(Eigen::MatrixXd(iNumCoils,3)),
          mom(Eigen::MatrixXd::Zero(iNumCoils,3)),
          dpfiterror(Eigen::VectorXd::Zero(iNumCoils)),
          dpfitnumitr(Eigen::VectorXd::Zero(iNumCoils))
    {}
};

/**
 * The struct specifing all data needed to perform coil-wise fitting.
 */
struct HpiFitResult {
    QVector<int>                hpiFreqs;
    FIFFLIB::FiffDigPointSet    fittedCoils;
    FIFFLIB::FiffCoordTrans     devHeadTrans;
    QVector<double>             errorDistances;
    Eigen::VectorXd             GoF;
    QString                     sFilePathDigitzers;
    bool                        bIsLargeHeadMovement;
    float                       fHeadMovementDistance;
    float                       fHeadMovementAngle;
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
     */
    explicit HPIFit();

    //=========================================================================================================
    /**
     * Constructs the HPI from a SensorSet.
     *
     * @param[in] SensorSet     The MEG sensorSet used for the hpi fitting.
     */
    explicit HPIFit(const SensorSet& sensorSet);

    //=========================================================================================================
    /**
     * Checks if SensorSet has changed and updates member SensorSet.
     *
     * @param[in] SensorSet     The MEG sensorSet used for the hpi fitting.
     */
    void checkForUpdate(const SensorSet& sensorSet);

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in]   matProjectedData            Data to estimate the HPI positions from. Projectars should be already applied.
     * @param[in]   matProjectors               The projectors to apply.
     * @param[in]   hpiModelParameters          The model parameters to use for the Hpi Fitting, especially to compute the coil amplitudes.
     * @param[in]   matCoilsHead                The hpi coil locations in head space.
     * @param[in]   bOrderFrequencies           Order Hpi coils yes/no.
     * @param[out]  hpiFitResult                The fitting results.
     */
    void fit(const Eigen::MatrixXd& matProjectedData,
             const Eigen::MatrixXd& matProjectors,
             const HpiModelParameters& hpiModelParameters,
             const Eigen::MatrixXd& matCoilsHead,
             HpiFitResult& hpiFitResult);

    void fit(const Eigen::MatrixXd& matProjectedData,
             const Eigen::MatrixXd& matProjectors,
             const HpiModelParameters& hpiModelParameters,
             const Eigen::MatrixXd& matCoilsHead,
             const bool bOrderFrequencies,
             HpiFitResult& hpiFitResult);

    //=========================================================================================================
    /**
     * Store results from dev_Head_t as quaternions in position matrix. The format is the same as you
     * get from Neuromag's MaxFilter.
     *
     *
     * @param[in]   fTime               The corresponding time in the measurement for the fit.
     * @param[in]   matTransDevHead     The device->head transformation matrix.
     * @param[out]  matPosition         The matrix to store the results.
     * @param[in]   vecGoF              The goodness of fit per coil.
     * @param[in]   vecError            The Hpi estimation Error per coil.
     *
     * ToDo: get estimated movement velocity and store it in channel 9
     */
    static void storeHeadPosition(float fTime,
                                  const Eigen::MatrixXf& matTransDevHead,
                                  Eigen::MatrixXd& matPosition,
                                  const Eigen::VectorXd& vecGoF,
                                  const QVector<double>& vecError);

private:

    //=========================================================================================================
    /**
     * Fit linear model to data to get amplitudes for the dipole fit.
     *
     * @param[in]   matProjectedData    Projected data to estimate the HPI positions from.
     * @param[in]   hpiModelParameters     The model parameters to use for the hpi signal model.
     * @return  matAmplitudes       The computed amplitudes amplitudes (n_channels x n_coils).
     *
     */
    Eigen::MatrixXd computeAmplitudes(const Eigen::MatrixXd& matProjectedData,
                                      const HpiModelParameters& hpiModelParameters);

    //=========================================================================================================
    /**
     * Compute initial coil positions for the dipole fit.
     *
     * @param[in]   matAmplitudes       The amplitudes fitted using computeAmplitudes.
     * @param[in]   transDevHead        The dev head transformation matrix for an initial guess.
     * @param[in]   vecError            The Error Distances from the last fit.
     * @param[in]   matCoilsHead        The hpi coil locations in head space.
     * @return Returns the seed points.
     */
    Eigen::MatrixXd computeSeedPoints(const Eigen::MatrixXd& matAmplitudes,
                                      const FIFFLIB::FiffCoordTrans& transDevHead,
                                      const QVector<double>& vecError,
                                      const Eigen::MatrixXd& matCoilsHead);

    //=========================================================================================================
    /**
     * Fits dipoles for the given coils and a given data set.
     *
     * @param[in]   coil              The coil parameters.
     * @param[in]   sensors           The sensor information.
     * @param[in]   matData           The data which used to fit the coils.
     * @param[in]   iNumCoils         The number of coils.
     * @param[in]   t_matProjectors   The projectors to apply.
     * @param[in]   iMaxIterations    The maximum allowed number of iterations used to fit the dipoles. Default is 500.
     * @param[in]   fAbortError       The error which will lead to aborting the dipole fitting process. Default is 1e-9.
     *
     * @return Returns the coil parameters.
     */
    CoilParam dipfit(const Eigen::MatrixXd matCoilsSeed,
                     const SensorSet& sensors,
                     const Eigen::MatrixXd &matData,
                     const int iNumCoils,
                     const Eigen::MatrixXd &t_matProjectors,
                     const int iMaxIterations,
                     const float fAbortError);

    //=========================================================================================================
    /**
     * Compute the goodness of fit from the dipole fit error.
     *
     * @param[in]   vecDipFitError      The dipole fit error.
     * @return Returns the goodness of fit.
     */
    Eigen::VectorXd computeGoF(const Eigen::VectorXd& vecDipFitError);

    //=========================================================================================================
    /**
     * Compute the device to head transformation matrix.
     *
     * @param[in]   matCoilsDev         The estimated coil positions in device space.
     * @param[in]   matCoilsHead        The hpi coil locations in head space.
     * @return  Returns the dev head transformation matrix.
     * @param[out]  vecError            The HPI estimation Error in mm for each fitted HPI coil.
     * @param[out]  fittedPointSet      The final fitted positions in form of a digitizer set.
     *
     */
    FIFFLIB::FiffCoordTrans computeDeviceHeadTransformation(const Eigen::MatrixXd& matCoilsDev,
                                                            const Eigen::MatrixXd& matCoilsHead);

    //=========================================================================================================
    /**
     * Compute the distance between fitted and digitized coils.
     *
     * @param[in]   matCoilsDev         The estimated coil positions in device space.
     * @param[in]   matCoilsHead        The hpi coil locations in head space.
     * @param[out]  transDevHead        The dev->head transformation matrix.
     * @return Returns the coil estimation error.
     */
    QVector<double> computeEstimationError(const Eigen::MatrixXd& matCoilsDev,
                                           const Eigen::MatrixXd& matCoilsHead,
                                           const FIFFLIB::FiffCoordTrans& transDevHead);

    //=========================================================================================================
    /**
     * Converts the fitted coils from a MAtrix into a fitted point set.
     *
     * @param[in]   matCoilsDev         The estimated coil positions in device space.
     * @return The final fitted positions in form of a digitizer set.
     *
     */
    FIFFLIB::FiffDigPointSet getFittedPointSet(const Eigen::MatrixXd& matCoilsDev);

    //=========================================================================================================
    /**
     * Computes the transformation matrix between two sets of 3D points.
     *
     * @param[in]   matCoilsDev         The estimated coil positions in device space.
     * @param[in]   matCoilsHead        The hpi coil locations in head space.
     *
     * @return Returns the transformation matrix.
     */
    Eigen::Matrix4d computeTransformation(Eigen::MatrixXd matNH,
                                          Eigen::MatrixXd matBT);

    //=========================================================================================================
    /**
     * Find the coil ordering.
     *
     * @param[in] matNH    The first set of input 3D points (row-wise order) - To.
     * @param[in] matBT    The second set of input 3D points (row-wise order) - From.
     *
     * @return Returns the order of the coils in head space.
     */
    std::vector<int> findCoilOrder(const Eigen::MatrixXd& matCoilsDev,
                                   const Eigen::MatrixXd& matCoilsHead);

    //=========================================================================================================
    /**
     * Order a vector or matrix.
     *
     * @param[in] vecOrder      The order.
     * @param[in] ToOrder       The vector/matrix to order.
     *
     * @return Returns the ordered vector/matrix.
     */
    Eigen::MatrixXd order(const std::vector<int>& vecOrder,
                          const Eigen::MatrixXd& matToOrder);

    QVector<int> order(const std::vector<int>& vecOrder,
                       const QVector<int>& vecToOrder);

    //=========================================================================================================
    /**
     * The objective function to measure the goodness of the calculated transform.
     *
     * @param[in] matCoilsDev       The fitted coil positions (device space).
     * @param[in] matCoilsHead      The digitized coil positions (head space).
     * @param[in] matTrans          The dev head transformation matrix.
     *
     * @return Returns the registration error.
     */
    double objectTrans(const Eigen::MatrixXd& matHeadCoil,
                       const Eigen::MatrixXd& matCoilsDev,
                       const Eigen::MatrixXd& matTrans);

    SensorSet m_sensors;            /**< The sensor struct that contains information about all sensors. */
    SignalModel m_signalModel;      /**< The signal model for the Hpi signals used to compute extract the coil amplitudes */

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
