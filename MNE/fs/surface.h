//=============================================================================================================
/**
* @file     surface.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
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
    * @param[in] p_sFile    Surface file name with path
    */
    explicit Surface(const QString& p_sFile);
    
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
    * @return hemisphere id
    */
    inline qint32 getHemi() const;

    //=========================================================================================================
    /**
    * Returns whether Surface is empty.
    *
    * @return true if is empty, false otherwise
    */
    inline bool isEmpty();

    //=========================================================================================================
    /**
    * mne_read_surface
    *
    * Reads a FreeSurfer surface file
    *
    * @param[in] p_sFileName        The file to read
    * @param[out] p_Surface         The read surface
    * @param[in] p_bLoadCurvature   True if the curvature should be read (optional, default = true)
    *
    * @return true if read sucessful, false otherwise
    */
    static bool read(const QString &p_sFileName, Surface &p_Surface, bool p_bLoadCurvature = true);

    //=========================================================================================================
    /**
    * reads a binary curvature file into a vector
    *
    * @return the read curvature
    */
    static VectorXf read_curv(const QString &p_sFileName);

    //=========================================================================================================
    /**
    * Efficiently compute vertex normals for triangulated surface
    *
    * @param[in] rr     Vertex coordinates in meters
    * @param[out] tris  The triangle descriptions
    *
    * @return The computed normals
    */
    static MatrixX3f compute_normals(const MatrixX3f& rr, const MatrixX3i& tris);

public:
    QString m_sFilePath; /**< Path to surf directory. */
    QString m_sFileName; /**< Surface file name. */
    qint32 hemi;        /**< Hemisphere (lh = 0; rh = 1) */
    QString surf;       /**< Name of the surface to load (eg. inflated, orig ...) */
    MatrixX3f rr;       /**< alias verts. Vertex coordinates in meters */
    MatrixX3i tris;     /**< alias faces. The triangle descriptions */
    MatrixX3f nn;       /**< Normalized surface normals for vertices. */
    VectorXf curv;      /**< FreeSurfer curvature data */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 Surface::getHemi() const
{
    return hemi;
}


//*************************************************************************************************************

inline bool Surface::isEmpty()
{
    return hemi == -1;
}

} // NAMESPACE

#endif // SURFACE_H
