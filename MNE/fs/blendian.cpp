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
* @brief     BLEndian class implementation. Ported from machine.c in FreeSurfer.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "blendian.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BLEndian::BLEndian()
{

}

//*************************************************************************************************************


// big und little endian swap
int BLEndian::swapInt(int source)
{
    unsigned char *csource = (unsigned char *) &source;
    int result;
    unsigned char *cresult = (unsigned char *) &result;
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return result;
}

//*************************************************************************************************************

// read Integer value from file
int BLEndian::freadInt(FILE *fp)
{
    int i, size_i;

    size_i = fread(&i, sizeof(int), 1, fp);
    i = swapInt(i);
    return i;
}

//*************************************************************************************************************

short BLEndian::swapShort(short source)
{
    unsigned char *csource = (unsigned char *)&source;
    short result;
    unsigned char *cresult = (unsigned char *)&result;
    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return result;
}

//*************************************************************************************************************

short BLEndian::freadShort(FILE *fp)
{
    int size_s;
    short s;

    size_s = fread(&s, sizeof(short), 1, fp);
    s = swapShort(s);
    return s;
}

//*************************************************************************************************************

float BLEndian::swapFloat(float source)
{
    char *csource = (char*) &source;
    float result;
    char *cresult = (char*) &result;

    // swap the bytes into a temporary buffer
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];

    return result;
}

//*************************************************************************************************************

float BLEndian::freadFloat(FILE *fp)
{
    float f;
    int size_f;

    size_f = fread(&f, sizeof(float), 1, fp);
    f = swapFloat(f);
    return f;
}

//*************************************************************************************************************

float BLEndian::freadFloatEx(float *pf, FILE *fp)
{
    int size_pf;
    size_pf = fread(pf, sizeof(float), 1, fp);
    *pf = swapFloat(*pf);
    return size_pf;
}

//*************************************************************************************************************


BLEndian::~BLEndian()
{

}

