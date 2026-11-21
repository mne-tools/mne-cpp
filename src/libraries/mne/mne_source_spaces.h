//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_source_spaces.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Container pairing the left and right cortical source spaces of a subject.
 *
 * @ref mne::MNESourceSpaces wraps the list of
 * @ref mne::MNESourceSpace objects that form a complete cortical source space
 * as written by @c mne_setup_source_space (one entry per hemisphere, or
 * additional entries for discrete / volume source spaces). It is the
 * natural input to @ref mne::MNEForwardSolution and @ref mne::MNEInverseOperator
 * and feeds the 3D rendering pipeline via @ref mne::MNEHemisphere. Provides
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
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_stream.h>
#include <base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <string_view>
#include <filesystem>
#include <memory>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FsLabel;
}

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
 * Container pairing the left and right cortical source spaces (and optional
 * discrete / volume source spaces) of a subject as written by
 * @c mne_setup_source_space.
 *
 * @brief List of @ref MNESourceSpace objects forming a subject source space.
 *
 * @snippet ex_mne_api/main.cpp mne_source_spaces_usage
 */
class MNESHARED_EXPORT MNESourceSpaces
{
public:
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
    MNESourceSpaces(const MNESourceSpaces& p_MNESourceSpaces);

    //=========================================================================================================
    /**
     * Copy assignment; like the copy constructor it copies the source spaces, it does not share them.
     *
     * @param[in] other   Object to assign from.
     *
     * @return reference to this object.
     */
    MNESourceSpaces& operator=(const MNESourceSpaces& other);

    //=========================================================================================================
    /**
     * Destroys the MNE forward solution
     */
    ~MNESourceSpaces();

    //=========================================================================================================
    /**
     * Move constructor.
     *
     * @param[in, out] other   Source spaces to move from; left empty.
     */
    MNESourceSpaces(MNESourceSpaces&& other) noexcept = default;

    //=========================================================================================================
    /**
     * Move assignment.
     *
     * @param[in, out] other   Source spaces to move from; left empty.
     *
     * @return reference to this object.
     */
    MNESourceSpaces& operator=(MNESourceSpaces&& other) noexcept = default;

    //=========================================================================================================
    /**
     * Reads a source space file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The -src.fif file (or any FIFF file holding source spaces).
     */
    explicit MNESourceSpaces(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the source spaces of a file, as mne.read_source_spaces does.
     *
     * @param[in] path       The -src.fif file (or any FIFF file holding source spaces).
     * @param[in] add_geom   Complete the geometry: triangle normals, areas and centroids, and vertex normals.
     *
     * @return The source spaces, or why the file cannot be read: Io if it cannot be opened, NotFound if it holds
     *         no source space, Format if a source space is incomplete or inconsistent.
     */
    [[nodiscard]] static mne::Expected<MNESourceSpaces> read(const std::filesystem::path& path, bool add_geom = true);

    //=========================================================================================================
    /**
     * Reads the source spaces from an open FIFF stream.
     *
     * @param[in] stream     The stream; opened here if it is not open yet.
     * @param[in] add_geom   Complete the geometry: triangle normals, areas and centroids, and vertex normals.
     *
     * @return The source spaces, or why the stream cannot be read (see read(path)).
     */
    [[nodiscard]] static mne::Expected<MNESourceSpaces> read(mne::FiffStream& stream, bool add_geom = true);

    //=========================================================================================================
    /**
     * Writes the source spaces, as mne.write_source_spaces does.
     *
     * @param[in] path   The -src.fif file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

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
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Returns the Zero based (different to MATLAB) indices of the used vertices of both hemispheres
     *
     * @return the hemisphere vertices.
     */
    std::vector<Eigen::VectorXi> get_vertno() const;

    //=========================================================================================================
    /**
     * Find vertex numbers and indices from label
     *
     * @param[in] p_label      Source space label.
     * @param[out] src_sel   array of int (idx.size() = vertno[0].size() + vertno[1].size()).
     *                       Indices of the selected vertices in sourse space
     *
     * @return vertno list of length 2 Vertex numbers for lh and rh.
     */
    std::vector<Eigen::VectorXi> label_src_vertno_sel(const mne::FsLabel& p_label, Eigen::VectorXi& src_sel) const;

    //=========================================================================================================
    /**
     * Reduces a source space to selected regions
     *
     * @param[in] p_qListLabels  ROIs.
     *
     * @return the reduced source space.
     */
    MNESourceSpaces pick_regions(const std::vector<mne::FsLabel>& p_qListLabels) const;

    //=========================================================================================================
    /**
     * Returns the number of stored hemispheres 0, 1 or 2
     *
     * @return number of stored hemispheres.
     */
    inline std::int32_t size() const;

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
    bool transform_source_space_to(mne::fiff_int_t dest,
                                   mne::FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * Writes the source space blocks into an open FIFF stream (mne write_source_spaces_to_fid).
     *
     * @param[in] stream  The stream to write to.
     */
    void write_to_stream(mne::FiffStream& stream) const;

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
    MNESourceSpace& operator[](std::int32_t idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by index.
     *
     * @param[in] idx    the source space index (0 or 1).
     *
     * @return Const reference to the source space at the given index.
     */
    const MNESourceSpace& operator[](std::int32_t idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by hemisphere identifier.
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Reference to the source space for the given hemisphere.
     */
    MNESourceSpace& operator[](std::string_view idt);

    //=========================================================================================================
    /**
     * Subscript operator [] to access source spaces by hemisphere identifier.
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return Const reference to the source space for the given hemisphere.
     */
    const MNESourceSpace& operator[](std::string_view idt) const;

    //=========================================================================================================
    /**
     * Safe downcast to MNEHemisphere at the given index.
     *
     * @param[in] idx    the source space index.
     *
     * @return Pointer to MNEHemisphere, or nullptr if the element is not a hemisphere.
     */
    MNEHemisphere* hemisphereAt(std::int32_t idx);

    //=========================================================================================================
    /**
     * Safe downcast to MNEHemisphere at the given index (const).
     *
     * @param[in] idx    the source space index.
     *
     * @return Const pointer to MNEHemisphere, or nullptr if the element is not a hemisphere.
     */
    const MNEHemisphere* hemisphereAt(std::int32_t idx) const;

    //=========================================================================================================
    /**
     * Access the underlying shared_ptr at the given index.
     *
     * @param[in] idx    the source space index.
     *
     * @return shared_ptr to the MNESourceSpace.
     */
    std::shared_ptr<MNESourceSpace>& at(std::int32_t idx);

    //=========================================================================================================
    /**
     * Access the underlying shared_ptr at the given index (const).
     *
     * @param[in] idx    the source space index.
     *
     * @return const shared_ptr to the MNESourceSpace.
     */
    const std::shared_ptr<MNESourceSpace>& at(std::int32_t idx) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand source space set.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const MNESourceSpaces& a, const MNESourceSpaces& b);

private:
    std::vector<std::shared_ptr<MNESourceSpace>> m_sourceSpaces; /**< Source spaces (typically hemispheres). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNESourceSpaces::empty() const
{
    return m_sourceSpaces.empty();
}

//=============================================================================================================

inline std::int32_t MNESourceSpaces::size() const
{
    return static_cast<std::int32_t>(m_sourceSpaces.size());
}

//=============================================================================================================

inline bool operator==(const MNESourceSpaces& a, const MNESourceSpaces& b)
{
    if (a.size() != b.size())
        return false;
    for (std::int32_t i = 0; i < a.size(); ++i) {
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
