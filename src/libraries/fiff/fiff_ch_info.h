//=============================================================================================================
/**
 * @file     fiff_ch_info.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     FiffChInfo class declaration.
 *
 */

#ifndef FIFF_CH_INFO_H
#define FIFF_CH_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_ch_pos.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Channel Info descriptor replaces _fiffChInfoRec struct.
 *
 * @brief Channel info descriptor.
 */
class FIFFSHARED_EXPORT FiffChInfo
{
public:
    typedef QSharedPointer<FiffChInfo> SPtr;            /**< Shared pointer type for FiffChInfo. */
    typedef QSharedPointer<const FiffChInfo> ConstSPtr; /**< Const shared pointer type for FiffChInfo. */

    //=========================================================================================================
    /**
     * Constructs the channel info descriptor.
     */
    FiffChInfo();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffChInfo   Channel Info descriptor which should be copied.
     */
    FiffChInfo(const FiffChInfo &p_FiffChInfo);

    //=========================================================================================================
    /**
     * Destroys the channel info descriptor.
     */
    ~FiffChInfo();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffChInfoRec) 20*int + 16 = 20*4 + 16 = 96
     *
     * @return the size of the old struct fiffChInfoRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffChInfo &a, const FiffChInfo &b);

public:
    fiff_int_t    scanNo;       /**< Scanning order number 1*/
    fiff_int_t    logNo;        /**< Logical channel # 1*/
    fiff_int_t    kind;         /**< Kind of channel 1*/
    fiff_float_t  range;        /**< Voltmeter range (-1 = auto ranging) 1*/
    fiff_float_t  cal;          /**< Calibration from volts to units used 1*/
    FiffChPos     chpos;        /**< Channel location. */
    fiff_int_t    unit;         /**< Unit of measurement 1*/
    fiff_int_t    unit_mul;     /**< Unit multiplier exponent 1*/
    QString       ch_name;      /**< Descriptive name for the channel 16*/

    //Convinience members - MATLAB -
    Eigen::Matrix<float,4,4, Eigen::DontAlign>    coil_trans;     /**< Coil coordinate system transformation. */
    Eigen::Matrix<float,3,2, Eigen::DontAlign>    eeg_loc;        /**< Channel location. */
    fiff_int_t    coord_frame;                      /**< Coordinate Frame. */

// ### OLD STRUCT ###
//typedef struct _fiffChInfoRec {
//    fiff_int_t    scanNo;       /**< Scanning order number. */
//    fiff_int_t    logNo;        /**< Logical channel #. */
//    fiff_int_t    kind;         /**< Kind of channel. */
//    fiff_float_t  range;        /**< Voltmeter range (-1 = auto ranging). */
//    fiff_float_t  cal;          /**< Calibration from volts to units used. */
//    fiff_ch_pos_t chpos;        /**< Channel location. */
//    fiff_int_t    unit;         /**< Unit of measurement. */
//    fiff_int_t    unit_mul;     /**< Unit multiplier exponent. */
//    fiff_char_t   ch_name[16];  /**< Descriptive name for the channel. */
//} fiffChInfoRec,*fiffChInfo;    /**< Description of one channel. */

// /** Alias for fiffChInfoRec *
// typedef fiffChInfoRec fiff_ch_info_t;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffChInfo::storageSize()
{
    return 96;
}

//=============================================================================================================

inline bool operator== (const FiffChInfo &a, const FiffChInfo &b)
{
    return (a.scanNo == b.scanNo &&
            a.logNo == b.logNo &&
            a.kind == b.kind &&
            a.range == b.range &&
            a.cal == b.cal &&
            a.chpos == b.chpos &&
            a.unit == b.unit &&
            a.unit_mul == b.unit_mul &&
            a.coil_trans.isApprox(b.coil_trans, 0.0001f),
            a.eeg_loc.isApprox(b.eeg_loc, 0.0001f),
            a.coord_frame == b.coord_frame);
}
} // NAMESPACE

#endif // FIFF_CH_INFO_H
