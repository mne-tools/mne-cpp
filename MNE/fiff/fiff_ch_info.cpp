//=============================================================================================================
/**
* @file     fiff_ch_info.cpp
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
* @brief    Implementation of the FiffChInfo Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_ch_info.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffChInfo::FiffChInfo()
: scanno(0)
, logno(0)
, kind(0)
, range(-1)
, cal(0.0f)
, coil_type(0)
, coord_frame(FIFFV_COORD_UNKNOWN)
, unit(0)
, unit_mul(0)
, ch_name(QString(""))
{

}


//*************************************************************************************************************

FiffChInfo::FiffChInfo(const FiffChInfo &p_FiffChInfo)
: scanno(p_FiffChInfo.scanno)
, logno(p_FiffChInfo.logno)
, kind(p_FiffChInfo.kind)
, range(p_FiffChInfo.range)
, cal(p_FiffChInfo.cal)
, coil_type(p_FiffChInfo.coil_type)
, loc(p_FiffChInfo.loc)
, coil_trans(p_FiffChInfo.coil_trans)
, eeg_loc(p_FiffChInfo.eeg_loc)
, coord_frame(p_FiffChInfo.coord_frame)
, unit(p_FiffChInfo.unit)
, unit_mul(p_FiffChInfo.unit_mul)
, ch_name(p_FiffChInfo.ch_name)
{

}


//*************************************************************************************************************

FiffChInfo::~FiffChInfo()
{

}
