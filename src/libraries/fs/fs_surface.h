//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
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
 * @c $SUBJECTS_DIR/<id>/surf/{lh|rh}.<surf>, so existing analysis scripts
 * port across without path rewrites.
 */

#ifndef FS_SURFACE_H
#define FS_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDataStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
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
 */
class FSSHARED_EXPORT FsSurface
{
public:
    typedef QSharedPointer<FsSurface> SPtr;            /**< Shared pointer type for FsSurface class. */
    typedef QSharedPointer<const FsSurface> ConstSPtr; /**< Const shared pointer type for FsSurface class. */
    
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
    explicit FsSurface(const QString& p_sFile);

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit FsSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir);

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] path               path to surface directory.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     *
     * @return true if read sucessful, false otherwise.
     */
    explicit FsSurface(const QString &path, qint32 hemi, const QString &surf);

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
    inline qint32 hemi() const;

    //=========================================================================================================
    /**
     * Returns whether FsSurface is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Loaded surface (eg. inflated, orig ...)
     *
     * @return the surface.
     */
    inline QString surf() const;

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
    static bool read(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, FsSurface &p_Surface, bool p_bLoadCurvature = true);

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
    static bool read(const QString &path, qint32 hemi, const QString &surf, FsSurface &p_Surface, bool p_bLoadCurvature = true);

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
    static bool read(const QString &p_sFileName, FsSurface &p_Surface, bool p_bLoadCurvature = true);

    //=========================================================================================================
    /**
     * reads a binary curvature file into a vector
     *
     * @return the read curvature.
     */
    static Eigen::VectorXf read_curv(const QString &p_sFileName);

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
    inline QString filePath() const;

    //=========================================================================================================
    /**
     * surf file name
     *
     * @return the surf file name.
     */
    inline QString fileName() const;

    //=========================================================================================================
    /**
     * Reads a 3-byte integer out of a QDataStream (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @return the read 3-byte integer.
     */
    static qint32 fread3(QDataStream &stream);

    //=========================================================================================================
    /**
     * Reads a 3-byte integer out of a std::iostream (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @return the read 3-byte integer.
     */
    static qint32 fread3(std::iostream &stream);

    //=========================================================================================================
    /**
     * Reads multiple 3-byte integers out of a QDataStream (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @param[in] count   Number of elements to read.
     * @return the read 3-byte integers.
     */
    static Eigen::VectorXi fread3_many(QDataStream &stream, qint32 count);

    //=========================================================================================================
    /**
     * Reads multiple 3-byte integers out of a std::iostream (FreeSurfer format).
     *
     * @param[in] stream  Stream to read from.
     * @param[in] count   Number of elements to read.
     * @return the read 3-byte integers.
     */
    static Eigen::VectorXi fread3_many(std::iostream &stream, qint32 count);

private:
    QString m_sFilePath;    /**< Path to surf directory. */
    QString m_sFileName;    /**< FsSurface file name. */
    qint32 m_iHemi;         /**< Hemisphere (lh = 0; rh = 1). */
    QString m_sSurf;        /**< Loaded surface (eg. inflated, orig ...). */
    Eigen::MatrixX3f m_matRR;      /**< alias verts. Vertex coordinates in meters. */
    Eigen::MatrixX3i m_matTris;    /**< alias faces. The triangle descriptions. */
    Eigen::MatrixX3f m_matNN;      /**< Normalized surface normals for each vertex. -> not needed since qglbuilder is doing that for us. */
    Eigen::VectorXf m_vecCurv;     /**< FreeSurfer curvature data. */

    Eigen::Vector3f m_vecOffset; /**< FsSurface offset. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FsSurface::hemi() const
{
    return m_iHemi;
}

//=============================================================================================================

inline bool FsSurface::isEmpty() const
{
    return m_iHemi == -1;
}

//=============================================================================================================

inline QString FsSurface::surf() const
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

inline QString FsSurface::filePath() const
{
    return m_sFilePath;
}

//=============================================================================================================

inline QString FsSurface::fileName() const
{
    return m_sFileName;
}
} // NAMESPACE

#endif // FS_SURFACE_H
