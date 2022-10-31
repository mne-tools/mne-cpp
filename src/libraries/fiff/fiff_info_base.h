//=============================================================================================================
/**
 * @file     fiff_info_base.h
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
 * @brief    FiffInfoBase class declaration.
 *
 */

#ifndef FIFF_INFO_BASE_H
#define FIFF_INFO_BASE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_ch_info.h"
#include "fiff_dig_point.h"
#include "fiff_ctf_comp.h"
#include "fiff_coord_trans.h"
#include "fiff_proj.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QStringList>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Light measurement info
 *
 * @brief light measurement info
 */
class FIFFSHARED_EXPORT FiffInfoBase
{

public:
    typedef QSharedPointer<FiffInfoBase> SPtr;              /**< Shared pointer type for FiffInfoBase. */
    typedef QSharedPointer<const FiffInfoBase> ConstSPtr;   /**< Const shared pointer type for FiffInfoBase. */

    //=========================================================================================================
    /**
     * Constructors the light fiff measurement file information.
     */
    FiffInfoBase();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffInfoBase  light FIFF measurement information which should be copied.
     */
    FiffInfoBase(const FiffInfoBase& p_FiffInfoBase);

    //=========================================================================================================
    /**
     * Destroys the light fiff measurement file information.
     */
    virtual ~FiffInfoBase();

    //=========================================================================================================
    /**
     * Initializes light FIFF measurement information.
     */
    void clear();

    //=========================================================================================================
    /**
     * Get channel type.
     *
     * @param[in] idx    Index of channel.
     *
     * @return Type of channel ('grad', 'mag', 'eeg', 'stim', 'eog', 'emg', 'ecg').
     */
    QString channel_type(qint32 idx) const;

    //=========================================================================================================
    /**
     * True if FIFF measurement file information is empty.
     *
     * @return true if FIFF measurement file information is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * fiff_pick_channels
     *
     * ### MNE toolbox root function ###
     *
     * Make a selector to pick desired channels from data
     *
     * @param[in] ch_names  - The channel name list to consult.
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row Vector).
     */
    static Eigen::RowVectorXi pick_channels(const QStringList& ch_names,
                                            const QStringList& include = defaultQStringList,
                                            const QStringList& exclude = defaultQStringList);

    //=========================================================================================================
    /**
     * fiff_pick_info
     *
     * ### MNE toolbox root function ###
     *
     * Pick desired channels from measurement info
     *
     * @param[in] sel    List of channels to select.
     *
     * @return Info modified according to sel.
     */
    FiffInfoBase pick_info(const Eigen::RowVectorXi* sel = Q_NULLPTR) const;

    //=========================================================================================================
    /**
     * fiff_pick_types (highy diversity in meg picking)
     *
     * ### MNE toolbox root function ###
     *
     * Create a selector to pick desired channel types from data
     *
     * @param[in] meg        It can be "all", to select all or it can be "mag" or "grad" to select only gradiometers or magnetometers.
     * @param[in] eeg        Include EEG channels.
     * @param[in] stim       Include stimulus channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row vector).
     */
    Eigen::RowVectorXi pick_types(const QString meg,
                                  bool eeg = false,
                                  bool stim = false,
                                  const QStringList& include = defaultQStringList,
                                  const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
     * fiff_pick_types
     *
     * ### MNE toolbox root function ###
     *
     * Create a selector to pick desired channel types from data
     * Use overloaded pick_types method to specify meg (grad, mag, ref_meg)type
     *
     * @param[in] meg        Include MEG channels.
     * @param[in] eeg        Include EEG channels.
     * @param[in] stim       Include stimulus channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row vector).
     */
    Eigen::RowVectorXi pick_types(bool meg,
                                  bool eeg = false,
                                  bool stim = false,
                                  const QStringList& include = defaultQStringList,
                                  const QStringList& exclude = defaultQStringList) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffInfoBase &a, const FiffInfoBase &b);

    /**
     * Parses the channel info information and returns a string list of channel types.
     *
     * @return The channel types present in this fiff info (grad,mag,eeg,ecg,emg,misc,stim).
     */
    QStringList get_channel_types();

public:
    QString filename;           /**< Filename when the info is read of a fiff file. */
    QStringList bads;           /**< List of bad channels. */
    FiffId meas_id;             /**< Measurement ID. */
    fiff_int_t  nchan;          /**< Number of channels. */
    QList<FiffChInfo> chs;      /**< List of all channel info descriptors. */
    QStringList ch_names;       /**< List of all channel names. */
    FiffCoordTrans dev_head_t;  /**< Coordinate transformation ToDo... */
    FiffCoordTrans ctf_head_t;  /**< Coordinate transformation ToDo... */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffInfoBase::isEmpty() const
{
    return this->nchan <= 0;
}

//=============================================================================================================

inline bool operator== (const FiffInfoBase &a, const FiffInfoBase &b)
{
    return (a.filename == b.filename &&
            a.bads == b.bads &&
            //a.meas_id == b.meas_id &&
            a.nchan == b.nchan &&
            a.chs == b.chs &&
            a.ch_names == b.ch_names &&
            a.dev_head_t == b.dev_head_t &&
            a.ctf_head_t == b.ctf_head_t);
}
} // NAMESPACE

#endif // FIFF_INFO_BASE_H
