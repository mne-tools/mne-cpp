//=============================================================================================================
/**
* @file     fiff_info.h
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
* @brief    Contains the FiffInfo class declaration.
*
*/

#ifndef FIFF_INFO_H
#define FIFF_INFO_H

//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QStringList>
#include <QSharedData>


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


//=============================================================================================================
/**
* Provides fiff measurement file information
*
* @brief FIFF measurement file information
*/
class FIFFSHARED_EXPORT FiffInfo : public QSharedData
{
public:
    typedef QSharedPointer<FiffInfo> SPtr;              /**< Shared pointer type for FiffInfo. */
    typedef QSharedPointer<const FiffInfo> ConstSPtr;   /**< Const shared pointer type for FiffInfo. */
    typedef QSharedDataPointer<FiffInfo> SDPtr;         /**< Shared data pointer type for FiffNamedMatrix. */

    //=========================================================================================================
    /**
    * Constructors the fiff measurement file information.
    */
    FiffInfo();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffInfo  FIFF measurement information which should be copied
    */
    FiffInfo(const FiffInfo& p_FiffInfo);

    //=========================================================================================================
    /**
    * Destroys the fiff measurement file information.
    */
    ~FiffInfo();

    //=========================================================================================================
    /**
    * Initializes FIFF measurement information.
    */
    void clear();

    //=========================================================================================================
    /**
    * True if FIFF measurement file information is empty.
    *
    * @return true if FIFF measurement file information is empty
    */
    inline bool isEmpty() const
    {
        return this->nchan <= 0;
    }

    //=========================================================================================================
    /**
    * mne_make_compensator
    *
    * ### MNE toolbox root function ### Implementation of the mne_make_compensator function
    *
    * Create a compensation matrix to bring the data from one compensation state to another
    *
    * @param[in] from               compensation in the input data
    * @param[in] to                 desired compensation in the output
    * @param[out] comp              Compensation Matrix
    * @param[in] exclude_comp_chs   exclude compensation channels from the output (optional)
    *
    * @return true if succeeded, false otherwise
    */
    bool make_compensator(fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs = false) const;


    //=========================================================================================================
    /**
    * mne_get_current_comp
    *
    * ### MNE toolbox root function ### Implementation of the mne_get_current_comp function
    *
    * Get the current compensation in effect in the data
    *
    * @return the current compensation
    */
    qint32 get_current_comp();


    //=========================================================================================================
    /**
    * mne_make_projector
    *
    * ### MNE toolbox root function ### Implementation of the mne_make_projector function
    *
    * Make an SSP operator
    *
    * @param[in] projs      A set of projection vectors
    * @param[in] ch_names   A cell array of channel names
    * @param[out] proj      The projection operator to apply to the data
    * @param[in] bads       Bad channels to exclude
    * @param[out] U         The orthogonal basis of the projection vectors (optional)
    *
    * @return nproj - How many items in the projector
    */
    static fiff_int_t make_projector(QList<FiffProj>& projs, QStringList& ch_names, MatrixXd& proj, QStringList& bads = defaultQStringList, MatrixXd& U = defaultMatrixXd);


    //=========================================================================================================
    /**
    * mne_make_projector_info
    *
    * ### MNE toolbox root function ###  Implementation of the mne_make_projector_info function
    *
    * Make a SSP operator using the meas info
    *
    * @param[in] info       Fiff measurement info
    * @param[out] proj      The projection operator to apply to the data
    *
    * @return nproj - How many items in the projector
    */
    inline qint32 make_projector_info(MatrixXd& proj)
    {
        return make_projector(this->projs,this->ch_names, proj, this->bads);
    }


    //=========================================================================================================
    /**
    * fiff_pick_channels
    *
    * ### MNE toolbox root function ###
    *
    * Make a selector to pick desired channels from data
    *
    * @param[in] ch_names  - The channel name list to consult
    * @param[in] include   - Channels to include (if empty, include all available)
    * @param[in] exclude   - Channels to exclude (if empty, do not exclude any)
    *
    * @return the selector matrix (row Vector)
    */
    static MatrixXi pick_channels(QStringList& ch_names, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);

