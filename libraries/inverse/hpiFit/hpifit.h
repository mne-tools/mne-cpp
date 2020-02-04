//=============================================================================================================
/**
 * @file     hpifit.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
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
    explicit HPIFit();

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in]    t_mat           Data to estimate the HPI positions from
     * @param[in]    t_matProjectors The projectors to apply. Bad channels are still included.
     * @param[out]   transDevHead    The final dev head transformation matrix
     * @param[in]    vFreqs          The frequencies for each coil.
     * @param[out]   vGof            The goodness of fit in mm for each fitted HPI coil.
     * @param[out]   fittedPointSet  The final fitted positions in form of a digitizer set.
     * @param[in]    p_pFiffInfo     Associated Fiff Information.
     * @param[in]    bDoDebug        Print debug info to cmd line and write debug info to file.
     * @param[in]    sHPIResourceDir The path to the debug file which is to be written.
     */
    static void fitHPI(const Eigen::MatrixXd& t_mat,
                       const Eigen::MatrixXd& t_matProjectors,
                       FIFFLIB::FiffCoordTrans &transDevHead,
                       const QVector<int>& vFreqs,
                       QVector<double> &vGof,
                       FIFFLIB::FiffDigPointSet& fittedPointSet,
                       QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                       bool bDoDebug = false,
                       const QString& sHPIResourceDir = QString("./HPIFittingDebug"));


protected:
    //=========================================================================================================
    /**
     * Fits dipoles for the given coils and a given data set.
     *
     * @param[in] CoilParam       The coil parameters.
     * @param[in] sensors         The sensor information.
     * @param[in] data            The data which used to fit the coils.
     * @param[in] numCoils        The number of coils.
     * @param[in] t_matProjectors The projectors to apply. Bad channels are still included.
     *
     * @return Returns the coil parameters.
     */
    static CoilParam dipfit(struct CoilParam coil,
                            struct SensorInfo sensors,
                            const Eigen::MatrixXd &data,
                            int numCoils,
                            const Eigen::MatrixXd &t_matProjectors);

    //=========================================================================================================
    /**
     * Computes the transformation matrix between two sets of 3D points.
     *
     * @param[in] NH     The first set of input 3D points (row-wise order).
     * @param[in] BT     The second set of input 3D points (row-wise order).
     *
     * @return Returns the transformation matrix.
     */
    static Eigen::Matrix4d computeTransformation(Eigen::MatrixXd NH, Eigen::MatrixXd BT);

    /**
     * Read from FwdCoilSet and store into sensors struct.
     * Can be deleted as soon as FwdCoilSet is refactored to QList and EigenMatrix.
     *
     * @param[in] sensors     The struct to save sensor information.
     * @param[in] coils     The coilset to read the sensor information from.
     *
     */
    static void create_sensor_set(QList<struct SInfo>& sensors, FWDLIB::FwdCoilSet* coils);

    static QString         m_sHPIResourceDir;      /**< Hold the resource folder to store the debug information in. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // HPIFIT_H
