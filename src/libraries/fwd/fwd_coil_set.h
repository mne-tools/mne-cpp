//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwd_coil_set.h
 * @since March 2017
 * @brief Container of FwdCoil instances representing either a sensor-type *template database* or a concrete per-channel sensor array.
 *
 * FwdCoilSet serves two roles. As a *template database* it is the
 * in-memory image of @c coil_def.dat, the canonical CTF/Elekta file
 * mapping a numeric coil type (3022 = MEGIN VectorView magnetometer,
 * 3024 = planar gradiometer, 5001 = CTF axial gradiometer, ...) to a
 * normalised set of integration points and weights. As a *per-channel
 * array* it holds one FwdCoil per recorded MEG channel, with the
 * template's integration points already rotated and translated into the
 * device-, head- or MRI-coordinate frame via the supplied FiffCoordTrans.
 *
 * Mirrors @c fwdCoilSetRec from MNE-C; @c create_meg_coils() is the
 * counterpart of @c read_meg_coils()/@c make_meg_coils() and is the only
 * route by which raw @c FiffChInfo records become objects the forward
 * solver can integrate against.
 */

#ifndef FWD_COIL_SET_H
#define FWD_COIL_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_coil.h"
#include <fiff/fiff_types.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{
class FwdBemSolution;
//=============================================================================================================
/**
 * Implements FwdCoilSet (replaces @c fwdCoilSet / @c fwdCoilSetRec from MNE-C @c fwd_types.h).
 *
 * @brief Container of FwdCoil instances acting both as the in-memory image of the @c coil_def.dat template database and as a per-channel sensor array in a chosen coordinate frame (device / head / MRI).
 */
class FWDSHARED_EXPORT FwdCoilSet
{
public:
    typedef QSharedPointer<FwdCoilSet> SPtr;              /**< Shared pointer type for FwdCoilSet. */
    typedef QSharedPointer<const FwdCoilSet> ConstSPtr;   /**< Const shared pointer type for FwdCoilSet. */
    typedef std::unique_ptr<FwdCoilSet> UPtr;             /**< Unique pointer type for FwdCoilSet. */

    //=========================================================================================================
    /**
     * Constructs the Forward Coil Set description
     */
    FwdCoilSet();

    //=========================================================================================================
    /**
     * Destroys the Forward Coil Set description
     */
    ~FwdCoilSet();

    //=========================================================================================================
    /**
     * Create a MEG coil definition using a database of templates
     * Change the coordinate frame if so desired
     *
     * @param[in] ch     Channel information to use.
     * @param[in] acc    Required accuracy.
     * @param[in] t      Transform the points using this.
     *
     * @return   The created meg coil.
     */
    FwdCoil::UPtr create_meg_coil(const FIFFLIB::FiffChInfo& ch, int acc, const FIFFLIB::FiffCoordTrans& t = FIFFLIB::FiffCoordTrans());

    //=========================================================================================================
    /**
     * Create a MEG coil set definition using a database of templates
     * Change the coordinate frame if so desired
     *
     * @param[in] ch     Channel information to use.
     * @param[in] nch    Number of channels.
     * @param[in] acc    Required accuracy.
     * @param[in] t      Transform the points using this.
     *
     * @return   The created meg coil set.
     */
    FwdCoilSet::UPtr create_meg_coils(const QList<FIFFLIB::FiffChInfo>& chs,
                                 int nch,
                                 int acc,
                                 const FIFFLIB::FiffCoordTrans& t = FIFFLIB::FiffCoordTrans());

    //=========================================================================================================
    /**
     * Create a EEG coil set definition using a channel information
     * Change the coordinate frame if so desired
     *
     * @param[in] ch     Channel information to use.
     * @param[in] nch    Number of channels.
     * @param[in] t      Transform the points using this.
     *
     * @return   The created meg coil set.
     */
    static FwdCoilSet::UPtr create_eeg_els(const QList<FIFFLIB::FiffChInfo>& chs,
                                      int nch,
                                      const FIFFLIB::FiffCoordTrans& t = FIFFLIB::FiffCoordTrans());

    //=========================================================================================================
    /**
     * Read a coil definitions from file
     *
     * @param[in] name   File name to read from.
     *
     * @return   The read meg coil set.
     */
    static FwdCoilSet::UPtr read_coil_defs(const QString& name);

    //=========================================================================================================
    /**
     * Make a coil set duplicate
     *
     * @param[in] t      Transformation which should be applied to the duplicate.
     *
     * @return   The duplicated coil set.
     */
    FwdCoilSet::UPtr dup_coil_set(const FIFFLIB::FiffCoordTrans& t = FIFFLIB::FiffCoordTrans()) const;

    //=========================================================================================================
    /**
     * Checks if a set of templates contains a planar coil of a specified type.
     *
     * @param[in] type   This is the coil type we are interested in.
     *
     * @return   True if set contains a planar coil of specified type, false otherwise.
     */
    bool is_planar_coil_type(int type) const;

    //=========================================================================================================
    /**
     * Checks if a set of templates contains an axial coil of a specified type.
     *
     * @param[in] type   This is the coil type we are interested in.
     *
     * @return   True if set contains an axial coil of specified type, false otherwise.
     */
    bool is_axial_coil_type(int type) const;

    //=========================================================================================================
    /**
     * Checks if a set of templates contains a magnetometer of a specified type.
     *
     * @param[in] type   This is the coil type we are interested in.
     *
     * @return   True if set contains a magnetometer of specified type, false otherwise.
     */
    bool is_magnetometer_coil_type(int type) const;

    //=========================================================================================================
    /**
     * Checks if a set of templates contains an EEG electrode of a specified type.
     *
     * @param[in] type   This is the coil type we are interested in.
     *
     * @return   True if set contains an EEG electrode of specified type, false otherwise.
     */
    bool is_eeg_electrode_type(int type) const;

private:
    //=========================================================================================================
    /**
     * Add a new coil template to this set.
     *
     * @param[in] type            Coil type identifier.
     * @param[in] coil_class      Coil class (MAG, AXIAL_GRAD, PLANAR_GRAD, etc.).
     * @param[in] acc             Accuracy level.
     * @param[in] np              Number of integration points.
     * @param[in] size            Coil size.
     * @param[in] base            Baseline.
     * @param[in] desc            Human-readable description.
     *
     * @return Non-owning pointer to the newly created coil, or nullptr on error.
     */
    FwdCoil* fwd_add_coil_to_set(int type, int coil_class, int acc, int np, float size, float base, const QString& desc);

public:
    std::vector<FwdCoil::UPtr> coils;  /**< The coil or electrode positions. */
    int     coord_frame;            /**< Common coordinate frame. */
    std::unique_ptr<FwdBemSolution> user_data;  /**< Coil-specific BEM solution. */

    /** Number of coils (convenience accessor). */
    inline int ncoil() const { return static_cast<int>(coils.size()); }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FWDLIB

#endif // FWD_COIL_SET_H
