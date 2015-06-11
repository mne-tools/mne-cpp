//=============================================================================================================
/**
* @file     mgh.h
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
* @brief     Mgh class declaration.
*
*/

#ifndef MGH_H
#define MGH_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri.h"
#include <vector>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================



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

class Mri;
class BLENdian;

//=============================================================================================================
/**
* Routines to handle proprietary mgh and mgz files containing mri information. File reading, unzipping,
* inclusion to data structure, etc.
*
* @brief mgh file handling.
*/
class FSSHARED_EXPORT Mgh
{
public:
    //=========================================================================================================
    /**
    * Constructs a Mgh object.
    */
    Mgh();

    //=========================================================================================================
    /**
    * Destroys the Mgh object.
    */
    ~Mgh();

    //=========================================================================================================
    /**
    * The function loadMGH reads in binary MGH files and stores the content to the data structure
    * defined in the Mri class.
    *
    * @param[in] fName  filename
    * @param[in] slices   todo
    * @param[in] frame  int number, indicating the index of the time frame, negativ numbers indicate a sequence
    * @param[in] headerOnly   bool flag indicating if only header or also volume of file should be read
    *
    * @return calculated result as an integer
    */
    static Mri loadMGH(QString fName, std::vector<int> slices, int frame, bool headerOnly);

    //=========================================================================================================
    /**
    *
    *
    * @param[in]  gzFName  absolute file name of compressed mgz file
    * @param[in]  fName  absolute file name of resulting uncompressed mgh file
    */
    static void unGz(QString gzFName, QString fName);

};

} // NAMESPACE

#endif // MGH_H
