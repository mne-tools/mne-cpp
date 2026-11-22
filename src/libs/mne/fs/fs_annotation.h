//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_annotation.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Reader for FreeSurfer per-vertex annotation (parcellation) files such as lh.aparc.annot.
 *
 * A FreeSurfer @c .annot file assigns one parcellation label to each surface
 * vertex and ships the colour-lookup table needed to render it. It is the
 * output of @c mris_ca_label / @c mri_annotation2label and is produced by
 * @c recon-all for every standard atlas (Desikan-Killiany @c aparc, Destrieux
 * @c aparc.a2009s, @c aparc.DKTatlas40, Brodmann @c BA, …). The file is
 * big-endian and has the layout:
 *
 * - @c int32 num_vertices
 * - @c num_vertices × (@c int32 vertex_index, @c int32 packed_rgba_label)
 * - optional @c TAG_OLD_COLORTABLE / @c TAG_NEW_COLORTABLE block carrying the
 *   embedded colortable (struct names + RGBA), parsed into an
 *   @ref mne::FsColortable
 *
 * The per-vertex label is the @c int32 obtained by packing
 * @c R + (G ≪ 8) + (B ≪ 16) + (A ≪ 24) of the assigned region’s
 * colortable entry; reverse mapping from packed value to colortable row
 * yields the anatomical name. Format reference: @c utils/read_annotation.c
 * in FreeSurfer.
 *
 * This class loads one hemisphere; combine two instances via
 * @ref mne::FsAnnotationSet for whole-brain parcellations.
 */

#ifndef FS_ANNOTATION_H
#define FS_ANNOTATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "fs_colortable.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FsLabel;
class FsSurface;

//=============================================================================================================
/**
 * @brief Single-hemisphere FreeSurfer parcellation: vertex → region label plus embedded colortable.
 *
 * Holds the parsed contents of one @c .annot file: the dense per-vertex
 * label assignment, the explicit vertex index list it was authored against,
 * and the @ref FsColortable describing the colour and name of every region.
 * Indexing is aligned with the matching @ref FsSurface for the same
 * subject + hemisphere, so @c label[v] is the region of surface vertex @c v.
 *
 * @snippet ex_fs/main.cpp fs_annotation_read
 */
