//=============================================================================================================
/**
* @file     fiff_raw_data.h
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
* @brief    Contains the FiffRawData class declaration.
*
*/

#ifndef FIFF_RAW_DATA_H
#define FIFF_RAW_DATA_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_info.h"
#include "fiff_proj.h"
#include "fiff_ctf_comp.h"
#include "fiff_raw_dir.h"
#include "fiff_stream.h"
#include "fiff_tag.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../3rdParty/Eigen/Core"
#include "../3rdParty/Eigen/SparseCore"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffRawData;
class FiffStream;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
*Provides fiff raw measurement data, including I/O routines.
*
* @brief FIFF raw measurement data
*/
class FIFFSHARED_EXPORT FiffRawData {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffRawData();

    //=========================================================================================================
    /**
    * Destroys the FiffInfo.
    */
    ~FiffRawData();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_read_raw_segment function
    *
    * Read a specific raw data segment
    *
    * @param[out] data      returns the data matrix (channels x samples)
    * @param[out] times     returns the time values corresponding to the samples
    * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional)
    * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional)
    * @param[in] sel        channel selection vector (optional)
    *
    * @return true if succeeded, false otherwise
    */
    bool read_raw_segment(MatrixXd*& data, MatrixXd*& times, fiff_int_t from = -1, fiff_int_t to = -1, MatrixXi sel = defaultMatrixXi);


    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_read_raw_segment function
    *
    * Read a specific raw data segment
    *
    * @param[out] data      returns the data matrix (channels x samples)
    * @param[out] times     returns the time values corresponding to the samples
    * @param[in] from       starting time of the segment in seconds
    * @param[in] to         end time of the segment in seconds
    * @param[in] sel        optional channel selection vector
    *
    * @return true if succeeded, false otherwise
    */
    bool read_raw_segment_times(MatrixXd*& data, MatrixXd*& times, float from, float to, MatrixXi sel = defaultMatrixXi)
    {
        //
        //   Convert to samples
        //
        from = floor(from*this->info->sfreq);
        to   = ceil(to*this->info->sfreq);
        //
        //   Read it
        //
        return this->read_raw_segment(data, times, (qint32)from, (qint32)to, sel);
    }



public:
    FiffStream* file;//replaces fid
    FiffInfo* info;
    fiff_int_t first_samp;
    fiff_int_t last_samp;
    MatrixXd   cals;
    QList<FiffRawDir> rawdir;
    MatrixXd*  proj;
    FiffCtfComp comp;
};

} // NAMESPACE

#endif // FIFF_RAW_DATA_H
