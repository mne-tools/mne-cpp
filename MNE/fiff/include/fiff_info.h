//=============================================================================================================
/**
* @file     fiff_info.h
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
* @brief    Contains the FiffInfo class declaration.
*
*/

#ifndef FIFF_INFO_H
#define FIFF_INFO_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_ch_info.h"
#include "fiff_ctf_comp.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS FiffCtfComp
*
* CTF software compensation data
*
* @brief The FiffCtfComp class provides CTF software compensation data
*/
class FIFFSHARED_EXPORT FiffInfo {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffInfo()
    : acq_pars("")
    , acq_stim("")
    , filename("")
    {
    }


    //=========================================================================================================
    /**
    * Destroys the FiffInfo.
    */
    ~FiffInfo()
    {
        qint32 i;
        for (i = 0; i < projs.size(); ++i)
            if(projs[i])
                delete projs[i];
        for (i = 0; i < comps.size(); ++i)
            if(comps[i])
                delete comps[i];
    }

public:
    FiffId      file_id;
    FiffId      meas_id;
    fiff_int_t  meas_date[2];
    fiff_int_t  nchan;
    float sfreq;
    float highpass;
    float lowpass;
    QList<FiffChInfo> chs;
    QStringList ch_names;
    FiffCoordTrans dev_head_t;
    FiffCoordTrans ctf_head_t;
    FiffCoordTrans dev_ctf_t;
    QList<fiff_dig_point_t> dig;
    FiffCoordTrans dig_trans;
    QStringList bads;
    QList<FiffProj*> projs;
    QList<FiffCtfComp*> comps;
    QString acq_pars;
    QString acq_stim;
    QString filename;
};

} // NAMESPACE

#endif // FIFF_INFO_H
