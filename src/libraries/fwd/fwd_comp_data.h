//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_comp_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Software-gradiometer compensation wrapper that subtracts the reference-channel contribution from the primary forward field.
 *
 * CTF whole-head and 4D/BTi MEG systems acquire a bank of *reference
 * channels* located far from the head and form synthetic higher-order
 * gradiometers by subtracting linear combinations of those references
 * from each primary channel. The compensation coefficients @c k are
 * stored in the FIFF file as a @c MNECTFCompDataSet. To keep the forward
 * model consistent with the data, the same linear combination must be
 * applied to the predicted field: @c B_comp = B_primary − k·B_reference.
 *
 * FwdCompData wraps an underlying field/grad function pair (BEM or
 * sphere), evaluates it once for the primary coils and once for the
 * reference coils, and returns the compensated result. The class
 * mirrors @c fwdCompDataRec from MNE-C @c fwd_comp_data.h and preserves
 * the same callback signatures so it can be dropped into the source-space
 * loop without changes upstream.
 */

#ifndef FWD_COMP_DATA_H
#define FWD_COMP_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_types.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MNECTFCompDataSet;
}

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FWDLIB FORWARD DECLARATIONS
//=============================================================================================================

class FwdCoilSet;

//=============================================================================================================
/**
 * Implements the Forward Compensation Data description (replaces @c fwdCompData / @c fwdCompDataRec from MNE-C @c fwd_comp_data.h).
 *
 * @brief CTF / 4D software-gradiometer wrapper that re-evaluates the primary field callback on a separate reference-coil set and subtracts the linear combination @c k·B_ref from the primary field to mirror the compensation already applied to the recorded data.
 */
class FWDSHARED_EXPORT FwdCompData
{
public:

    //=========================================================================================================
    /**
     * Constructs the Forward Compensation Data
     */
    FwdCompData();

    //=========================================================================================================
    /**
     * Destroys the Forward Compensation Data
     */
    ~FwdCompData();

    //=========================================================================================================
    /**
     * Calculate the compensated field for one dipole component.
     *
     * @param[in] rd       Dipole position.
     * @param[in] Q        Dipole moment direction.
     * @param[in] coils    Coil definitions.
     * @param[out] res     Result vector.
     * @param[in] client   Pointer to FwdCompData.
     *
     * @return OK on success, FAIL on error.
     */
    static int fwd_comp_field(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils, Eigen::Ref<Eigen::VectorXf> res, void *client);

    //=========================================================================================================
    /**
     * Set up CTF compensation coils for field computations.
     *
     * @param[in] set          Available compensation data.
     * @param[in] coils        Main coil set.
     * @param[in] comp_coils   Compensation coils.
     *
     * @return OK on success, FAIL on error.
     */
    static int fwd_make_ctf_comp_coils(MNELIB::MNECTFCompDataSet* set,
                                       FwdCoilSet*        coils,
                                       FwdCoilSet*        comp_coils);

    //=========================================================================================================
    /**
     * Compose a compensation data set.
     *
     * @param[in] set          CTF compensation data read from file.
     * @param[in] coils        Principal coil set.
     * @param[in] comp_coils   Compensation coils.
     * @param[in] field        Field computation function (single dipole component).
     * @param[in] vec_field    Vector field computation function (all components).
     * @param[in] field_grad   Field and gradient computation function.
     * @param[in] client       Client data passed to the computation functions.
     *
     * @return Pointer to the created FwdCompData, or nullptr on error. Caller owns the pointer.
     */
    static FwdCompData* fwd_make_comp_data(MNELIB::MNECTFCompDataSet* set,
                                   FwdCoilSet*        coils,
                                   FwdCoilSet*        comp_coils,
                                   fwdFieldFunc      field,
                                   fwdVecFieldFunc   vec_field,
                                   fwdFieldGradFunc  field_grad,
                                   void              *client);

    //=========================================================================================================
    /**
     * Calculate the compensated field for all three dipole components.
     *
     * @param[in] rd       Dipole position.
     * @param[in] coils    Coil definitions.
     * @param[out] res     Result matrix (3 x ncoil).
     * @param[in] client   Pointer to FwdCompData.
     *
     * @return OK on success, FAIL on error.
     */
    static int fwd_comp_field_vec(const Eigen::Vector3f& rd, FwdCoilSet& coils, Eigen::Ref<Eigen::MatrixXf> res, void *client);

    //=========================================================================================================
    /**
     * Calculate the compensated field and gradient for one dipole component.
     *
     * @param[in] rd       Dipole position.
     * @param[in] Q        Dipole moment direction.
     * @param[in] coils    Coil definitions.
     * @param[out] res     Result vector.
     * @param[out] xgrad   X-gradient result.
     * @param[out] ygrad   Y-gradient result.
     * @param[out] zgrad   Z-gradient result.
     * @param[in] client   Pointer to FwdCompData.
     *
     * @return OK on success, FAIL on error.
     */
    static int fwd_comp_field_grad(const Eigen::Vector3f& rd, const Eigen::Vector3f& Q, FwdCoilSet& coils,
                Eigen::Ref<Eigen::VectorXf> res, Eigen::Ref<Eigen::VectorXf> xgrad, Eigen::Ref<Eigen::VectorXf> ygrad, Eigen::Ref<Eigen::VectorXf> zgrad,
                void *client);

public:
    MNELIB::MNECTFCompDataSet*  set;        /**< The compensation data set. */
    FwdCoilSet*         comp_coils; /**< The compensation coil definitions. */
    fwdFieldFunc        field;      /**< Computes the field of given direction dipole. */
    fwdVecFieldFunc     vec_field;  /**< Computes the fields of all three dipole components. */
    fwdFieldGradFunc    field_grad; /**< Computes the field and gradient of one dipole direction. */
    void                *client;    /**< Client data to pass to the above functions. */
    Eigen::VectorXf     work;       /**< The work area. */
    Eigen::MatrixXf     vec_work;   /**< The vector work area (3 x ncoil). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_COMP_DATA_H
