//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_morph_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Sphere-registration morph map of one hemisphere: the sparse matrix taking vertex values from one subject to another.
 *
 * @ref mne::MNEMorphMap is the C++ counterpart of MNE-C's morph maps
 * (@c mne_morph_maps.c) and of @c mne.read_morph_map. Each vertex of the
 * destination @c sphere.reg is located on the source @c sphere.reg: the source
 * triangle it falls into gives three linear interpolation weights. Maps are
 * stored in <tt>$SUBJECTS_DIR/morph-maps/&lt;from&gt;-&lt;to&gt;-morph.fif</tt>, one
 * @c FIFFB_MNE_MORPH_MAP block per hemisphere and direction.
 */

#ifndef MNEMORPHMAP_H
#define MNEMORPHMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "fiff/fiff_sparse_matrix.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <optional>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Morph map of one hemisphere from one subject to another.
 *
 * @snippet ex_mne_api/main.cpp mne_morph_map_usage
 */
class MNESHARED_EXPORT MNEMorphMap
{
public:
    MNEMorphMap() = default;
    MNEMorphMap(MNEMorphMap&&) = default;
    MNEMorphMap& operator=(MNEMorphMap&&) = default;
    ~MNEMorphMap() = default;

    //=========================================================================================================
    /**
     * Copies the map including its matrix.
     *
     * @param[in] other   The map to copy.
     */
    MNEMorphMap(const MNEMorphMap& other);

    //=========================================================================================================
    /**
     * Computes the map between two registered spheres (MNE-C @c mne_compose_morph_maps, accurate mode;
     * @c mne.surface._make_morph_map_hemi).
     *
     * Both spheres are scaled to unit radius. Each destination vertex is mapped to the source
     * triangle it falls into, among the triangles around its nearest source vertex, with
     * barycentric weights; best holds that nearest vertex.
     *
     * @param[in] fromRr     Source sphere vertices.
     * @param[in] fromTris   Source sphere triangles.
     * @param[in] toRr       Destination sphere vertices.
     *
     * @return The map (destination vertices x source vertices).
     */
    static MNEMorphMap compute(const Eigen::MatrixX3f& fromRr, const Eigen::MatrixX3i& fromTris, const Eigen::MatrixX3f& toRr);

    //=========================================================================================================
    /**
     * Reads one map from a morph-map file (MNE-C @c load_one_morph_map).
     *
     * @param[in] path      A @c -morph.fif file.
     * @param[in] fromSubj  Source subject.
     * @param[in] toSubj    Destination subject.
     * @param[in] hemi      0 for the left, 1 for the right hemisphere.
     *
     * @return The map, or no value if the file has no such map.
     */
    static std::optional<MNEMorphMap> read(const std::filesystem::path& path, const std::string& fromSubj, const std::string& toSubj, int hemi);

    //=========================================================================================================
    /**
     * Writes maps as @c FIFFB_MNE_MORPH_MAP blocks (MNE-C @c mne_save_morph_map_matrices).
     *
     * @param[in] path   The output file.
     * @param[in] maps   The maps; from_subj, to_subj and hemi identify each one.
     *
     * @return True if the file was written.
     */
    static bool write(const std::filesystem::path& path, const std::vector<const MNEMorphMap*>& maps);

    //=========================================================================================================
    /**
     * Returns the map as a matrix that takes source vertex values to destination vertices.
     *
     * @return The destination x source sparse matrix in double precision (e.g. for SourceMorph).
     */
    Eigen::SparseMatrix<double> toEigen() const;

public:
    std::unique_ptr<mne::FiffSparseMatrix> map; /**< Destination x source interpolation weights. */
    Eigen::VectorXi best;                       /**< Nearest source vertex of every destination vertex (computed maps only). */
    int hemi = -1;                              /**< 0 for the left, 1 for the right hemisphere, -1 if unknown. */
    std::string from_subj;                      /**< Source subject. */
    std::string to_subj;                        /**< Destination subject. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMORPHMAP_H
