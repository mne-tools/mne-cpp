//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fs_surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Seok Lew <slew@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Reader and in-memory representation of a single FreeSurfer triangular surface (e.g. lh.pial, rh.white, lh.inflated).
 *
 * A FreeSurfer surface file stores one cortical hemisphere as a closed
 * triangular mesh: a list of vertex positions in Tk-surface RAS millimetres
 * followed by a list of triangle faces given as indices into that vertex
 * array. The reader handles the two binary container variants emitted by
 * FreeSurfer’s @c MRISwrite():
 *
 * - The legacy quadrangle format @c QUAD_FILE_MAGIC_NUMBER (0xFFFFFE), whose
 *   quad faces are split into two triangles on load.
 * - The modern triangle format @c TRIANGLE_FILE_MAGIC_NUMBER (0xFFFFFE)
 *   produced by current versions of @c mris_convert and @c recon-all, with
 *   a textual @c created by / @c filename header line preceding the
 *   little-endian 24-bit magic.
 *
 * Magic numbers, file layout and the implicit big-endian byte order all
 * follow @c utils/mrisurf_io.c (in particular @c mrisReadTriangleFile and
 * @c mrisReadQuadFile) and the public reference in
 * @c freesurfer/utils/read_surf.c. Coordinates are returned in millimetres
 * in the FreeSurfer Tk-surface RAS frame; conversion to scanner RAS
 * requires the @c vox2ras-tkr / @c vox2ras-scanner transform pair from the
 * subject’s @c orig.mgz and is intentionally left to callers.
 *
 * The class also accepts the @c subject_id + @c hemi + @c surf shorthand
 * used throughout the MNE-Python and mne-c stack and resolves it against
 * `$SUBJECTS_DIR/<id>/surf/{lh|rh}.<surf>`, so existing analysis scripts
 * port across without path rewrites.
 */

#ifndef FS_SURFACE_H
#define FS_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include <base/byte_stream.h>

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
 * @brief In-memory FreeSurfer triangular cortical surface for one hemisphere.
 *
 * Owns the vertex coordinate matrix (n_vertices × 3, Tk-surface RAS mm), the
 * triangle index matrix (n_faces × 3, zero-based into the vertex array) and
 * the hemisphere id (0 = @c lh, 1 = @c rh). Construction is purely an I/O
 * operation: each constructor either takes a direct path to a @c lh.pial /
 * @c rh.white / @c lh.inflated style binary surface file, or the
 * @c subject_id / @c hemi / @c surf shorthand resolved against
 * @c $SUBJECTS_DIR. The instance is intended to be paired with an
 * @ref FsAnnotation or @ref FsLabel sharing the same vertex indexing.
 *
 * @snippet ex_fs/main.cpp fs_surface_read
 */
