//=============================================================================================================
/**
 * @file     mne_vol_geom.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MneVolGeom class declaration.
 *
 */

#ifndef MNEVOLGEOM_H
#define MNEVOLGEOM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements the MNE Volume Geometry description (Replaces *mneVolGeom,mneVolGeomRec; struct of MNE-C mne_types.h).
 *
 * @brief MRI data volume geometry information like FreeSurfer keeps it
 */
class MNESHARED_EXPORT MneVolGeom
{
public:
    typedef QSharedPointer<MneVolGeom> SPtr;              /**< Shared pointer type for MneVolGeom. */
    typedef QSharedPointer<const MneVolGeom> ConstSPtr;   /**< Const shared pointer type for MneVolGeom. */

    //=========================================================================================================
    /**
     * Constructs the MNE Volume Geometry
     */
    MneVolGeom();

    //=========================================================================================================
    /**
     * Destroys the MNE Volume Geometry
     * Refactored: mne_free_vol_geom (mne_mgh_mri_io.c)
     */
    ~MneVolGeom();

public:
    int     valid;                       /* Is the information below valid */
    int     width,height,depth;          /* Size of the stack */
    float   xsize,ysize,zsize;           /* Increments in the three voxel directions */
    float   x_ras[3],y_ras[3],z_ras[3];  /* Directions of the coordinate axes */
    float   c_ras[3];                    /* Center of the RAS coordinates */
    QString filename;                   /* Name of the MRI data file */

// ### OLD STRUCT ###
//typedef struct {
//    int            valid;                       /* Is the information below valid */
//    int            width,height,depth;          /* Size of the stack */
//    float          xsize,ysize,zsize;           /* Increments in the three voxel directions */
//    float          x_ras[3],y_ras[3],z_ras[3];  /* Directions of the coordinate axes */
//    float          c_ras[3];                    /* Center of the RAS coordinates */
//    char           *filename;                   /* Name of the MRI data file */
//} *mneVolGeom,mneVolGeomRec;                    /* MRI data volume geometry information like FreeSurfer keeps it */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEVOLGEOM_H
