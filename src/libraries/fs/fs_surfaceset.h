//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_surfaceset.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Bi-hemispheric grouping of FreeSurfer surfaces (lh + rh) loaded as a single object.
 *
 * Cortical analyses in FreeSurfer/MNE typically operate on a pair of
 * hemisphere surfaces of the same kind (both @c pial, both @c white, both
 * @c inflated, both @c sphere …) read from
 * `$SUBJECTS_DIR/<id>/surf/lh.<surf>` and the matching `rh.<surf>`.
 * @ref mne::FsSurfaceSet wraps that pair as a single in-memory object so
 * downstream code (annotations, source spaces, BEM, rendering) can iterate
 * over @c {lh, rh} without re-parsing or re-resolving paths.
 *
 * The class accepts the same three locator forms as @ref mne::FsSurface "FsSurface":
 * subject + hemi + surf resolved against @c $SUBJECTS_DIR, a direct
 * directory + hemi + surf path, or explicit @c lh / rh file paths.
 * The @c hemi argument follows the FreeSurfer/MNE convention
 * {0 = lh, 1 = rh, 2 = both}; only the requested hemispheres are loaded.
 */

#ifndef FS_SURFACESET_H
#define FS_SURFACESET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "fs_surface.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Container holding the lh and/or rh @ref FsSurface for one subject and one surface kind.
 *
 * The set is keyed on the hemisphere id (0 = lh, 1 = rh) so callers can do
 * @c set[0] / @c set[1] or iterate over @c set.data(). All member surfaces
 * are loaded with the same @c surf name (e.g. @c pial), so the set is
 * homogeneous across hemispheres and stays aligned with a paired
 * @ref FsAnnotationSet sharing the same vertex indexing.
 *
 * @snippet ex_fs/main.cpp fs_surface_set_read
 */
class FSSHARED_EXPORT FsSurfaceSet
{
public:
    //=========================================================================================================
    /**
     * Default constructor
     */
    FsSurfaceSet();

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit FsSurfaceSet(const std::string& subject_id, std::int32_t hemi, const std::string& surf, const std::filesystem::path& subjects_dir);

    //=========================================================================================================
    /**
     * Construts the surface set by reading it of the given files.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     */
    explicit FsSurfaceSet(const std::filesystem::path& path, std::int32_t hemi, const std::string& surf);

    //=========================================================================================================
    /**
     * Constructs a surface set by assembling given surfaces
     *
     * @param[in] p_LHSurface    Left hemisphere surface.
     * @param[in] p_RHSurface    Right hemisphere surface.
     */
    explicit FsSurfaceSet(const FsSurface& p_LHSurface, const FsSurface& p_RHSurface);

    //=========================================================================================================
    /**
     * Constructs an annotation set by reading from annotation files
     *
     * @param[in] p_sLHFileName  Left hemisphere annotation file.
     * @param[in] p_sRHFileName  Right hemisphere annotation file.
     */
    explicit FsSurfaceSet(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName);

    //=========================================================================================================
    /**
     * Destroys the FsSurfaceSet class.
     */
    ~FsSurfaceSet();

    //=========================================================================================================
    /**
     * Initializes the FsAnnotationSet.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns The surface set map
     *
     * @return the surface set map.
     */
    inline std::map<int, FsSurface>& data();

    //=========================================================================================================
    /**
     * True if FsSurfaceSet is empty.
     *
     * @return true if FsSurfaceSet is empty.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Insert a surface
     *
     * @param[in] p_Surface  FsSurface to insert.
     */
    void insert(const FsSurface& p_Surface);

    //=========================================================================================================
    /**
     * Reads different surface files and assembles them to a FsSurfaceSet
     *
     * @param[in] p_sLHFileName      Left hemisphere surface file.
     * @param[in] p_sRHFileName      Right hemisphere surface file.
     * @param[out] p_SurfaceSet      The read surface set.
     *
     * @return true if succesfull, false otherwise.
     */
    static bool read(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName, FsSurfaceSet& p_SurfaceSet);

    //=========================================================================================================
    /**
     * The kind of Surfaces which are held by the FsSurfaceSet (eg. inflated, orig ...)
     *
     * @return the loaded surfaces (eg. inflated, orig ...).
     */
    inline std::string surf() const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access surface by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return FsSurface related to the parameter index.
     */
    const FsSurface& operator[](std::int32_t idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access surface by index
     *
     * @param[in] idx    the hemisphere index (0 or 1).
     *
     * @return FsSurface related to the parameter index.
     */
    FsSurface& operator[](std::int32_t idx);

    //=========================================================================================================
    /**
     * Subscript operator [] to access surface by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return FsSurface related to the parameter identifier.
     */
    const FsSurface& operator[](std::string_view idt) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access surface by identifier
     *
     * @param[in] idt    the hemisphere identifier ("lh" or "rh").
     *
     * @return FsSurface related to the parameter identifier.
     */
    FsSurface& operator[](std::string_view idt);

    //=========================================================================================================
    /**
     * Returns number of loaded hemispheres
     *
     * @return number of loaded hemispheres.
     */
    inline std::int32_t size() const;

private:
    //=========================================================================================================
    /**
     * Calculates the offset between two Surfaces and sets the offset to each surface accordingly
     */
    void calcOffset();

    std::map<int, FsSurface> m_surfs; /**< Hemisphere surfaces (lh = 0; rh = 1). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::map<int, FsSurface>& FsSurfaceSet::data()
{
    return m_surfs;
}

//=============================================================================================================

inline bool FsSurfaceSet::empty() const
{
    return m_surfs.empty();
}

//=============================================================================================================

inline std::string FsSurfaceSet::surf() const
{
    return m_surfs.empty() ? std::string() : m_surfs.begin()->second.surf();
}

//=============================================================================================================

inline std::int32_t FsSurfaceSet::size() const
{
    return static_cast<std::int32_t>(m_surfs.size());
}
} // NAMESPACE

#endif // FS_SURFACESET_H
