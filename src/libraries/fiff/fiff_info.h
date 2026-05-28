//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    Full FIFF measurement metadata: everything from FIFFB_MEAS / FIFFB_MEAS_INFO needed to interpret a recording.
 *
 * @ref FiffInfo is the C++ counterpart of MNE-Python's
 * @c mne.Info dictionary. It collects everything stored under
 * @c FIFFB_MEAS / @c FIFFB_MEAS_INFO of a FIFF file: per-channel
 * descriptors (@ref FiffChInfo via @ref FiffInfoBase), sampling frequency,
 * lowpass / highpass filter cutoffs, line frequency, measurement date,
 * subject info, experimenter, projector list (@ref FiffProj), CTF
 * software compensation list (@ref FiffCtfComp), HPI fit results, the
 * device→head and CTF compensation transforms, the @c FIFFB_ISOTRAK
 * digitization (@ref FiffDigPoint), and the per-channel calibration vectors.
 *
 * Every higher-level container in FIFFLIB (@ref FiffRawData,
 * @ref FiffEvoked, @ref FiffCov, @ref FiffEvokedSet) carries a
 * @ref FiffInfo so that downstream consumers (filtering, source
 * localization, plotting, BIDS export) can interpret the data without
 * re-reading the file.
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
#include <memory>

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
 * @brief Full FIFF measurement info: per-channel descriptors, sampling and filter setup, projectors, compensators, transforms and dig points.
 *
 * Materializes the union of the tags found in @c FIFFB_MEAS /
 * @c FIFFB_MEAS_INFO: @c sfreq, @c lowpass / @c highpass / @c line_freq,
 * @c meas_date, @c experimenter, @c projs (@ref FiffProj),
 * @c comps (@ref FiffCtfComp), @c dig (@ref FiffDigPoint),
 * @c hpi_results / @c hpi_meas, @c dev_head_t / @c ctf_head_t, plus the
 * @ref FiffInfoBase channel / bads / sfreq subset. Drop-in counterpart of
 * @c mne.Info in MNE-Python.
 */
class FIFFSHARED_EXPORT FiffInfo : public FiffInfoBase
{
public:
    using SPtr = QSharedPointer<FiffInfo>;            /**< Shared pointer type for FiffInfo. */
    using ConstSPtr = QSharedPointer<const FiffInfo>; /**< Const shared pointer type for FiffInfo. */
    using UPtr = std::unique_ptr<FiffInfo>;             /**< Unique pointer type for FiffInfo. */
    using ConstUPtr = std::unique_ptr<const FiffInfo>;  /**< Const unique pointer type for FiffInfo. */

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
     * Get the current compensation in effect in the data
     *
     * @return the current compensation.
     */
    qint32 get_current_comp();

    //=========================================================================================================
    /**
     * mne_make_projector_info
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

    //=========================================================================================================
    /**
     * Read MEG and EEG channel information from a FIFF file, excluding bad channels.
     *
     * Opens the file, reads measurement info, and splits channels into MEG and EEG
     * lists (in that order), skipping channels in the bads list. EEG channels must
     * pass FiffChInfo::isValidEeg().
     *
     * @param[in]  name    Path to the FIFF measurement file.
     * @param[in]  do_meg  If true, include MEG channels.
     * @param[in]  do_eeg  If true, include EEG channels.
     * @param[in]  bads    List of bad channel names to exclude.
     * @param[out] chsp    Combined list of accepted channels (MEG first, then EEG).
     * @param[out] nmegp   Number of MEG channels in chsp.
     * @param[out] neegp   Number of EEG channels in chsp.
     * @return true on success, false on error.
     */
    static bool readMegEegChannels(const QString& name,
                                   bool do_meg,
                                   bool do_eeg,
                                   const QStringList& bads,
                                   QList<FiffChInfo>& chsp,
                                   int& nmegp,
                                   int& neegp);

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
    fiff_int_t  meas_date[2];       /**< Measurement date. */
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
    FiffCoordTrans dev_ctf_t;       /**< Device to CTF coordinate transformation. */
    QList<FiffDigPoint> dig;        /**< List of all digitization point descriptors. */
    FiffCoordTrans dig_trans;       /**< Digitizer coordinate transformation. */
    QList<FiffProj> projs;          /**< List of available SSP projectors. */
    QList<FiffCtfComp> comps;       /**< List of available CTF software compensators. */
    QString acq_pars;               /**< Acquisition parameters. */
    QString acq_stim;               /**< Acquisition stimulus information. */
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
