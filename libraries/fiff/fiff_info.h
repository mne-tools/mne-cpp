//=============================================================================================================
/**
 * @file     fiff_info.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
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
 * @brief    FiffInfo class declaration.
 *
 */

#ifndef FIFF_INFO_H
#define FIFF_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

#include "fiff_info_base.h"

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
// FIFFLIB FORWARD DECLARATIONS
//=============================================================================================================

class FiffStream;

//=============================================================================================================
/**
 * Provides fiff measurement file information
 *
 * @brief FIFF measurement file information
 */
class FIFFSHARED_EXPORT FiffInfo : public FiffInfoBase
{
public:
    typedef QSharedPointer<FiffInfo> SPtr;              /**< Shared pointer type for FiffInfo. */
    typedef QSharedPointer<const FiffInfo> ConstSPtr;   /**< Const shared pointer type for FiffInfo. */

    //=========================================================================================================
    /**
     * Constructors the fiff measurement file information.
     */
    FiffInfo();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffInfo  FIFF measurement information which should be copied.
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
     * mne_make_compensator
     *
     * ### MNE toolbox root function ### Definition of the mne_make_compensator function
     *
     * Create a compensation matrix to bring the data from one compensation state to another
     *
     * @param[in] from               compensation in the input data.
     * @param[in] to                 desired compensation in the output.
     * @param[out] ctf_comp          Compensation Matrix.
     * @param[in] exclude_comp_chs   exclude compensation channels from the output (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    bool make_compensator(fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs = false) const;

    //=========================================================================================================
    /**
     * mne_get_current_comp
     *
     * ### MNE toolbox root function ### Definition of the mne_get_current_comp function
     *
     * Get the current compensation in effect in the data
     *
     * @return the current compensation.
     */
    qint32 get_current_comp();

    //=========================================================================================================
    /**
     * mne_make_projector_info
     *
     * ### MNE toolbox root function ###  Definition of the mne_make_projector_info function
     *
     * Make a SSP operator using the meas info
     *
     * @param[out] proj      The projection operator to apply to the data.
     *
     * @return nproj - How many items in the projector.
     */
    inline qint32 make_projector(Eigen::MatrixXd& proj) const;

    //=========================================================================================================
    /**
     * mne_make_projector_info
     *
     * ### MNE toolbox root function ###  Definition of the mne_make_projector_info function
     *
     * Make a SSP operator using the meas info
     *
     * @param[out] proj      The projection operator to apply to the data.
     * @param[in] p_chNames   List of channels to include in the projection matrix.
     *
     * @return nproj - How many items in the projector.
     */
    inline qint32 make_projector(Eigen::MatrixXd& proj, const QStringList& p_chNames) const;

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
    FiffInfo pick_info(const Eigen::RowVectorXi &sel = defaultVectorXi) const;

    //=========================================================================================================
    /**
     * Set the current compensation value in the channel info structures
     *
     * @param[in] value  compensation value.
     */
    inline void set_current_comp(fiff_int_t value);

    //=========================================================================================================
    /**
     * mne_set_current_comp
     *
     * ### MNE toolbox root function ### Definition of the mne_set_current_comp function
     *
     * Consider taking the member function of set_current_comp(fiff_int_t value),
     * when compensation should be applied to the channels of FiffInfo
     *
     * Set the current compensation value in the channel info structures
     *
     * @param[in] chs    fiff channel info list.
     * @param[in] value  compensation value.
     *
     * @return the current compensation.
     */
    static QList<FiffChInfo> set_current_comp(QList<FiffChInfo>& listFiffChInfo, fiff_int_t value);

// ToDo
//    //=========================================================================================================
//    /**
//    * Writes the fiff information to an I/O Device, e.g. fiff file
//    *
//    * @param[in] p_IODevice   IO device to write the fiff info to.
//    */
//    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Writes the fiff information to a FIF stream.
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FiffStream* p_pStream) const;

    //=========================================================================================================
    /**
     * Prints class contents.
     */
    void print() const;

private:
    //=========================================================================================================
    /**
     * function this_comp = make_compensator(info,kind)
     *
     * Create a compensation matrix to bring the data from one compensation state to another
     *
     * @param[in] kind               Compensation in the input data.
     * @param[out] comp              Compensation Matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    bool make_compensator(fiff_int_t kind, Eigen::MatrixXd& this_comp) const;

public: //Public because it's a mne struct
    FiffId file_id;                 /**< File ID. */
    fiff_int_t  meas_date[2];       /**< Measurement date. TODO: use fiffTime instead to be MNE-C consistent*/
    float sfreq;                    /**< Sample frequency. */
    float linefreq;                 /**< Power line frequency. */
    float highpass;                 /**< Highpass frequency. */
    float lowpass;                  /**< Lowpass frequency. */
    int proj_id;                    /**< Project ID. */
    QString proj_name;              /**< Project name. */
    QString xplotter_layout;        /**< xplotter layout tag. */
    QString experimenter;           /**< Experimenter name. */
    QString description;            /**< (Textual) Description of an object.*/
    QString utc_offset;             /**< UTC offset of related meas_date (sHH:MM).*/
    fiff_int_t gantry_angle;        /**< Tilt angle of the dewar in degrees.*/
    FiffCoordTrans dev_ctf_t;       /**< Coordinate transformation ToDo... */
    QList<FiffDigPoint> dig;        /**< List of all digitization point descriptors. */
    FiffCoordTrans dig_trans;       /**< Coordinate transformation ToDo... */
    QList<FiffProj> projs;          /**< List of available SSP projectors. */
    QList<FiffCtfComp> comps;       /**< List of available CTF software compensators. */
    QString acq_pars;               /**< Acquisition information ToDo... */
    QString acq_stim;               /**< Acquisition information ToDo... */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffInfo::make_projector(Eigen::MatrixXd& proj) const
{
    return FiffProj::make_projector(this->projs,this->ch_names, proj, this->bads);
}

//=============================================================================================================

inline qint32 FiffInfo::make_projector(Eigen::MatrixXd& proj, const QStringList& p_chNames) const
{
    return FiffProj::make_projector(this->projs, p_chNames, proj, this->bads);
}

//=============================================================================================================

inline void FiffInfo::set_current_comp(fiff_int_t value)
{
    this->chs = set_current_comp(this->chs, value);
}
} // NAMESPACE

#endif // FIFF_INFO_H
