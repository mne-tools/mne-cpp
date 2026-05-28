//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_source_spaces.h
 * @since 2026
 * @date  March 2026
 * @brief Container pairing the left and right cortical source spaces of a subject.
 *
 * @ref MNELIB::MNESourceSpaces wraps the @c QList of
 * @ref MNESourceSpace objects that form a complete cortical source space
 * as written by @c mne_setup_source_space (one entry per hemisphere, or
 * additional entries for discrete / volume source spaces). It is the
 * natural input to @ref MNEForwardSolution and @ref MNEInverseOperator
 * and feeds the 3D rendering pipeline via @ref MNEHemisphere. Provides
 * FIFF read/write at the file level so callers do not have to walk the
 * block tree by hand.
 */

#ifndef MNE_SOURCE_SPACES_H
#define MNE_SOURCE_SPACES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_hemisphere.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff.h>

#include <algorithm>
#include <vector>
#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB
{
    class FsLabel;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Container pairing the left and right cortical source spaces (and optional
 * discrete / volume source spaces) of a subject as written by
 * @c mne_setup_source_space.
 *
 * @brief List of @ref MNESourceSpace objects forming a subject source space.
 */
class MNESHARED_EXPORT MNESourceSpaces
{
public:
    using SPtr = std::shared_ptr<MNESourceSpaces>;            /**< Shared pointer type for MNESourceSpaces. */
    using ConstSPtr = std::shared_ptr<const MNESourceSpaces>; /**< Const shared pointer type for MNESourceSpaces. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNESourceSpaces();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNESourceSpaces   MNE forward solution.
     */
    MNESourceSpaces(const MNESourceSpaces &p_MNESourceSpaces);

    //=========================================================================================================
    /**
     * Destroys the MNE forward solution
     */
    ~MNESourceSpaces();

    //=========================================================================================================
    /**
     * Initializes MNE source space.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if MNE Source Space is empty.
     *
     * @return true if MNE Source Space is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_find_source_space_hemi function
     *
     * Returns the hemisphere id ( FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
     *
     * @param[in] p_Hemisphere the hemisphere to investigate.
     *
     * @return the deduced hemisphere id.
     */
    static qint32 find_source_space_hemi(MNESourceSpace& p_SourceSpace);

    //=========================================================================================================
    /**
     * Returns the Zero based (different to MATLAB) indices of the used vertices of both hemispheres
     *
     * @return the hemisphere vertices.
     */
    QList<Eigen::VectorXi> get_vertno() const;

    //=========================================================================================================
    /**
     * Find vertex numbers and indices from label
     *
     * @param[in] label      Source space label.
     * @param[out] src_sel   array of int (idx.size() = vertno[0].size() + vertno[1].size()).
     *                       Indices of the selected vertices in sourse space
     *
     * @return vertno list of length 2 Vertex numbers for lh and rh.
     */
    QList<Eigen::VectorXi> label_src_vertno_sel(const FSLIB::FsLabel &p_label, Eigen::VectorXi &src_sel) const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_patch_info function
     *
     * Generate the patch information from the 'nearest' vector in a source space. For vertex in the source
     * space it provides the list of neighboring vertices in the high resolution triangulation.
     *
     * @param[in, out] p_Hemisphere  The source space.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool patch_info(MNEHemisphere &p_Hemisphere);//VectorXi& nearest, QList<VectorXi>& pinfo);@param[in] nearest   The nearest vector of the source space.@param[in, out] pinfo    The requested patch information.

    //=========================================================================================================
    /**
     * Reduces a source space to selected regions
     *
     * @param[in] p_qListLabels  ROIs.
     *
     * @return the reduced source space.
     */
    MNESourceSpaces pick_regions(const QList<FSLIB::FsLabel> &p_qListLabels) const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_read_source_spaces function
     *
     * Reads source spaces from a fif file
     *
     * @param[in, out] p_pStream         The opened fif file.
     * @param[in] add_geom          Add geometry information to the source spaces.
     * @param[in, out] p_SourceSpace    The read source spaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool readFromStream(FIFFLIB::FiffStream::SPtr& p_pStream,
                               bool add_geom,
                               MNESourceSpaces& p_SourceSpace);

    //=========================================================================================================
    /**
     * Returns the number of stored hemispheres 0, 1 or 2
     *
     * @return number of stored hemispheres.
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_transform_source_space_to function
     * Wrapper for the MNESourceSpaces transform_source_space_to member function
     *
     * Note: In difference to mne-matlab this is not a static function. This is a method of the MNESourceSpaces
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * Transforms source space data to the desired coordinate system
     *
     * @param[in] dest destination check code.
     * @param[in] trans transformation information.
     *
     * @return true if succeeded, false otherwise.
     */
    bool transform_source_space_to(FIFFLIB::fiff_int_t dest,
                                   FIFFLIB::FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the write_source_spaces_to_fid function
     *
     * Write the source spaces to a FIF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Appends a deep copy of the given source space.
     *
     * @param[in] space  The source space to append.
     */
    void append(const MNESourceSpace& space);

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by index.
     *
     * @param[in] idx    the source space index (0 or 1).
     *
     * @return Reference to the source space at the given index.
     */
    MNESourceSpace& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by index.
     *
     * @param[in] idx    the source space index (0 or 1).
     *
     * @return Const reference to the source space at the given index.
     */
    const MNESourceSpace& operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by hemisphere identifier.
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Reference to the source space for the given hemisphere.
     */
    MNESourceSpace& operator[] (QString idt);

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by hemisphere identifier.
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Const reference to the source space for the given hemisphere.
     */
    const MNESourceSpace& operator[] (QString idt) const;

    //=========================================================================================================
    /**
     * Safe downcast to MNEHemisphere at the given index.
     *
     * @param[in] idx    the source space index.
     *
     * @return Pointer to MNEHemisphere, or nullptr if the element is not a hemisphere.
     */
    MNEHemisphere* hemisphereAt(qint32 idx);

    //=========================================================================================================
    /**
     * Safe downcast to MNEHemisphere at the given index (const).
     *
     * @param[in] idx    the source space index.
     *
     * @return Const pointer to MNEHemisphere, or nullptr if the element is not a hemisphere.
     */
    const MNEHemisphere* hemisphereAt(qint32 idx) const;

    //=========================================================================================================
    /**
     * Access the underlying shared_ptr at the given index.
     *
     * @param[in] idx    the source space index.
     *
     * @return shared_ptr to the MNESourceSpace.
     */
    std::shared_ptr<MNESourceSpace>& at(qint32 idx);

    //=========================================================================================================
    /**
     * Access the underlying shared_ptr at the given index (const).
     *
     * @param[in] idx    the source space index.
     *
     * @return const shared_ptr to the MNESourceSpace.
     */
    const std::shared_ptr<MNESourceSpace>& at(qint32 idx) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const MNESourceSpaces &a, const MNESourceSpaces &b);

private:

    //=========================================================================================================
    /**
     * Definition of the complete_source_space_info function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
     *
     * Completes triangulation info
     *
     * @param[in, out] p_pHemisphere   Hemisphere to be completed.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool complete_source_space_info(MNEHemisphere& p_Hemisphere);

    //=========================================================================================================
    /**
     * Definition of the read_source_space function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
     *
     * Reads a single source space (hemisphere)
     *
     * @param[in] p_pStream         The opened fif file.
     * @param[in] p_Tree            Search for the source space here.
     * @param[in, out] p_pHemisphere    The read source space (hemisphere).
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_source_space(FIFFLIB::FiffStream::SPtr& p_pStream,
                                  const FIFFLIB::FiffDirNode::SPtr& p_Tree,
                                  MNEHemisphere& p_Hemisphere);

private:
    std::vector<std::shared_ptr<MNESourceSpace>> m_sourceSpaces;    /**< Source spaces (typically hemispheres). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNESourceSpaces::isEmpty() const
{
    return m_sourceSpaces.empty();
}

//=============================================================================================================

inline qint32 MNESourceSpaces::size() const
{
    return static_cast<qint32>(m_sourceSpaces.size());
}

//=============================================================================================================

inline bool operator== (const MNESourceSpaces &a, const MNESourceSpaces &b)
{
    if (a.size() != b.size())
        return false;
    for (qint32 i = 0; i < a.size(); ++i) {
        // Compare as hemispheres if both elements are hemispheres
        auto* ha = dynamic_cast<const MNEHemisphere*>(&a[i]);
        auto* hb = dynamic_cast<const MNEHemisphere*>(&b[i]);
        if (ha && hb) {
            if (!(*ha == *hb))
                return false;
        } else {
            // Fallback: compare base-class identity (pointer equality or basic fields)
            // For now, require both to be hemispheres for equality
            return false;
        }
    }
    return true;
}

} // NAMESPACE

#endif // MNE_SOURCE_SPACES_H