    //=========================================================================================================
    /**
    * fiff_pick_info
    *
    * ### MNE toolbox root function ###
    *
    * Pick desired channels from measurement info
    *
    * @param[in] sel    List of channels to select
    *
    * @return Info modified according to sel
    */
    FiffInfo pick_info(const MatrixXi* sel = NULL) const;

    //=========================================================================================================
    /**
    * fiff_pick_types
    *
    * ### MNE toolbox root function ###
    *
    * Create a selector to pick desired channel types from data
    *
    * @param[in] meg        Include MEG channels
    * @param[in] eeg        Include EEG channels
    * @param[in] stim       Include stimulus channels
    * @param[in] include    Additional channels to include (if empty, do not add any)
    * @param[in] exclude    Channels to exclude (if empty, do not exclude any)
    *
    * @return the selector matrix (row vector)
    */
    MatrixXi pick_types(bool meg, bool eeg = false, bool stim = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);

    //=========================================================================================================
    /**
    * Set the current compensation value in the channel info structures
    *
    * @param[in] value  compensation value
    */
    inline void set_current_comp(fiff_int_t value)
    {
        this->chs = set_current_comp(this->chs, value);
    }


    //=========================================================================================================
    /**
    * mne_set_current_comp
    *
    * ### MNE toolbox root function ### Implementation of the mne_set_current_comp function
    *
    * Consider taking the member function of set_current_comp(fiff_int_t value),
    * when compensation should be applied to the channels of FiffInfo
    *
    * Set the current compensation value in the channel info structures
    *
    * @param[in] chs    fiff channel info list
    * @param[in] value  compensation value
    *
    * @return the current compensation
    */
    static QList<FiffChInfo> set_current_comp(QList<FiffChInfo>& chs, fiff_int_t value);

private:

    //=========================================================================================================
    /**
    * function this_comp = make_compensator(info,kind)
    *
    * Create a compensation matrix to bring the data from one compensation state to another
    *
    * @param[in] kind               Compensation in the input data
    * @param[out] comp              Compensation Matrix
    *
    * @return true if succeeded, false otherwise
    */
    bool make_compensator(fiff_int_t kind, MatrixXd& this_comp) const;


public: //Public because it's a mne struct
    FiffId      file_id;        /**< File ID. */
    FiffId      meas_id;        /**< Measurement ID. */
    fiff_int_t  meas_date[2];   /**< Measurement date. */
    fiff_int_t  nchan;          /**< Number of channels. */
    float sfreq;                /**< Sample frequency. */
    float highpass;             /**< Highpass frequency. */
    float lowpass;              /**< Lowpass frequency. */
    QList<FiffChInfo> chs;      /**< List of all channel info descriptors. */
    QStringList ch_names;       /**< List of all channel names. */
    FiffCoordTrans dev_head_t;  /**< Coordinate transformation ToDo... */
    FiffCoordTrans ctf_head_t;  /**< Coordinate transformation ToDo... */
    FiffCoordTrans dev_ctf_t;   /**< Coordinate transformation ToDo... */
    QList<FiffDigPoint> dig;    /**< List of all digitization point descriptors. */
    FiffCoordTrans dig_trans;   /**< Coordinate transformation ToDo... */
    QStringList bads;           /**< List of bad channels. */
    QList<FiffProj> projs;      /**< List of available SSP projectors. */
    QList<FiffCtfComp> comps;   /**< List of available CTF software compensators. */
    QString acq_pars;           /**< Acquisition information ToDo... */
    QString acq_stim;           /**< Acquisition information ToDo... */
    QString filename;           /**< Filename when the info is read of a fiff file. */
};

} // NAMESPACE

#endif // FIFF_INFO_H
