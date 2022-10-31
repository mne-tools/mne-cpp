//=============================================================================================================
/**
 * @file     surface.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Surface class declaration
 *
 */

#ifndef SURFACE_H
#define SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

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
 * A FreeSurfer surface mesh in triangular format
 *
 * @brief FreeSurfer surface mesh
 */
class FSSHARED_EXPORT Surface
{
public:
    typedef QSharedPointer<Surface> SPtr;            /**< Shared pointer type for Surface class. */
    typedef QSharedPointer<const Surface> ConstSPtr; /**< Const shared pointer type for Surface class. */
    
    //=========================================================================================================
    /**
     * Default constructor
     */
    Surface();

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] p_sFile    Surface file name with path.
     */
    explicit Surface(const QString& p_sFile);

    //=========================================================================================================
    /**
     * Construts the surface by reading it of the given file.
     *
     * @param[in] subject_id         Name of subject.
     * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh}.
     * @param[in] surf               Name of the surface to load (eg. inflated, orig ...).
     * @param[in] subjects_dir       Subjects directory.
     */
    explicit Surface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir);

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
    explicit Surface(const QString &path, qint32 hemi, const QString &surf);

    //=========================================================================================================
    /**
     * Destroys the Surface class.
     */
    ~Surface();
    
    //=========================================================================================================
    /**
     * Initializes the Surface.
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
     * Returns whether Surface is empty.
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
    static bool read(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, Surface &p_Surface, bool p_bLoadCurvature = true);

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
    static bool read(const QString &path, qint32 hemi, const QString &surf, Surface &p_Surface, bool p_bLoadCurvature = true);

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
    static bool read(const QString &p_sFileName, Surface &p_Surface, bool p_bLoadCurvature = true);

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

private:
    QString m_sFilePath;    /**< Path to surf directory. */
    QString m_sFileName;    /**< Surface file name. */
    qint32 m_iHemi;         /**< Hemisphere (lh = 0; rh = 1). */
    QString m_sSurf;        /**< Loaded surface (eg. inflated, orig ...). */
    Eigen::MatrixX3f m_matRR;      /**< alias verts. Vertex coordinates in meters. */
    Eigen::MatrixX3i m_matTris;    /**< alias faces. The triangle descriptions. */
    Eigen::MatrixX3f m_matNN;      /**< Normalized surface normals for each vertex. -> not needed since qglbuilder is doing that for us. */
    Eigen::VectorXf m_vecCurv;     /**< FreeSurfer curvature data. */

    Eigen::Vector3f m_vecOffset; /**< Surface offset. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 Surface::hemi() const
{
    return m_iHemi;
}

//=============================================================================================================

inline bool Surface::isEmpty() const
{
    return m_iHemi == -1;
}

//=============================================================================================================

inline QString Surface::surf() const
{
    return m_sSurf;
}

//=============================================================================================================

inline const Eigen::MatrixX3f& Surface::rr() const
{
    return m_matRR;
}

//=============================================================================================================

inline const Eigen::MatrixX3i& Surface::tris() const
{
    return m_matTris;
}

//=============================================================================================================

inline const Eigen::MatrixX3f& Surface::nn() const
{
    return m_matNN;
}

//=============================================================================================================

inline const Eigen::VectorXf& Surface::curv() const
{
    return m_vecCurv;
}

//=============================================================================================================

inline const Eigen::Vector3f& Surface::offset() const
{
    return m_vecOffset;
}

//=============================================================================================================

inline Eigen::Vector3f& Surface::offset()
{
    return m_vecOffset;
}

//=============================================================================================================

inline QString Surface::filePath() const
{
    return m_sFilePath;
}

//=============================================================================================================

inline QString Surface::fileName() const
{
    return m_sFileName;
}
} // NAMESPACE

#endif // SURFACE_H
