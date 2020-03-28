//=============================================================================================================
/**
 * @file     hpifit.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @version  dev
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
};

/**
 * The strucut specifing the sensor parameters.
 */
struct Sensor {
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
     */
    explicit HPIFit(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in]    t_mat           Data to estimate the HPI positions from
     * @param[in]    t_matProjectors The projectors to apply. Bad channels are still included.
     * @param[out]   transDevHead    The final dev head transformation matrix
     * @param[in]    vFreqs          The frequencies for each coil.
     * @param[out]   vError          The HPI estimation Error in mm for each fitted HPI coil.
     * @param[out]   vGoF            The goodness of fit for each fitted HPI coil
     * @param[out]   fittedPointSet  The final fitted positions in form of a digitizer set.
     * @param[in]    pFiffInfo     Associated Fiff Information.
     * @param[in]    bDoDebug        Print debug info to cmd line and write debug info to file.
     * @param[in]    sHPIResourceDir The path to the debug file which is to be written.
     */
    void fitHPI(const Eigen::MatrixXd& t_mat,
                const Eigen::MatrixXd& t_matProjectors,
                FIFFLIB::FiffCoordTrans &transDevHead,
                const QVector<int>& vFreqs,
                QVector<double> &vError,
                Eigen::VectorXd& vGoF,
                FIFFLIB::FiffDigPointSet& fittedPointSet,
                QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                bool bDoDebug = false,
                const QString& sHPIResourceDir = QString("./HPIFittingDebug"));

    //=========================================================================================================
    /**
     * assign frequencies to correct position
     *
     * @param[in]    t_mat           Data to estimate the HPI positions from
     * @param[in]    t_matProjectors The projectors to apply. Bad channels are still included.
     * @param[in]    transDevHead    The final dev head transformation matrix
     * @param[in]    vFreqs          The frequencies for each coil in unknown order.
     * @param[out]   vFreqs          The frequencies for each coil in correct order.
     * @param[in]    vError          The HPI estimation Error in mm for each fitted HPI coil.
     * @param[in]    vGoF            The goodness of fit for each fitted HPI coil
     * @param[in]    fittedPointSet  The final fitted positions in form of a digitizer set.
     * @param[in]    pFiffInfo     Associated Fiff Information.
     */
    void findOrder(const Eigen::MatrixXd& t_mat,
                   const Eigen::MatrixXd& t_matProjectors,
                   FIFFLIB::FiffCoordTrans &transDevHead,
                   QVector<int>& vFreqs,
                   QVector<double> &vError,
                   Eigen::VectorXd& vGoF,
                   FIFFLIB::FiffDigPointSet& fittedPointSet,
                   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Store results from dev_Head_t as quaternions in position matrix. The format is the same as you
     * get from Neuromag's MaxFilter.
     *
     *
     * @param[in]   fTime          The corresponding time in the measurement for the fit.
     * @param[in]   pFiffInfo     The FiffInfo file from the measurement.
     * @param[out]  mPosition      The matrix to store the results.
     * @param[in]   vGoF          The goodness of fit for each coil.
     * @param[in]   vError        The Hpi estimation Error per coil.
     *
     * ToDo: get estimated movement velocity and store it in channel 9
     */
    static void storeHeadPosition(float fTime,
                                  const Eigen::MatrixXf& transDevHead,
                                  Eigen::MatrixXd& mPosition,
                                  const Eigen::VectorXd& vGoF,
                                  const QVector<double>& vError);
protected:
    //=========================================================================================================
    /**
     * Fits dipoles for the given coils and a given data set.
     *
     * @param[in] CoilParam       The coil parameters.
     * @param[in] lSensorSet      The sensor information.
     * @param[in] mData           The data which used to fit the coils.
     * @param[in] iNumCoils       The number of coils.
     * @param[in] t_matProjectors The projectors to apply. Bad channels are still included.
     *
     * @return Returns the coil parameters.
     */
    CoilParam dipfit(struct CoilParam coil,
                     const Sensor& sensors,
                     const Eigen::MatrixXd &mData,
                     int iNumCoils,
                     const Eigen::MatrixXd &t_matProjectors);

    //=========================================================================================================
    /**
     * Computes the transformation matrix between two sets of 3D points.
     *
     * @param[in] mNH    The first set of input 3D points (row-wise order).
     * @param[in] mBT    The second set of input 3D points (row-wise order).
     *
     * @return Returns the transformation matrix.
     */

    Eigen::Matrix4d computeTransformation(Eigen::MatrixXd mNH,
                                          Eigen::MatrixXd mBT);

    //=========================================================================================================
    /**
     * Read from FwdCoilSet and store into lSensorSet struct.
     * Can be deleted as soon as FwdCoilSet is refactored to QList and EigenMatrix.
     *
     * @param[in] lSensorSet    The struct to save sensor information.
     * @param[in] coils         The coilset to read the sensor information from.
     *
     */
    void createSensorSet(Sensor& sensor,
                         FWDLIB::FwdCoilSet* coils);

    //=========================================================================================================

    Sensor                m_sensors;            /**< sensors */

private:
    //=========================================================================================================
    /**
     * Update FwdCoilSet and store into lSensorSet struct.
     *
     */
    void updateCoils();

    FWDLIB::FwdCoilSet* m_coilTemplate;
    FWDLIB::FwdCoilSet* m_coilMeg;

    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     */
    void updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    QList<FIFFLIB::FiffChInfo>   m_lChannels;             /**< Channellist with excluded bads */
    QVector<int>                 m_vInnerind;             /**< index of inner channels  */
    QList<QString>               m_lBads;                 /**< contains bad channels  */

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
