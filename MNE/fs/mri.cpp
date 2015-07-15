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
* @brief     Mri class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri.h"
#include <QtDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Mri::Mri()
{

}

//*************************************************************************************************************

// width, height, depth, type
void Mri::allocHeader(int x, int y, int z, int t)
{
    imNr0 = 1;
    imNr1 = depth;
    fov = x;
    thick = 1.0;
    scale = 1;
    roi.dX = width = x;
    roi.dY = height = y;
    roi.dZ = depth = z;
    yInvert = 1 ;
    xSize = ySize = zSize = 1;
    type = t;
    nFrames = 1;
    xI = yI = zI = 0;
    ps = 1;
    xStart = -width/2.0;
    xEnd = width/2.0;
    yStart = -height/2.0;
    yEnd = height/2.0;
    zStart = -depth/2.0 ;
    zEnd = depth/2 ;
    //
    x_r = -1;
    x_a = 0.;
    x_s = 0.;
    //
    y_r = 0.;
    y_a = 0.;
    y_s = -1;
    //
    z_r = 0.;
    z_a = 1.;
    z_s = 0.;
    //
    c_r = c_a = c_s = 0.0;
    ras_good_flag = 0;
    brightness = 1;
    subject_name = '\0';
    path_to_t1 = '\0';
    fName_format = '\0';
    gdf_image_stem = '\0';
    tag_data = 0;
    tag_data_size = 0;
}

//*************************************************************************************************************

void Mri::allocSequence(int x, int y, int z, int t, int f)
{

    // todo: what is the mri allocChunk method for? check, use and implement if useful...
    if (x<=0 || y<=0 || z<=0)
        qDebug() << "allocSequence("
                 << x << ", "
                 << y << ", "
                 << z << ", "
                 << t << ", "
                 << f << "): bad parameters";
    allocHeader(x, y, z, t);
    //todo: initHeader();
    nFrames = f;
}

//*************************************************************************************************************

Mri::~Mri()
{

}
