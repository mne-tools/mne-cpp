//=============================================================================================================
/**
 * @file     fiff_evoked_set.h
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
 * @brief    FiffEvokedSet class declaration.
 *
 */

#ifndef FIFF_EVOKED_SET_H
#define FIFF_EVOKED_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info.h"
#include "fiff_evoked.h"
#include "fiff_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Fiff evoked data set
 *
 * @brief evoked data set
 */
class FIFFSHARED_EXPORT FiffEvokedSet
{

public:
    typedef QSharedPointer<FiffEvokedSet> SPtr;             /**< Shared pointer type for FiffEvokedSet. */
    typedef QSharedPointer<const FiffEvokedSet> ConstSPtr;  /**< Const shared pointer type for FiffEvokedSet. */

    //=========================================================================================================
    /**
     * Constructs a fiff evoked data set.
     */
    FiffEvokedSet();

    //=========================================================================================================
    /**
     * Constructs a fiff evoked data set, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the evoked data set.
     */
    FiffEvokedSet(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffEvokedSet    Fiff evoked data set which should be copied.
     */
    FiffEvokedSet(const FiffEvokedSet& p_FiffEvokedSet);

    //=========================================================================================================
    /**
     * Destroys the fiff evoked data set.
     */
    ~FiffEvokedSet();

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
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the desired fiff evoked data set.
     */
    FiffEvokedSet pick_channels(const QStringList& include = defaultQStringList,
                                const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
     * mne_compensate_to
     *
     * ### MNE toolbox root function ### Definition of the mne_compensate_to function
     *
     * Apply compensation to the data as desired
     *
     * @param[in] to                 desired compensation in the output.
     * @param[in, out] p_FiffEvoked   Evoked set to compensate.
     *
     * @return true if succeeded, false otherwise.
     */
    bool compensate_to(FiffEvokedSet &p_FiffEvokedSet,
                       fiff_int_t to) const;

    //=========================================================================================================
    /**
     * fiff_find_evoked
     *
     * ### MNE toolbox root function ###
     *
     * Find evoked data sets
     *
     * @param[out] p_FiffEvokedSet   The read evoked data set.
     *
     * @return true when any set was found, false otherwise.
     */
    bool find_evoked(const FiffEvokedSet& p_FiffEvokedSet) const;

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
     * @param[in] p_IODevice         An fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] p_FiffEvokedSet   The read evoked data set.
     * @param[in] baseline           The time interval to apply rescaling / baseline correction. If None do not apply it. If baseline is (a, b).
     *                               the interval is between "a (s)" and "b (s)". If a is None the beginning of the data is used and if b is
     *                               None then b is set to the end of the interval. If baseline is equal ot (None, None) all the time interval is used.
     *                               If None, no correction is applied.
     * @param[in] proj               Apply SSP projection vectors (optional, default = true).
     *
     * @return true when successful, false otherwise.
     */
    static bool read(QIODevice& p_IODevice,
                     FiffEvokedSet& p_FiffEvokedSet,
                     QPair<float,float> baseline = defaultFloatPair,
                     bool proj = true);

public:
    FiffInfo             info;   /**< FIFF measurement information. */
    QList<FiffEvoked>    evoked; /**< List of Fiff Evoked Data. */
};
} // NAMESPACE

#ifndef metatype_fiffevokedset
#define metatype_fiffevokedset
Q_DECLARE_METATYPE(FIFFLIB::FiffEvokedSet);/**< Provides QT META type declaration of the FIFFLIB::FiffEvokedSet type. For signal/slot and QVariant usage.*/
#endif

#ifndef metatype_fiffevokedsetsptr
#define metatype_fiffevokedsetsptr
Q_DECLARE_METATYPE(FIFFLIB::FiffEvokedSet::SPtr);/**< Provides QT META type declaration of the FIFFLIB::FiffEvokedSet type. For signal/slot and QVariant usage.*/
#endif

#endif // FIFF_EVOKED_SET_H
