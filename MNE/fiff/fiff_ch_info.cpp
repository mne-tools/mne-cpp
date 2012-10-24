//=============================================================================================================
/**
* @file     fiff_ch_info.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    Contains the implementation of the FiffChInfo Class.
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
, range(0.0f)
, cal(0.0f)
, coil_type(0)
, coord_frame(FIFFV_COORD_UNKNOWN)
, unit(0)
, unit_mul(0)
, ch_name(QString(""))
{
    loc.setZero();
    coil_trans.setZero();
    eeg_loc.setZero();
}


//*************************************************************************************************************

FiffChInfo::FiffChInfo(const FiffChInfo* ch)
: scanno(ch->scanno)
, logno(ch->logno)
, kind(ch->kind)
, range(ch->range)
, cal(ch->cal)
, coil_type(ch->coil_type)
, coord_frame(ch->coord_frame)
, unit(ch->unit)
, unit_mul(ch->unit_mul)
, ch_name(ch->ch_name)
, loc(Matrix<double,12,1, DontAlign>(ch->loc))
, coil_trans(Matrix<double,4,4, DontAlign>(ch->coil_trans))
, eeg_loc(Matrix<double,3,2, DontAlign>(ch->eeg_loc))
{


    Matrix<double,4,4, DontAlign>    coil_trans;  /**< Channel location */
    Matrix<double,3,2, DontAlign>    eeg_loc;
    loc.setZero();
    coil_trans.setZero();
    eeg_loc.setZero();
}


//*************************************************************************************************************

FiffChInfo::~FiffChInfo()
{

}
