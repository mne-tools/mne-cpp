//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_label.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Reader and in-memory representation of a FreeSurfer/MNE surface label (.label).
 *
 * A FreeSurfer @c .label file describes a region of interest on a cortical
 * surface as an explicit list of vertex indices restricted to one
 * hemisphere. The file is plain ASCII: a comment line, a vertex count, then
 * one whitespace-separated row per vertex of the form
 * @c "index  x  y  z  value" where @c (x, y, z) is the Tk-surface RAS
 * position in millimetres and @c value is an optional scalar (statistic,
 * cluster weight, time, …). The format is shared with MNE-Python
 * (@c mne.Label) and with @c mri_annotation2label, which is how individual
 * regions of a parcellation are persisted as standalone labels.
 *
 * This class loads such a file, keeps the indices, positions and values
 * together with the hemisphere id and a human-readable label name, and
 * provides helpers to project the label back onto a triangulation
 * (@ref mne::FsLabel::selectTris "FsLabel::selectTris") so it can be drawn or fed into source-space
 * masking.
 */

#ifndef FS_LABEL_H
#define FS_LABEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

#include <cstdint>
#include <filesystem>
#include <string>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

const static Eigen::MatrixX3i defaultTris(0, 3);

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FsSurface;

//=============================================================================================================
/**
 * @brief A FreeSurfer/MNE surface label: per-vertex indices, Tk-RAS positions and scalar values for one hemisphere.
 *
 * Wraps the parsed contents of a @c .label file or an in-memory label
 * constructed programmatically (e.g. by thresholding a source estimate via
 * @ref FsLabelUtils::stcToLabel). Vertex indices reference the matching
 * @ref FsSurface for the same subject and hemisphere; positions are the
 * Tk-surface RAS coordinates copied from that surface; values are the
 * per-vertex scalar payload (statistic, time, weight). Hemisphere is
 * encoded with the FreeSurfer convention {0 = lh, 1 = rh}.
 *
 * @snippet ex_fs/main.cpp fs_label_read
 */
class FSSHARED_EXPORT FsLabel
{
public:
    //=========================================================================================================
    /**
     * Default constructor
     */
    FsLabel();

    //=========================================================================================================
    /**
     * Constructs a label
     *
     * @param[in] p_vertices     Vertices.
     * @param[in] p_pos          Positions.
     * @param[in] p_values       Values.
     * @param[in] p_hemi         Hemisphere (lh = 0; rh = 1).
     * @param[in] p_name         label names.
     * @param[in] p_id           label id (optional, default = -1).
     */
    FsLabel(const Eigen::VectorXi& p_vertices,
            const Eigen::MatrixX3f& p_pos,
            const Eigen::VectorXd& p_values,
            std::int32_t p_hemi, const std::string& p_name,
            std::int32_t p_id = -1);

    //=========================================================================================================
    /**
     * Destroys the FsLabel class.
     */
    ~FsLabel();

    //=========================================================================================================
    /**
     * Initializes the FsLabel.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if FsLabel is empty.
     *
     * @return true if FsLabel is empty, false otherwise.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Select tris for this label from a given surface file.
     *
     * @param[in] p_Surface      to generate the label tris from.
     *
     * @return the generated tris.
     */
    Eigen::MatrixX3i selectTris(const FsSurface& p_Surface);

    //=========================================================================================================
    /**
     * Select tris for this label from a given tri matrix.
     *
     * @param[in] p_matTris      tris from which the selection should be made.
     *
     * @return the generated tris.
     */
    Eigen::MatrixX3i selectTris(const Eigen::MatrixX3i& p_matTris);

    //=========================================================================================================
    /**
     * mne_read_label_file
     *
     * Reads a FsLabel from a FreeSurfer label file.
     * This is based on the FreeSurfer read_label routine
     * SUBJECTS_DIR environment variable is not consulted for the standard location
     *
     * @param[in] p_sFileName    label file name.
     * @param[in] p_Label        read label.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(const std::filesystem::path& p_sFileName, FsLabel& p_Label);

public:
    std::string comment;      /**< Comment from the first line of the label file. */
    Eigen::VectorXi vertices; /**< Vertex indices (0 based). */
    Eigen::MatrixX3f pos;     /**< Locations in meters. */
    Eigen::VectorXd values;   /**< Values at the vertices. */
    std::int32_t hemi;        /**< Hemisphere (lh = 0; rh = 1). */
    std::string name;         /**< Name of the label. */
    std::int32_t label_id;    /**< FsLabel id (optional). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FsLabel::empty() const
{
    return this->hemi == -1;
}
} // NAMESPACE

#endif // FS_LABEL_H
