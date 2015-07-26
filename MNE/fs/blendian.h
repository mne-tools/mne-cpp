//=============================================================================================================
/**
* @file     blendian.h
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
* @brief     BLEndian class declaration.
*
*/

#ifndef BLENDIAN_H
#define BLENDIAN_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include <stdio.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
/**
* Functions to swap between big and little endian format. Ported from machine.h in FreeSurfer.
*
* @brief conversion beetween big and little endian.
*/

class FSSHARED_EXPORT BLEndian
{
public:
    //=========================================================================================================
    /**
    * Constructs a BLEndian object.
    */
    BLEndian();

    //=========================================================================================================
    /**
    * Destroys the BLEndian object.
    */
    ~BLEndian();

    //=========================================================================================================
    /**
    * The function swapInt takes integer values and returns the swaped ones.
    *
    * @param[in]  source  An integer value which should be swaped
    *
    * @return calculated result as an integer
    */
    static int swapInt(int source);

    //=========================================================================================================
    /**
    * The function freadInt reads file and swaps data.
    *
    * @param[in]  fp  file to read
    *
    * @return calculated result as an integer
    */
    static int freadInt(FILE *fp);

    //=========================================================================================================
    /**
    * The function swapShort takes short values and returns the swaped ones.
    *
    * @param[in]  source  A short value which should be swaped
    *
    * @return calculated result as a short
    */
    static short swapShort(short source);

    //=========================================================================================================
    /**
    * The function freadShort reads file and swaps data.
    *
    * @param[in]  fp  file to read
    *
    * @return calculated result as a short
    */
    static short freadShort(FILE *fp);

    //=========================================================================================================
    /**
    * The function swapFloat takes float values and returns the swaped ones.
    *
    * @param[in]  source  a float value which should be swaped
    *
    * @return calculated result as a float
    */
    static float swapFloat(float source);

    //=========================================================================================================
    /**
    * The function freadFloat reads file and swaps data.
    *
    * @param[in]  fp  file to read
    *
    * @return calculated result as a float
    */
    static float freadFloat(FILE *fp);

    //=========================================================================================================
    /**
    * The function freadFloatEx reads file, swaps data, and returns size of it.
    *
    * @param[in]  pf  TemplateClass which should be copied
    * @param[in]  fp  file to read
    *
    * @return calculated result as a float
    */
    static float freadFloatEx(float *pf, FILE *fp);

};

} // NAMESPACE

#endif // BLENDIAN_H