class FSSHARED_EXPORT FsAnnotation
{
public:
    //=========================================================================================================
    /**
     * Default constructor
     */
    FsAnnotation();

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] p_sFileName    FsAnnotation file.
     */
    explicit FsAnnotation(const std::filesystem::path& p_sFileName);

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit FsAnnotation(const std::string& subject_id, std::int32_t hemi, const std::string& surf, const std::filesystem::path& subjects_dir);

    //=========================================================================================================
    /**
     * Construts the annotation by reading it of the given file.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     */
    explicit FsAnnotation(const std::filesystem::path& path, std::int32_t hemi, const std::string& surf);

    //=========================================================================================================
    /**
     * Destroys the annotation.
     */
    ~FsAnnotation();

    //=========================================================================================================
    /**
     * Initializes the FsAnnotation.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns whether FsAnnotation is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Returns the hemisphere id (0 = lh; 1 = rh)
     *
     * @return hemisphere id.
     */
    inline std::int32_t hemi() const;

    //=========================================================================================================
    /**
     * Returns the vertix indeces
     *
     * @return vertix indeces.
     */
    inline Eigen::VectorXi& getVertices();

    //=========================================================================================================
    /**
     * Returns the vertix indeces
     *
     * @return vertix indeces.
     */
    inline const Eigen::VectorXi getVertices() const;

    //=========================================================================================================
    /**
     * Returns the vertix labels
     *
     * @return vertix labels.
     */
    inline Eigen::VectorXi& getLabelIds();

    //=========================================================================================================
    /**
     * Returns the vertix labels
     *
     * @return vertix labels.
     */
    inline const Eigen::VectorXi getLabelIds() const;

    //=========================================================================================================
    /**
     * Returns the coloratable containing the label based nomenclature
     *
     * @return colortable.
     */
    inline FsColortable& getColortable();

    //=========================================================================================================
    /**
     * Returns the coloratable containing the label based nomenclature
     *
     * @return colortable.
     */
    inline const FsColortable getColortable() const;

    //=========================================================================================================
    /**
     * Reads a FreeSurfer annotation file
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[in] subjects_dir       Subjects directory.
     * @param[out] p_Annotation      The read annotation.
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const std::string& subject_id, std::int32_t hemi, const std::string& atlas, const std::filesystem::path& subjects_dir, FsAnnotation& p_Annotation);

    //=========================================================================================================
    /**
     * Reads a FreeSurfer annotation file
     *
     * @param[in] path               path to label directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...).
     * @param[out] p_Annotation      The read annotation.
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const std::filesystem::path& path, std::int32_t hemi, const std::string& atlas, FsAnnotation& p_Annotation);

    //=========================================================================================================
    /**
     * Reads an annotation of a file
     *
     * @param[in] p_sFileName    FsAnnotation file.
     * @param[out] p_Annotation  the read annotation.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(const std::filesystem::path& p_sFileName, FsAnnotation& p_Annotation);

    //=========================================================================================================
    /**
     * python labels_from_parc
     *
     * Converts annotation to a label list and colortable, like mne.read_labels_from_annot: one label
     * per region with vertices, named "<region>-lh|rh", values 1, sorted by name.
     *
     * @param[in] p_surf                 the surface to read the vertex positions from.
     * @param[out] p_qListLabels         the converted labels are appended to a given list. Stored data are not affected.
     * @param[out] p_qListLabelRGBAs     the converted label RGBAs are appended to a given list. Stored data are not affected.
     * @param[out] lLabelPicks           the label names which should be picked.
     *
     * @return true if successful, false otherwise.
     */
    bool toLabels(const FsSurface& p_surf,
                  std::vector<FsLabel>& p_qListLabels,
                  std::vector<Eigen::RowVector4i>& p_qListLabelRGBAs,
                  const mne::StringList& lLabelPicks = {}) const;

    //=========================================================================================================
    /**
     * annotation file path
     *
     * @return the surf file path.
     */
    inline const std::string& filePath() const;

    //=========================================================================================================
    /**
     * annotation file name
     *
     * @return the surf file name.
     */
    inline const std::string& fileName() const;

private:
    std::string m_sFileName; /**< FsAnnotation file name. */
    std::string m_sFilePath; /**< FsAnnotation file path. */

    std::int32_t m_iHemi;       /**< Hemisphere (lh = 0; rh = 1). */
    Eigen::VectorXi m_Vertices; /**< Vertice indeces. */
    Eigen::VectorXi m_LabelIds; /**< Vertice label ids. */

    FsColortable m_Colortable; /**< Lookup table label colors & ids. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FsAnnotation::hemi() const
{
    return m_iHemi;
}

//=============================================================================================================

inline bool FsAnnotation::empty() const
{
    return m_iHemi == -1;
}

//=============================================================================================================

inline Eigen::VectorXi& FsAnnotation::getVertices()
{
    return m_Vertices;
}

//=============================================================================================================

inline const Eigen::VectorXi FsAnnotation::getVertices() const
{
    return m_Vertices;
}

//=============================================================================================================

inline Eigen::VectorXi& FsAnnotation::getLabelIds()
{
    return m_LabelIds;
}

//=============================================================================================================

inline const Eigen::VectorXi FsAnnotation::getLabelIds() const
{
    return m_LabelIds;
}

//=============================================================================================================

inline FsColortable& FsAnnotation::getColortable()
{
    return m_Colortable;
}

//=============================================================================================================

inline const FsColortable FsAnnotation::getColortable() const
{
    return m_Colortable;
}

//=============================================================================================================

inline const std::string& FsAnnotation::filePath() const
{
    return m_sFilePath;
}

//=============================================================================================================

inline const std::string& FsAnnotation::fileName() const
{
    return m_sFileName;
}
} // NAMESPACE

#endif // FS_ANNOTATION_H
