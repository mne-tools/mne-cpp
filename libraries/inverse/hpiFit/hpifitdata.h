//=============================================================================================================
/**
 * @file     hpifitdata.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @version  dev
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
 * @brief    HpiFitData class declaration.
 *
 */

#ifndef HPIFITDATA_H
#define HPIFITDATA_H

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
 * The strucut specifing the dipole error.
 */
struct DipFitError {
    double error;
    Eigen::MatrixXd moment;
    int numIterations;
};

//=========================================================================================================
/**
 * The strucut specifing the sensor parameters.
 */
struct SensorInfo {
    Eigen::MatrixXd coilpos;
    Eigen::MatrixXd coilori;
    Eigen::MatrixXd tra;
};

//=========================================================================================================
/**
 * The strucut specifing the sorting parameters.
 */
struct HpiSortStruct {
    double base_arr;
    int idx;
};


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * HPI Fit algorithm data structure.
 *
 * @brief HPI Fit algorithm data structure.
 */
class INVERSESHARED_EXPORT HpiFitData
{

public:
    typedef QSharedPointer<HpiFitData> SPtr;             /**< Shared pointer type for HpiFitData. */
    typedef QSharedPointer<const HpiFitData> ConstSPtr;  /**< Const shared pointer type for HpiFitData. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit HpiFitData();

    //=========================================================================================================
    /**
     * dipfit function is adapted from Fieldtrip Software. It has been heavily edited for use with MNE Scan Software.
     */
    void doDipfitConcurrent();

    Eigen::RowVectorXd  coilPos;        /**< . */
    Eigen::RowVectorXd  sensorData;     /**< . */
    DipFitError         errorInfo;      /**< . */
    SensorInfo          sensorPos;      /**< . */
    Eigen::MatrixXd     matProjector;   /**< . */

protected:
    //=========================================================================================================
    /**
     * magnetic_dipole leadfield for a magnetic dipole in an infinite medium.
     * The function has been compared with matlab magnetic_dipole and it gives same output.
     *
     * @param[in] pos
     * @param[in] pnt
     * @param[in] ori
     */
    Eigen::MatrixXd magnetic_dipole(Eigen::MatrixXd pos,
                                    Eigen::MatrixXd pnt,
                                    Eigen::MatrixXd ori);

    //=========================================================================================================
    /**
     * compute_leadfield computes a forward solution for a dipole in a a volume
     * conductor model. The forward solution is expressed as the leadfield
     * matrix (Nchan*3), where each column corresponds with the potential or field
     * distributions on all sensors for one of the x,y,z-orientations of the dipole.
     * The function has been compared with matlab ft_compute_leadfield and it gives
     * same output.
     *
     * @param[in] pos
     * @param[in] sensors
     */
    Eigen::MatrixXd compute_leadfield(const Eigen::MatrixXd& pos,
                                      const struct SensorInfo& sensors);

    //=========================================================================================================
    /**
     * dipfitError computes the error between measured and model data
     * and can be used for non-linear fitting of dipole position.
     * The function has been compared with matlab dipfit_error and it gives
     * same output
     *
     * @param[in] pos
     * @param[in] data
     * @param[in] sensors
     * @param[in] matProjectors
     */
    DipFitError dipfitError(const Eigen::MatrixXd& pos,
                            const Eigen::MatrixXd& data,
                            const struct SensorInfo& sensors,
                            const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * Compare function for sorting
     *
     * @param[in] a
     * @param[in] b
     */
    static bool compare(HpiSortStruct a,
                        HpiSortStruct b);

    //=========================================================================================================
    /**
     * fminsearch Multidimensional unconstrained nonlinear minimization (Nelder-Mead).
     * X = fminsearch(X0, maxiter, maxfun, display, data, sensors) starts at X0 and
     * attempts to find a local minimizer
     *
     * @param[in] pos
     * @param[in] maxiter
     * @param[in] maxfun
     * @param[in] display
     * @param[in] data
     * @param[in] matProjectors
     * @param[in] sensors
     * @param[in] simplex_numitr
     */
    Eigen::MatrixXd fminsearch(const Eigen::MatrixXd& pos,
                               int maxiter,
                               int maxfun,
                               int display,
                               const Eigen::MatrixXd& data,
                               const Eigen::MatrixXd& matProjectors,
                               const struct SensorInfo& sensors,
                               int &simplex_numitr);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // HPIFITDATA_H