class FSSHARED_EXPORT FsSurface
{
public:
    //=========================================================================================================
    /**
     * Default constructor
     */
    FsSurface();

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] p_sFile    FsSurface file name with path.
     */
    explicit FsSurface(const std::filesystem::path& p_sFile);

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit FsSurface(const std::string& subject_id, std::int32_t hemi, const std::string& surf, const std::filesystem::path& subjects_dir);

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     */
    explicit FsSurface(const std::filesystem::path& path, std::int32_t hemi, const std::string& surf);

    //=========================================================================================================
    /**
     * Destroys the FsSurface class.
     */
    ~FsSurface();

    //=========================================================================================================
    /**
     * Initializes the FsSurface.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns the hemisphere id (0 = lh; 1 = rh)
     *
     * @return hemisphere id.
     */
    inline std::int32_t hemi() const;

    //=========================================================================================================
    /**
     * Returns whether FsSurface is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Loaded surface (eg. inflated, orig ...)
     *
     * @return the surface.
     */
    inline const std::string& surf() const;

    //=========================================================================================================
    /**
     * mne_read_surface
     *
     * Reads a FreeSurfer surface file
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[in] subjects_dir       Subjects directory.
     * @param[out] p_Surface         The read surface.
     * @param[in] p_bLoadCurvature   True if the curvature should be read (optional, default = true).
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const std::string& subject_id, std::int32_t hemi, const std::string& surf, const std::filesystem::path& subjects_dir, FsSurface& p_Surface, bool p_bLoadCurvature = true);

    //=========================================================================================================
    /**
     * mne_read_surface
     *
     * Reads a FreeSurfer surface file
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[out] p_Surface         The read surface.
     * @param[in] p_bLoadCurvature   True if the curvature should be read (optional, default = true).
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const std::filesystem::path& path, std::int32_t hemi, const std::string& surf, FsSurface& p_Surface, bool p_bLoadCurvature = true);

    //=========================================================================================================
    /**
     * mne_read_surface
     *
     * Reads a FreeSurfer surface file
     *
     * @param[in] p_sFileName        The file to read.
     * @param[out] p_Surface         The read surface.
     * @param[in] p_bLoadCurvature   True if the curvature should be read (optional, default = true).
     *
     * @return true if read sucessful, false otherwise.
     */
    static bool read(const std::filesystem::path& p_sFileName, FsSurface& p_Surface, bool p_bLoadCurvature = true);

    /** Vertices and triangles of a FreeSurfer surface file. */
    struct Geometry
    {
        Eigen::MatrixX3f rr;   /**< Vertex positions in meters. */
        Eigen::MatrixX3i tris; /**< Triangles, zero-based vertex indices. */
    };

    //=========================================================================================================
    /**
     * Reads the vertices and triangles of any FreeSurfer surface file (triangle or quad format), like
     * nibabel.freesurfer.read_geometry but in meters; the file name need not name a hemisphere, so BEM
     * surfaces such as inner_skull.surf can be read too.
     *
     * @param[in] file   The surface file.
     *
     * @return The geometry; an Io error if the file cannot be read, a Format error if it is not a valid surface.
     */
    static mne::Expected<Geometry> read_geometry(const std::filesystem::path& file);

    //=========================================================================================================
    /**
     * reads a binary curvature file into a vector
     *
     * @param[in] p_sFileName    Path to the FreeSurfer curvature file (e.g. lh.curv).
     *
     * @return the read curvature.
     */
    static Eigen::VectorXf read_curv(const std::filesystem::path& p_sFileName);

    //=========================================================================================================
    /**
     * Efficiently compute vertex normals for triangulated surface
     *
     * @param[in] rr     Vertex coordinates in meters.
     * @param[out] tris  The triangle descriptions.
     *
     * @return The computed normals.
     */
    static Eigen::MatrixX3f compute_normals(const Eigen::MatrixX3f& rr, const Eigen::MatrixX3i& tris);

    //=========================================================================================================
    /**
     * Coordinates of vertices (rr)
     *
     * @return coordinates of vertices.
     */
    inline const Eigen::MatrixX3f& rr() const;

    //=========================================================================================================
    /**
     * The triangle descriptions
     *
     * @return triangle descriptions.
     */
    inline const Eigen::MatrixX3i& tris() const;

    //=========================================================================================================
    /**
     * Normalized surface normals for each vertex
     *
     * @return surface normals.
     */
    inline const Eigen::MatrixX3f& nn() const;

    //=========================================================================================================
    /**
     * FreeSurfer curvature
     *
     * @return the FreeSurfer curvature data.
     */
    inline const Eigen::VectorXf& curv() const;

    //=========================================================================================================
    /**
     * Vector offset
     *
     * @return the offset vector.
     */
    inline const Eigen::Vector3f& offset() const;

    //=========================================================================================================
    /**
     * Vector offset
     *
     * @return the offset vector.
     */
    inline Eigen::Vector3f& offset();

    //=========================================================================================================
    /**
     * path to surf directuryt
     *
     * @return the path to surf directory.
     */
    inline const std::string& filePath() const;

    //=========================================================================================================
    /**
     * surf file name
     *
     * @return the surf file name.
     */
    inline const std::string& fileName() const;

    //=========================================================================================================
    /**
     * Reads a big-endian 3-byte integer (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @return the read 3-byte integer.
     */
    static std::int32_t fread3(mne::ByteStream& stream);

    //=========================================================================================================
    /**
     * Reads multiple big-endian 3-byte integers (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @param[in] count   Number of elements to read.
     * @return the read 3-byte integers.
     */
    static Eigen::VectorXi fread3_many(mne::ByteStream& stream, std::int32_t count);

private:
    std::string m_sFilePath;    /**< Path to surf directory. */
    std::string m_sFileName;    /**< FsSurface file name. */
    std::int32_t m_iHemi;       /**< Hemisphere (lh = 0; rh = 1). */
    std::string m_sSurf;        /**< Loaded surface (eg. inflated, orig ...). */
    Eigen::MatrixX3f m_matRR;   /**< alias verts. Vertex coordinates in meters. */
    Eigen::MatrixX3i m_matTris; /**< alias faces. The triangle descriptions. */
    Eigen::MatrixX3f m_matNN;   /**< Normalized surface normals for each vertex. -> not needed since qglbuilder is doing that for us. */
    Eigen::VectorXf m_vecCurv;  /**< FreeSurfer curvature data. */

    Eigen::Vector3f m_vecOffset; /**< FsSurface offset. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FsSurface::hemi() const
{
    return m_iHemi;
}

//=============================================================================================================

inline bool FsSurface::empty() const
{
    return m_iHemi == -1;
}

//=============================================================================================================

inline const std::string& FsSurface::surf() const
{
    return m_sSurf;
}

//=============================================================================================================

inline const Eigen::MatrixX3f& FsSurface::rr() const
{
    return m_matRR;
}

//=============================================================================================================

inline const Eigen::MatrixX3i& FsSurface::tris() const
{
    return m_matTris;
}

//=============================================================================================================

inline const Eigen::MatrixX3f& FsSurface::nn() const
{
    return m_matNN;
}

//=============================================================================================================

inline const Eigen::VectorXf& FsSurface::curv() const
{
    return m_vecCurv;
}

//=============================================================================================================

inline const Eigen::Vector3f& FsSurface::offset() const
{
    return m_vecOffset;
}

//=============================================================================================================

inline Eigen::Vector3f& FsSurface::offset()
{
    return m_vecOffset;
}

//=============================================================================================================

inline const std::string& FsSurface::filePath() const
{
    return m_sFilePath;
}

//=============================================================================================================

inline const std::string& FsSurface::fileName() const
{
    return m_sFileName;
}
} // NAMESPACE

#endif // FS_SURFACE_H
