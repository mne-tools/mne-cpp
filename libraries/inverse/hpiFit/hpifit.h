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
     */
    explicit HPIFit();

    //=========================================================================================================

    /**
     * Constructs the HPI object with FiffInfo.
     *
     * @param[in] pFiffInfo        Associated Fiff Information.
     */
    explicit HPIFit(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in]   t_mat                      Data to estimate the HPI positions from.
     * @param[in]   t_matProjectors            The projectors to apply. Bad channels are still included.
     * @param[out]  transDevHead               The final dev head transformation matrix.
     * @param[in]   vecFreqs                   The frequencies for each coil.
     * @param[out]  vecError                   The HPI estimation Error in mm for each fitted HPI coil.
     * @param[out]  vecGoF                     The goodness of fit for each fitted HPI coil.
     * @param[out]  fittedPointSet             The final fitted positions in form of a digitizer set.
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
                bool bDrop = false,
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
     * @param[out]  vecFreqs           The frequencies for each coil in correct order.
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
     * Fit linear model to data to get amplitudes for the dipole fit
     *
     * @param[in]   matData            Data to estimate the HPI positions from.
     * @param[in]   vecFreqs           The ordered coil frequencies.
     * @param[in]   pFiffInfo          Associated Fiff Information.
     * @param[out]  matAmplitudes      The computed amplitudes amplitudes (n_channels x n_coils).
     * @param[in]   iLineFreq          The line frequency in Hz to use for the advanced model. Defaults to 60 Hz.
     * @param[in]   bAdvanced          Use the advanced model to compute the coil amplitudes.
     *
     */
    void computeAmplitudes(const Eigen::MatrixXd& matData,
                           const QVector<int>& vecFreqs,
                           const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                           Eigen::MatrixXd& matAmplitudes,
                           const int iLineFreq = 60,
                           const bool bBasic = false);

    //=========================================================================================================
    /**
     * Compute the coil locations using dipole fit.
     *
     * @param[in]   matAmplitudes      The amplitudes fitted using computeAmplitudes.
     * @param[in]   t_matProjectors    The projectors to apply. Bad channels are still included.
     * @param[in]   transDevHead       The dev head transformation matrix for an initial guess.
     * @param[in]   pFiffInfo          Associated Fiff Information.
     * @param[out]  matCoilLoc         The computed coil locations.
     * @param[in]   iMaxIterations     The maximum allowed number of iterations used to fit the dipoles. Default is 500.
     * @param[in]   fAbortError        The error which will lead to aborting the dipole fitting process. Default is 1e-9.
     *
     */
    void computeCoilLoc(const Eigen::MatrixXd& matAmplitudes,
                        const Eigen::MatrixXd& matProjectors,
                        const FIFFLIB::FiffCoordTrans& transDevHead,
                        const QVector<int>& vecFreqs,
                        const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                        Eigen::MatrixXd& matCoilLoc,
                        int iMaxIterations = 500,
                        float fAbortError = 1e-9);

    //=========================================================================================================
    /**
     * Compute the device to head transformation.
     *
     * @param[in]   matCoilsDev         The estimated coil positions in device space.
     * @param[in]   pFiffInfo           Associated Fiff Information.
     * @param[out]  transDevHead        The dev head transformation matrix for an initial guess.
     * @param[out]  vecError            The HPI estimation Error in mm for each fitted HPI coil.
     * @param[out]  vecGof              The goodness of fit for each fitted HPI coil.
     * @param[out]  fittedPointSet      The final fitted positions in form of a digitizer set.
     * @param[in]   bDrop               Only use good coils yes/no. Defaults to yes.
     *
     */
    void computeHeadPos(const Eigen::MatrixXd& matCoilsDev,
                        const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                        FIFFLIB::FiffCoordTrans& transDevHead,
                        QVector<double> &vecError,
                        Eigen::VectorXd& vecGoF,
                        FIFFLIB::FiffDigPointSet& fittedPointSet,
                        bool bDrop = true);

    //=========================================================================================================
    /**
     * Update FwdCoilSet and store into sensors struct.
     *
     * @param[in] iAcc       The accuracy level to use for the sensor set. Defaults to 2 (highest).
     *
     */
    void updateSensor(const int iAcc = 2);

    //=========================================================================================================
    /**
     * Update the model of sinoids for the hpi data
     *
     * @param[in] iSamF             The sample frequency.
     * @param[in] iSamLoc           The minimum samples required to localize numLoc times in a second.
     * @param[in] iLineF            The line frequency.
     * @param[in] vecFreqs          The frequencies for each coil in unknown order.
     * @param[in] bBasic            Use the basic model yes/no.
     */
    void updateModel(const int iSamF,
                     const int iSamLoc,
                     const int iLineF,
                     const QVector<int>& vecFreqs,
                     bool bBasic);

    //=========================================================================================================
    /**
     * inline get functions for private member variables.
     *
     */
    inline QList<FIFFLIB::FiffChInfo> getChannels() const;
    inline QList<QString> getBads() const;
    inline SensorSet getSensors() const;
    inline Eigen::MatrixXd getModel() const;

    //=========================================================================================================
    /**
     * Store results from dev_Head_t as quaternions in position matrix. The format is the same as you
     * get from Neuromag's MaxFilter.
     *
     *
     * @param[in]   fTime           The corresponding time in the measurement for the fit.
     * @param[in]   pFiffInfo       The FiffInfo file from the measurement.
     * @param[out]  matPosition     The matrix to store the results.
     * @param[in]   vecGoF          The goodness of fit for each coil.
     * @param[in]   vecError        The Hpi estimation Error per coil.
     *
     * ToDo: get estimated movement velocity and store it in channel 9
     */
    static void storeHeadPosition(float fTime,
                                  const Eigen::MatrixXf& transDevHead,
                                  Eigen::MatrixXd& matPosition,
                                  const Eigen::VectorXd& vecGoF,
                                  const QVector<double>& vecError);

