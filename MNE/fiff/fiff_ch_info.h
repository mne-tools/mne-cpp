//=============================================================================================================
/**
* @file     fiff_ch_info.h
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
* @brief    Contains the FiffChInfo class declaration.
*
*/

#ifndef FIFF_CH_INFO_H
#define FIFF_CH_INFO_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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
* Channel Info descriptor replaces _fiffChInfoRec struct.
*
* @brief Channel info descriptor.
*/
class FIFFSHARED_EXPORT FiffChInfo {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffChInfo();

    //=========================================================================================================
    /**
    * Copy ctor
    */
    FiffChInfo(const FiffChInfo* ch);

    //=========================================================================================================
    /**
    * Destroys the FiffChInfoRec.
    */
    ~FiffChInfo();

    //=========================================================================================================
    /**
    * Size of the old struct (fiffChInfoRec) 20*int + 16 = 20*4 + 16 = 96
    *
    * @return the size of the old struct fiffChInfoRec.
    */
    inline static qint32 storageSize();

public:
    //EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    fiff_int_t    scanno;       /**< Scanning order number 1*/
    fiff_int_t    logno;        /**< Logical channel # 1*/
    fiff_int_t    kind;         /**< Kind of channel 1*/
    fiff_float_t  range;        /**< Voltmeter range (-1 = auto ranging) 1*/
    fiff_float_t  cal;          /**< Calibration from volts to units used 1*/

    fiff_int_t coil_type;       /**< Which kind of coil. */

    Matrix<double,12,1, DontAlign>  loc;
    Matrix<double,4,4, DontAlign>   coil_trans;  /**< Channel location */
    Matrix<double,3,2, DontAlign>   eeg_loc;
    fiff_int_t                      coord_frame;

    //    fiff_ch_pos_t chpos;        /**< Channel location 15*/
    fiff_int_t    unit;         /**< Unit of measurement 1*/
    fiff_int_t    unit_mul;     /**< Unit multiplier exponent 1*/
    QString       ch_name;      /**< Descriptive name for the channel 16*/

// typedef struct _fiffChInfoRec {
//     fiff_int_t    scanNo;		/**< Scanning order number *
//     fiff_int_t    logNo;		/**< Logical channel # *
//     fiff_int_t    kind;			/**< Kind of channel *
//     fiff_float_t  range;		/**< Voltmeter range (-1 = auto ranging) *
//     fiff_float_t  cal;			/**< Calibration from volts to units used *
//     fiff_ch_pos_t chpos;		/**< Channel location *
//     fiff_int_t    unit;			/**< Unit of measurement *
//     fiff_int_t    unit_mul;		/**< Unit multiplier exponent *
//     fiff_char_t   ch_name[16];	/**< Descriptive name for the channel *
// } fiffChInfoRec,*fiffChInfo;	/**< Description of one channel *

// /** Alias for fiffChInfoRec *
// typedef fiffChInfoRec fiff_ch_info_t;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffChInfo::storageSize()
{
    return 96;
}

} // NAMESPACE

#endif // FIFF_CH_INFO_H
