//=============================================================================================================
/**
 * @file     hpifitdata.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
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
 * @brief    HPIFitData class declaration.
 *
 */

#ifndef HPIFITDATA_H
#define HPIFITDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "hpifit.h"
#include "sensorset.h"

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
 * The strucut specifing the dipole error.
 */
struct DipFitError {
    double error;
    Eigen::MatrixXd moment;
    int numIterations;
};

//=========================================================================================================
/**
 * The strucut specifing the sorting parameters.
 */
struct HPISortStruct {
    double base_arr;
    int idx;
};

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * HPI Fit algorithm data structure.
 *
 * @brief HPI Fit algorithm data structure.
 */
class INVERSESHARED_EXPORT HPIFitData
{

public:
    typedef QSharedPointer<HPIFitData> SPtr;             /**< Shared pointer type for HPIFitData. */
    typedef QSharedPointer<const HPIFitData> ConstSPtr;  /**< Const shared pointer type for HPIFitData. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit HPIFitData();

    //=========================================================================================================
    /**
     * dipfit function is adapted from Fieldtrip Software.
     */
    void doDipfitConcurrent();

    Eigen::MatrixXd         m_coilPos;
    Eigen::RowVectorXd      m_sensorData;
    DipFitError             m_errorInfo;
    SensorSet               m_sensors;
    Eigen::MatrixXd         m_matProjector;

    int                     m_iMaxIterations;
    float                   m_fAbortError;

protected:
    //=========================================================================================================
    /**
     * magnetic_dipole leadfield for a magnetic dipole in an infinite medium.
     * The function has been compared with matlab magnetic_dipole and it gives same output.
     */
    Eigen::MatrixXd magnetic_dipole(Eigen::MatrixXd matPos,
                                    Eigen::MatrixXd matPnt,
                                    Eigen::MatrixXd matOri);

    //=========================================================================================================
    /**
     * compute_leadfield computes a forward solution for a dipole in a a volume
     * conductor model. The forward solution is expressed as the leadfield
     * matrix (Nchan*3), where each column corresponds with the potential or field
     * distributions on all sensors for one of the x,y,z-orientations of the dipole.
     * The function has been compared with matlab ft_compute_leadfield and it gives
     * same output.
     */
    Eigen::MatrixXd compute_leadfield(const Eigen::MatrixXd& matPos,
                                      const SensorSet sensors);

    //=========================================================================================================
    /**
     * dipfitError computes the error between measured and model data
     * and can be used for non-linear fitting of dipole position.
     * The function has been compared with matlab dipfit_error and it gives
     * same output
     */
    DipFitError dipfitError(const Eigen::MatrixXd& matPos,
                            const Eigen::MatrixXd& matData,
                            const SensorSet sensors,
                            const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * Compare function for sorting
     */
    static bool compare(HPISortStruct a, HPISortStruct b);

    //=========================================================================================================
    /**
     * fminsearch Multidimensional unconstrained nonlinear minimization (Nelder-Mead).
     * X = fminsearch(X0, iMaxiter, iMaxfun, iDisplay, matData, sensors) starts at X0 and
     * attempts to find a local minimizer
     */
    Eigen::MatrixXd fminsearch(const Eigen::MatrixXd& matPos,
                               int iMaxiter,
                               int iMaxfun,
                               int iDisplay,
                               const Eigen::MatrixXd& matData,
                               const Eigen::MatrixXd& matProjectors,
                               const SensorSet sensors,
                               int &iSimplexNumitr);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // HPIFITDATA_H
