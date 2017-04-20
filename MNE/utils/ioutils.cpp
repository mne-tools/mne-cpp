//=============================================================================================================
/**
* @file     ioutils.cpp
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
* @brief    Implementation of the IOUtils class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ioutils.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

qint32 IOUtils::fread3(QDataStream &p_qStream)
{
    char* bytes = new char[3];
    p_qStream.readRawData(bytes, 3);
    qint32 int3 = (((unsigned char) bytes[0]) << 16) + (((unsigned char) bytes[1]) << 8) + ((unsigned char) bytes[2]);
    delete[] bytes;
    return int3;
}


//*************************************************************************************************************

VectorXi IOUtils::fread3_many(QDataStream &p_qStream, qint32 count)
{
    VectorXi res(count);

    for(qint32 i = 0; i < count; ++i)
        res[i] = IOUtils::fread3(p_qStream);

    return res;
}


//*************************************************************************************************************
//fiff_combat
qint16 IOUtils::swap_short(qint16 source)
{
    unsigned char *csource = (unsigned char *)(&source);
    qint16 result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return (result);
}


//*************************************************************************************************************

qint32 IOUtils::swap_int(qint32 source)
{
    unsigned char *csource =  (unsigned char *)(&source);
    qint32 result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return (result);
}


//*************************************************************************************************************

void IOUtils::swap_intp(qint32 *source)

{
  unsigned char *csource =  (unsigned char *)(source);

  unsigned char c;

  c = csource[3];
  csource[3] = csource[0];
  csource[0] = c;
  c = csource[2];
  csource[2] = csource[1];
  csource[1] = c;

  return;
}


//*************************************************************************************************************

qint64 IOUtils::swap_long(qint64 source)
{
    unsigned char *csource =  (unsigned char *)(&source);
    qint64    result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[7];
    cresult[1] = csource[6];
    cresult[2] = csource[5];
    cresult[3] = csource[4];
    cresult[4] = csource[3];
    cresult[5] = csource[2];
    cresult[6] = csource[1];
    cresult[7] = csource[0];
    return (result);
}


//*************************************************************************************************************

void IOUtils::swap_longp(qint64 *source)
{
    unsigned char *csource =  (unsigned char *)(source);
    unsigned char c;

    c = csource[0];
    csource[0] = csource[7];
    csource[7] = c;

    c = csource[1];
    csource[1] = csource[6];
    csource[6] = c;

    c = csource[2];
    csource[2] = csource[5];
    csource[5] = c;

    c = csource[3];
    csource[3] = csource[4];
    csource[4] = c;

    return;
}


//*************************************************************************************************************

float IOUtils::swap_float(float source)
{
    unsigned char *csource =  (unsigned char *)(&source);
    float result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return (result);
}

//*************************************************************************************************************

void IOUtils::swap_floatp(float *source)

{
    unsigned char *csource =  (unsigned char *)(source);
    unsigned char c;

    c = csource[3];
    csource[3] = csource[0];
    csource[0] = c;
    c = csource[2];
    csource[2] = csource[1];
    csource[1] = c;

    return;
}


//*************************************************************************************************************

void IOUtils::swap_doublep(double *source)

{
    unsigned char *csource =  (unsigned char *)(source);
    unsigned char c;

    c = csource[7];
    csource[7] = csource[0];
    csource[0] = c;

    c = csource[6];
    csource[6] = csource[1];
    csource[1] = c;

    c = csource[5];
    csource[5] = csource[2];
    csource[2] = c;

    c = csource[4];
    csource[4] = csource[3];
    csource[3] = c;

    return;
}