private:
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
     * Drop coils with worst GoF to improve the transformation.
     *
     * @param[in]   vecGoF        The goodness of fit per coil.
     * @param[in]   matCoil       The fitted coil positions (device space).
     * @param[in]   matHeadCoil   The digitized coil positions (head space).
     * @param[out]  vecInd        The indices of the coils used to trace back which one was dropped.
     *
     * @return Returns the transformation matrix.
     */
    Eigen::MatrixXd dropCoils(const Eigen::VectorXd vecGoF,
                              const Eigen::MatrixXd matCoil,
                              const Eigen::MatrixXd matHeadCoil,
                              Eigen::VectorXi& vecInd);

    //=========================================================================================================
    /**
     * The objective function to measure the goodness of the calculated transform.
     *
     * @param[in] matCoil       The fitted coil positions (device space).
     * @param[in] matHeadCoil   The digitized coil positions (head space).
     * @param[in] matHeadCoil   The digitized coil positions (head space).
     *
     * @return Returns the fiducial registration error.
     */
    double objectTrans(const Eigen::MatrixXd matHeadCoil,
                       const Eigen::MatrixXd matCoil,
                       const Eigen::MatrixXd matTrans);

    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     * @param[in] pFiffInfo       The FiffInfo file from the measurement.
     *
     */
    void updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    QList<FIFFLIB::FiffChInfo>          m_lChannels;        /**< Channellist with bads excluded. */
    QVector<int>                        m_vecInnerind;      /**< index of inner channels . */
    QList<QString>                      m_lBads;            /**< contains bad channels . */
    SensorSet                           m_sensors;          /**< sensor struct that contains information about all sensors. */
    Eigen::MatrixXd                     m_matModel;         /**< The model that contains the sines/cosines for the hpi fit*/
    bool                                m_bDoFastFit;       /**< Do fast fit. */
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilTemplate;    /**< */
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilMeg;         /**< */
    QVector<int>                        m_vecFreqs;         /**< The frequencies for each coil in unknown order. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
inline QList<FIFFLIB::FiffChInfo> HPIFit::getChannels() const
{
    return m_lChannels;
}

inline QList<QString> HPIFit::getBads() const
{
    return m_lBads;
}

inline SensorSet HPIFit::getSensors() const
{
    return m_sensors;
}

inline Eigen::MatrixXd HPIFit::getModel() const
{
    return m_matModel;
}

} //NAMESPACE

#ifndef metatype_HpiFitResult
#define metatype_HpiFitResult
Q_DECLARE_METATYPE(INVERSELIB::HpiFitResult)
#endif

#endif // HPIFIT_H
