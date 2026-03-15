//=============================================================================================================
/**
 * @file     fwd_coil_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FwdCoilSet class declaration.
 *
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
 * Implements FwdCoilSet (Replaces *fwdCoilSet,fwdCoilSetRec; struct of MNE-C fwd_types.h).
 *
 * @brief Collection of FwdCoil objects representing a full MEG or EEG sensor array.
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
