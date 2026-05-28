//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_hpi_fit_data.h
 * @since March 2026
 * @brief Per-coil magnetic-dipole fitting workspace — Nelder-Mead optimiser plus leadfield computation for HPI coil localisation.
 *
 * @ref INVLIB::InvHpiFitData implements the inner loop of
 * @ref InvHpiFit: for every HPI coil it computes the magnetic-dipole
 * leadfield in an infinite homogeneous medium, evaluates the residual
 * between the model field and the measured projection, and runs a
 * Nelder-Mead simplex search (@c fminsearch) to refine the coil
 * position. Helper structs @ref DipFitError and @ref HPISortStruct
 * carry per-iteration diagnostics and the post-fit coil-ordering
 * metadata. The leadfield and fit-error routines are validated against
 * the FieldTrip @c magnetic_dipole / @c ft_compute_leadfield reference
 * implementations.
 */

#ifndef INV_HPI_FIT_DATA_H
#define INV_HPI_FIT_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_hpi_fit.h"
#include "inv_sensor_set.h"

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
// DEFINE NAMESPACE HPILIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================
/**
 * The strucut specifing the dipole error.
 *
 * @brief Residual error and moment vector from a single magnetic dipole fit iteration
 */
struct DipFitError {
    double error;
    Eigen::MatrixXd moment;
    int numIterations;
};

//=========================================================================================================
/**
 * The strucut specifing the sorting parameters.
 *
 * @brief Helper for sorting HPI coil dipole fits by matching each fit to the nearest expected coil position
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
 * Per-coil magnetic-dipole fitter: computes the infinite-medium leadfield
 * for a candidate coil position, evaluates the residual against the
 * projected MEG sample and refines the position with a Nelder-Mead simplex
 * search. Routines are validated against FieldTrip's @c magnetic_dipole
 * and @c ft_compute_leadfield reference implementations.
 *
 * @brief Per-coil magnetic-dipole fitter (leadfield, residual, Nelder-Mead refinement) for the HPI pipeline.
 */
class INVSHARED_EXPORT InvHpiFitData
{

public:
    typedef QSharedPointer<InvHpiFitData> SPtr;             /**< Shared pointer type for InvHpiFitData. */
    typedef QSharedPointer<const InvHpiFitData> ConstSPtr;  /**< Const shared pointer type for InvHpiFitData. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit InvHpiFitData();

    //=========================================================================================================
    /**
     * dipfit function is adapted from Fieldtrip Software.
     */
    void doDipfitConcurrent();

    Eigen::MatrixXd         m_coilPos;
    Eigen::RowVectorXd      m_sensorData;
    DipFitError             m_errorInfo;
    InvSensorSet               m_sensors;
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
                                      const InvSensorSet& sensors);

    //=========================================================================================================
    /**
     * dipfitError computes the error between measured and model data
     * and can be used for non-linear fitting of dipole position.
     * The function has been compared with matlab dipfit_error and it gives
     * same output
     */
    DipFitError dipfitError(const Eigen::MatrixXd& matPos,
                            const Eigen::MatrixXd& matData,
                            const InvSensorSet& sensors,
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
                               const InvSensorSet& sensors,
                               int &iSimplexNumitr);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // INV_HPI_FIT_DATA_H
