//=============================================================================================================
/**
* @file     mri.h
* @author   Carsten Boensel <carsten.boensel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*           Bruce Fischl <fischl@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) June, 2015 Carsten Boensel and Matti Hamalainen. All rights reserved.
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
* @brief     Mri class declaration.
*
*/

#ifndef MRI_H
#define MRI_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

#define BUFTYPE  unsigned char

#define VERBOSE true

#define MRI_UCHAR 0
#define MRI_INT 1
#define MRI_LONG 2;
#define MRI_FLOAT 3
#define MRI_SHORT 4
#define MRI_BITMAP 5

//=============================================================================================================
/**
* Mri class holds and maintains the data structure needed for mri data.
*
* @brief Mri data structure.
*/
class FSSHARED_EXPORT Mri
{
public:
    //=========================================================================================================
    /**
    * Default constructor.
    */
    Mri();

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~Mri();

    //=========================================================================================================
    struct MriRegion {
        int x;
        int y;
        int z;
        int dX;
        int dY;
        int dZ;
    };
    MriRegion mriRegion;

    int width;
    int height;
    int depth;     /* # of slices */
    int type;      /* data type for slices below */
    int imNr0;     /* starting image # */
    int imNr1;     /* ending image # */

    float fov;
    float thick;
    float ps;
    float xSize;     /* size of a voxel in the x direction */
    float ySize;     /* size of a voxel in the y direction */
    float zSize;     /* size of a voxel in the z direction */
    float xStart;    /* start x (in xsize units) */
    float xEnd;      /* end x  (in xsize units) */
    float yStart;    /* start y   (in ysize units) */
    float yEnd;      /* end y (in ysize units) */
    float zStart;    /* start z */
    float zEnd;      /* end z */
    float tR;        /* time to recovery */
    float tE;        /* time to echo */
    float tI;        /* time to inversion */

//    char fname[STR_LEN]; // what data type? -> qstring?

    float x_r, x_a, x_s; /* these are the RAS distances
                            across the whole volume */
    float y_r, y_a, y_s; /* in x, y, and z */
    float z_r, z_a, z_s; /* c_r, c_a, and c_s are the
                            center ras coordinates */
    float c_r, c_a, c_s; /* ras_good_flag tells if
                            these coordinates are set */
    int ras_good_flag; /* and accurate for the volume */

    /*  for bshorts and bfloats */
    int STRLEN = 40; // as I use QString, apparently not needed
    int brightness;
    QString subject_name;
//    MATRIX        *register_mat; // struct MATRIX does not yet exist
    QString path_to_t1;
    QString fname_format;

//    /* for gdf volumes */
    char gdf_image_stem;

//    /*
//       each slice is an array of rows (mri->height of them) each of which is
//       mri->width long.
//    */
    BUFTYPE ***slices ;
    int scale;
//    char          transform_fname[STR_LEN] ;
//    General_transform transform ;   /* the next two are from this struct */
//    Transform         *linear_transform ;
//    Transform         *inverse_linear_transform ;
//    int           free_transform ;   /* are we responsible for freeing it? */
    int nFrames;          /* # of concatenated images */

    /* these are used to handle boundary conditions (arrays of indices) */
    int *xI;
    int *yI;
    int *zI;
    int yInvert;  /* for converting between MNC and coronal slices */
    MriRegion roi;
    int dof;
//    double        mean ;
//    double        flip_angle ;  /* in radians */

    void* tag_data; /* saved tag data */
    int tag_data_size; /* size of saved tag data */
//    MATRIX *i_to_r__; /* cache */
//    MATRIX *r_to_i__;
//    char   *cmdlines[MAX_CMDS];
//    int    ncmds;

    //=========================================================================================================
    /**
    * allocates the header in the mri data structure.
    *
    * @param[in]  x  width
    * @param[in]  y  height
    * @param[in]  z  depth
    * @param[in]  t  type
    */
    // width, height, depth, type, nFrames
    void allocHeader(int x, int y, int z, int t);

    //=========================================================================================================
    /**
    * allocates a sequence of data in the mri data structure.
    *
    * @param[in]  x  width
    * @param[in]  y  height
    * @param[in]  z  depth
    * @param[in]  t  type
    * @param[in]  f  number of frames
    */
    void allocSequence(int x, int y, int z, int t, int f);

};

} // NAMESPACE

#endif // MRI_H
