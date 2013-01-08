//=============================================================================================================
/**
* @file     fiff_evoked_data.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the FIFFEvokedData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_evoked_data.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvokedData::FiffEvokedData()
: aspect_kind(-1)
, is_smsh(-1)
, nave(-1)
, first(-1)
, last(-1)
, comment("")
, times(NULL)
, epochs(MatrixXd())
{

}


//*************************************************************************************************************

FiffEvokedData::FiffEvokedData(const FiffEvokedData* p_pFiffEvokedData)
: aspect_kind(p_pFiffEvokedData->aspect_kind)
, is_smsh(p_pFiffEvokedData->is_smsh)
, nave(p_pFiffEvokedData->nave)
, first(p_pFiffEvokedData->first)
, last(p_pFiffEvokedData->last)
, comment(p_pFiffEvokedData->comment)
, times(p_pFiffEvokedData->times ? new MatrixXd(*p_pFiffEvokedData->times) : NULL )
, epochs(p_pFiffEvokedData->epochs)//p_pFiffEvokedData->epochs ? new MatrixXd(*p_pFiffEvokedData->epochs) : NULL )
{

}


//*************************************************************************************************************

FiffEvokedData::~FiffEvokedData()
{
    if (times)
        delete times;
//    if (epochs)
//        delete epochs;
}
