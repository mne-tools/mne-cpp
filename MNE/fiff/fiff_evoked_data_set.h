//=============================================================================================================
/**
* @file     fiff_evoked_data_set.h
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
* @brief    FiffEvokedDataSet class declaration.
*
*/

#ifndef FIFF_EVOKED_DATA_SET_H
#define FIFF_EVOKED_DATA_SET_H


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_types.h"
#include "fiff_info.h"
#include "fiff_evoked_data.h"
#include "fiff_stream.h"
#include "fiff_dir_tree.h"
#include "fiff_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QList>
#include <QSharedPointer>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
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
* Fiff evoked data
*
* @brief evoked data
*/
class FIFFSHARED_EXPORT FiffEvokedDataSet
{

public:
    typedef QSharedPointer<FiffEvokedDataSet> SPtr;             /**< Shared pointer type for FiffEvokedDataSet. */
    typedef QSharedPointer<const FiffEvokedDataSet> ConstSPtr;  /**< Const shared pointer type for FiffEvokedDataSet. */

    //=========================================================================================================
    /**
    * Constructs a fiff evoked data set.
    */
    FiffEvokedDataSet();

    //=========================================================================================================
    /**
    * Constructs a fiff evoked data set, by reading from a IO device.
    *
    * @param[in] p_IODevice     IO device to read from the evoked data set.
    * @param[in] setno          The set to pick.
    */
    FiffEvokedDataSet(QIODevice& p_IODevice, fiff_int_t setno);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffEvokedDataSet    Fiff evoked data set which should be copied
    */
    FiffEvokedDataSet(const FiffEvokedDataSet& p_FiffEvokedDataSet);

    //=========================================================================================================
    /**
    * Destroys the fiff evoked data set.
    */
    ~FiffEvokedDataSet();

    //=========================================================================================================
    /**
    * Initializes fiff evoked data set.
    */
    void clear();

    //=========================================================================================================
    /**
    * fiff_pick_channels_evoked
    *
    * ### MNE toolbox root function ###
    *
    * Pick desired channels from evoked-response data
    *
    * @param[in] include   - Channels to include (if empty, include all available)
    * @param[in] exclude   - Channels to exclude (if empty, do not exclude any)
    *
    * @return the desired fiff evoked data set
    */
    FiffEvokedDataSet pick_channels(const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
    * fiff_read_evoked
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffEvokedDataSet::read_evoked static function
    *
    * Read one evoked data set
    *
    * @param[in] p_IODevice     An fiff IO device like a fiff QFile or QTCPSocket
    * @param[out] data          The read evoked data
    * @param[in] setno          the set to pick
    *
    * @return the CTF software compensation data
    */
    static bool read_evoked(QIODevice& p_IODevice, FiffEvokedDataSet& data, fiff_int_t setno = 0);

public:
    FiffInfo                        info;   /**< FIFF measurement information */
    QList<FiffEvokedData::SDPtr>    evoked; /**< List of Fiff Evoked Data */
};

} // NAMESPACE

#endif // FIFF_EVOKED_DATA_SET_H
