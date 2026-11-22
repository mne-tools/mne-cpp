//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_colortable.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December 2012
 * @brief    In-memory representation of a FreeSurfer colour/structure lookup table (FreeSurferColorLUT / embedded .annot ctab).
 *
 * FreeSurfer ships a global colour lookup table at
 * @c $FREESURFER_HOME/FreeSurferColorLUT.txt that maps integer labels
 * (cortical parcels, subcortical structures, white-matter parcels, …) to a
 * human-readable name and an RGBA tuple used by @c tkmedit, @c freeview
 * and downstream tooling. The same table is also embedded directly inside
 * @c .annot files via the @c TAG_OLD_COLORTABLE and @c TAG_NEW_COLORTABLE
 * blocks parsed by @ref mne::FsAnnotation.
 *
 * This class is the binary-friendly in-memory form of either source:
 * - @c struct_names — region name per entry, indexed by row
 * - @c table         — @c numEntries × 5 matrix of
 *   @c [R, G, B, A, packed_label] where @c packed_label is the same
 *   @c R + (G ≪ 8) + (B ≪ 16) + (A ≪ 24) integer used to encode
 *   per-vertex assignments in @c .annot files
 * - @c orig_tab      — atlas/table provenance string (e.g.
 *   @c "aparc.annot.ctab") preserved from the source
 */

#ifndef FS_COLORTABLE_H
#define FS_COLORTABLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief FreeSurfer colour lookup table: region name + RGBA + packed label, indexed by entry.
 *
 * Backs the colour/structure mapping used by @ref FsAnnotation and by
 * any caller that needs to translate a packed @c .annot label into the
 * matching anatomical name and RGBA colour. The container is a thin
 * value object: rows of @c table align with @c struct_names, and the
 * label column matches the per-vertex integer stored in @c .annot files
 * so direct equality lookup is sufficient.
 *
 * @snippet ex_fs/main.cpp fs_annotation_read
 */
class FSSHARED_EXPORT FsColortable
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit FsColortable();

    //=========================================================================================================
    /**
     * Initializes colortable.
     */
    void clear();

    //=========================================================================================================
    /**
     * Ids encoded in the colortable
     *
     * @return ids.
     */
    inline Eigen::VectorXi getLabelIds() const;

    //=========================================================================================================
    /**
     * Names encoded in the colortable
     *
     * @return ids.
     */
    inline const mne::StringList& getNames() const;

    //=========================================================================================================
    /**
     * RGBAs encoded in the colortable
     *
     * @return RGBAs.
     */
    inline Eigen::MatrixX4i getRGBAs() const;

public:
    std::string orig_tab;         /**< FsColortable raw data. */
    std::int32_t numEntries;      /**< Number of entries. */
    mne::StringList struct_names; /**< Anatomical ROI description. */
    Eigen::MatrixXi table;        /**< labels and corresponing colorcode. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Eigen::VectorXi FsColortable::getLabelIds() const
{
    Eigen::VectorXi p_vecIds;
    if (table.cols() == 5)
        p_vecIds = table.block(0, 4, table.rows(), 1);

    return p_vecIds;
}

//=============================================================================================================

inline const mne::StringList& FsColortable::getNames() const
{
    return struct_names;
}

//=============================================================================================================

inline Eigen::MatrixX4i FsColortable::getRGBAs() const
{
    Eigen::MatrixX4i p_matRGBAs;
    if (table.cols() == 5)
        p_matRGBAs = table.block(0, 0, table.rows(), 4);

    return p_matRGBAs;
}
} // NAMESPACE

#endif // FS_COLORTABLE_H
