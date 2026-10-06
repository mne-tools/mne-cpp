//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_coord_trans_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    The MNE-C transform chain from MEG head coordinates to MNI and FreeSurfer Talairach coordinates.
 *
 * C++ peer of the MNE-C @c coordTransSet that @c mne_analyze uses to report a picked
 * location in head, MRI, MNI and Talairach coordinates and that @c mne_collect_transforms
 * gathers into one file; @ref FIFFLIB::FiffCoordTransSet::headToMni matches
 * @c mne.head_to_mni.
 */

#ifndef FIFFCOORDTRANSSET_H
#define FIFFCOORDTRANSSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_coord_trans.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStream;

//=============================================================================================================
/**
 * @brief The chain of transforms from MEG head coordinates to MNI and FreeSurfer Talairach coordinates.
 *
 * Head -> surface RAS (the FreeSurfer MRI frame) comes from the coregistration, surface RAS -> scanner
 * RAS from the centre (c_ras) of the subject's MRI, scanner RAS -> MNI Talairach from
 * @c mri/transforms/talairach.xfm, and MNI -> FreeSurfer Talairach from the fixed Brett
 * approximations MNE-C uses (one matrix above and one below the AC-PC plane). Empty
 * transforms mark links that are not known.
 *
 * @snippet ex_fiff_api/main.cpp fiff_coord_trans_set_usage
 */
class FIFFSHARED_EXPORT FiffCoordTransSet
{
public:
    using SPtr = QSharedPointer<FiffCoordTransSet>;             /**< Shared pointer type for FiffCoordTransSet. */
    using ConstSPtr = QSharedPointer<const FiffCoordTransSet>;  /**< Const shared pointer type for FiffCoordTransSet. */
    using UPtr = std::unique_ptr<FiffCoordTransSet>;            /**< Unique pointer type for FiffCoordTransSet. */
    using ConstUPtr = std::unique_ptr<const FiffCoordTransSet>; /**< Const unique pointer type for FiffCoordTransSet. */

    //=========================================================================================================
    /**
     * Reads the scanner RAS -> MNI Talairach transform of a FreeSurfer/MNI @c .xfm file
     * (MNE-C read_mni_coord_transform_file).
     *
     * @param[in] path   Path to the @c .xfm file, usually @c mri/transforms/talairach.xfm in the subject directory.
     *
     * @return The transform in metres, or an empty transform if the file holds no linear transform.
     */
    static FiffCoordTrans readMniTransform(const QString& path);

    //=========================================================================================================
    /**
     * Sets the RAS -> MNI transform from a @c .xfm file and the two MNI -> FreeSurfer Talairach
     * transforms (MNE-C mne_mri_add_talairach_transforms).
     *
     * @param[in] xfmPath    Path to the @c talairach.xfm file.
     *
     * @return True if the @c .xfm file could be read.
     */
    bool addTalairach(const QString& xfmPath);

    //=========================================================================================================
    /**
     * Takes every transform of the chain stored in a FIFF file, such as a @c -trans.fif file or a
     * @c COR.fif MRI set (MNE-C mne_collect_transforms). Transforms already in the set are replaced,
     * so read the MRI set first and the coregistration last.
     *
     * @param[in] path   FIFF file path.
     *
     * @return The number of transforms taken from the file, or -1 if the file cannot be opened.
     */
    int read(const QString& path);

    //=========================================================================================================
    /**
     * Writes every non-empty transform, head -> MRI first.
     *
     * @param[in] stream     An open FIFF stream.
     */
    void write(FiffStream& stream) const;

    //=========================================================================================================
    /**
     * Maps MEG head coordinates to MNI Talairach coordinates, as @c mne.head_to_mni does.
     *
     * @param[in] rr     Points in head coordinates (metres), one per row.
     *
     * @return The points in MNI Talairach coordinates (metres), or an empty matrix if a link of the chain is missing.
     */
    Eigen::MatrixX3f headToMni(const Eigen::MatrixX3f& rr) const;

    //=========================================================================================================
    /**
     * Maps MNI Talairach coordinates to FreeSurfer Talairach coordinates, using the transform
     * for points above or below the AC-PC plane (MNI z > 0 or not).
     *
     * @param[in] rr     Points in MNI Talairach coordinates (metres), one per row.
     *
     * @return The points in FreeSurfer Talairach coordinates (metres), or an empty matrix if the transforms are missing.
     */
    Eigen::MatrixX3f mniToTalairach(const Eigen::MatrixX3f& rr) const;

    FiffCoordTrans head_surf_RAS_t;   /**< Transform from MEG head coordinates to surface RAS. */
    FiffCoordTrans surf_RAS_RAS_t;    /**< Transform from surface RAS to RAS (nonzero origin) coordinates. */
    FiffCoordTrans RAS_MNI_tal_t;     /**< Transform from RAS (nonzero origin) to MNI Talairach coordinates. */
    FiffCoordTrans MNI_tal_tal_gtz_t; /**< Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0). */
    FiffCoordTrans MNI_tal_tal_ltz_t; /**< Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FIFFLIB

#endif // FIFFCOORDTRANSSET_H
