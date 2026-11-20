//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_cov.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Noise / data covariance matrix as stored under FIFFB_MNE_COV, with channel names, kind, projector list and rank info.
 *
 * @ref mne::FiffCov is the C++ counterpart of MNE-Python's @c mne.Covariance.
 * It holds the (typically channel × channel) covariance matrix, the
 * channel-name list it is indexed by, the kind tag
 * (@c FIFFV_MNE_NOISE_COV / @c FIFFV_MNE_SOURCE_COV / ...), the diagonal
 * flag, the projector list that was active when the covariance was
 * estimated (@ref mne::FiffProj), the bad-channel list, the nfree degrees of
 * freedom and an optional pre-whitening eigendecomposition. The class is
 * what @ref mne::FiffStream reads from / writes to ``*-cov.fif`` files and
 * what the source-localization pipeline consumes to whiten the forward
 * gain matrix and the measurement data.
 */

#ifndef FIFF_COV_H
#define FIFF_COV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

#include <vector>
#include "fiff_proj.h"
#include "fiff_types.h"
#include "fiff_info.h"

#include <base/error.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <expected>
#include <filesystem>
#include <memory>
#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawData;
struct RejectionParams;

//=============================================================================================================
/**
 * @brief FIFF noise / data covariance: matrix, channel names, kind, applied projectors, bads, dof and optional whitening eigendecomposition.
 *
 * Round-trips with ``*-cov.fif`` files and with @c mne.Covariance in
 * MNE-Python. Carries both the raw matrix and the metadata needed to
 * regularize, project away SSP subspaces, and pre-whiten downstream
 * forward / inverse computations.
 *
 * @snippet ex_fiff_api/main.cpp fiff_cov_usage
 */
class FIFFSHARED_EXPORT FiffCov
{
public:
    //=========================================================================================================
    /**
     * Constructs the covariance data matrix.
     */
    FiffCov();

    //=========================================================================================================
    /**
     * Reads the noise covariance of a FIFF file, like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The file.
     */
    explicit FiffCov(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the noise covariance stored in a FIFF file, like mne.read_cov.
     *
     * @param[in] path   The file.
     *
     * @return The covariance, or why the file holds none.
     */
    [[nodiscard]] static mne::Expected<FiffCov> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Initializes the covariance data matrix.
     */
    void clear();

    //=========================================================================================================
    /**
     * True if FIFF covariance is empty.
     *
     * @return true if FIFF covariance is empty.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * python pick_channels_cov
     *
     * Pick channels from covariance matrix
     *
     * @param[in] p_include  List of channels to include (if empty, include all available). (optional).
     * @param[in] p_exclude  Channels to exclude (if empty, do not exclude any). (optional).
     *
     * @return Covariance solution restricted to selected channels.
     */
    FiffCov pick_channels(const StringList& p_include = {}, const StringList& p_exclude = {});

    //=========================================================================================================
    /**
     * Prepare noise covariance matrix. Before creating inverse operator.
     *
     * @param[in] p_info     measurement info.
     * @param[in] p_chNames  Channels which should be taken into account.
     *
     * @return the prepared noise covariance matrix.
     */
    FiffCov prepare_noise_cov(const FiffInfo& p_info, const StringList& p_chNames) const;

    //=========================================================================================================
    /**
     * Regularize noise covariance matrix
     *
     * This method works by adding a constant to the diagonal for each channel type separatly.
     * Special care is taken to keep the rank of the data constant.
     *
     * @param[in] p_info     The measurement info (used to get channel types and bad channels).
     * @param[in] p_fMag      Regularization factor for MEG magnetometers.
     * @param[in] p_fGrad     Regularization factor for MEG gradiometers.
     * @param[in] p_fEeg      Regularization factor for EEG.
     * @param[in] p_bProj     Apply or not projections to keep rank of data.
     * @param[in] p_exclude  List of channels to mark as bad. If None, bads channels are extracted from both info['bads'] and cov['bads'].
     *
     * @return the regularized covariance matrix.
     */
    FiffCov regularize(const FiffInfo& p_info, double p_fMag = 0.1, double p_fGrad = 0.1, double p_fEeg = 0.1, bool p_bProj = true, StringList p_exclude = {}) const;

    //=========================================================================================================
    /**
     * Compute a noise covariance matrix from raw data based on event-locked epochs.
     * Ported from compute_cov.c (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[in] events        Event matrix (nEvents x 3): [sample, before, after].
     * @param[in] eventCodes    Which event codes to include.
     * @param[in] tmin          Start of time window relative to event (seconds).
     * @param[in] tmax          End of time window relative to event (seconds).
     * @param[in] bmin          Baseline start (seconds, relative to event). Only used if doBaseline is true.
     * @param[in] bmax          Baseline end (seconds, relative to event). Only used if doBaseline is true.
     * @param[in] doBaseline    Whether to apply baseline correction before covariance computation.
     * @param[in] removeMean    Whether to remove each event code's average response from the epochs (mne keep_sample_mean=False).
     * @param[in] ignoreMask    Bit mask ANDed away from event codes before matching (default: 0 = no masking).
     * @param[in] delay         Delay in seconds applied to the event sample before extracting the epoch (default: 0).
     * @param[in] rej           If given, epochs failing these limits (before baseline, as MNE-C compute_cov) are skipped.
     *
     * @return The computed noise covariance matrix, or empty FiffCov on failure.
     */
    static FiffCov compute_from_epochs(const FiffRawData& raw,
                                       const Eigen::MatrixXi& events,
                                       const std::vector<int>& eventCodes,
                                       float tmin,
                                       float tmax,
                                       float bmin = 0.0f,
                                       float bmax = 0.0f,
                                       bool doBaseline = false,
                                       bool removeMean = true,
                                       unsigned int ignoreMask = 0,
                                       float delay = 0.0f,
                                       const RejectionParams* rej = nullptr);

    //=========================================================================================================
    /**
     * Compute a noise covariance matrix from epochs that are already in memory, e.g. filtered ones.
     *
     * @param[in] epochs        The epochs (channels x samples), grouped by event code.
     * @param[in] info          Measurement info whose channels the epoch rows are.
     * @param[in] removeMean    Whether to remove each group's average response (mne keep_sample_mean=False).
     *
     * @return The computed noise covariance matrix, or empty FiffCov on failure.
     */
    static FiffCov compute_from_epochs(const std::vector<std::vector<Eigen::MatrixXd>>& epochs,
                                       const FiffInfo& info,
                                       bool removeMean = true);

    //=========================================================================================================
    /**
     * Writes the covariance to a new FIFF file, like mne.Covariance.save.
     *
     * @param[in] path   Output file.
     *
     * @return Nothing, or why the file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> save(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Compute a weighted grand-average covariance from multiple covariance matrices,
     * weighting each by its degrees of freedom (nfree).
     *
     * @param[in] covs      List of covariance matrices to combine.
     * @return The grand-average covariance matrix, or empty FiffCov if covs is empty.
     */
    static FiffCov compute_grand_average(const std::vector<FiffCov>& covs);

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out           The stream to which the fiff covariance should be assigned to.
     * @param[in] p_FiffCov     FiffCov which should be assigned to the stream.
     *
     * @return the stream with the attached fiff covariance matrix.
     */
    friend std::ostream& operator<<(std::ostream& out, const mne::FiffCov& p_FiffCov);

public:
    fiff_int_t kind;             /**< Covariance kind -> fiff_constants.h. */
    Eigen::VectorXi chClass;     /**< Channel classification vector. */
    bool diag;                   /**< If the covariance is stored in a diagonal order. */
    fiff_int_t dim;              /**< Dimension of the covariance (dim x dim). */
    StringList names;            /**< Channel names. */
    Eigen::MatrixXd data;        /**< Covariance data. */
    std::vector<FiffProj> projs; /**< List of available ssp projectors. */
    StringList bads;             /**< List of bad channels. */
    fiff_int_t nfree;            /**< Number of degrees of freedom. */
    Eigen::VectorXd eig;         /**< Vector of eigenvalues. */
    Eigen::MatrixXd eigvec;      /**< Matrix of eigenvectors (each row represents an eigenvector). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffCov::empty() const
{
    return this->dim <= -1;
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const mne::FiffCov& p_FiffCov)
{
    bool t_bIsShort = true;
    out << "#### Fiff Covariance ####\n";
    out << "\tKind: " << p_FiffCov.kind << std::endl;
    out << "\tdiag: " << p_FiffCov.diag << std::endl;
    out << "\tdim: " << p_FiffCov.dim << std::endl;
    out << "\tnames " << p_FiffCov.names.size() << ":\n\t";

    if (t_bIsShort) {
        const int nchan = std::min(static_cast<int>(p_FiffCov.names.size()), 6);
        for (std::int32_t i = 0; i < nchan / 2; ++i)
            out << p_FiffCov.names[i].c_str() << " ";
        out << "... ";
        for (std::int32_t i = static_cast<int>(p_FiffCov.names.size()) - nchan / 2; i < static_cast<int>(p_FiffCov.names.size()); ++i)
            out << p_FiffCov.names[i].c_str() << " ";
        out << std::endl;
    }

    out << "\tdata " << p_FiffCov.data.rows() << " x " << p_FiffCov.data.cols() << ":\n\t";
    if (t_bIsShort) {
        Eigen::Index nrows = p_FiffCov.data.rows() > 6 ? 6 : p_FiffCov.data.rows();
        Eigen::Index ncols = p_FiffCov.data.cols() > 6 ? 6 : p_FiffCov.data.cols();
        for (Eigen::Index i = 0; i < nrows / 2; ++i) {
            for (Eigen::Index j = 0; j < ncols / 2; ++j)
                out << p_FiffCov.data(i, j) << " ";
            out << "... ";
            for (Eigen::Index j = p_FiffCov.data.cols() - ncols / 2; j < p_FiffCov.data.cols(); ++j)
                out << p_FiffCov.data(i, j) << " ";
            out << "\n\t";
        }
        out << "...\n\t";
        for (Eigen::Index i = p_FiffCov.data.rows() - nrows / 2; i < p_FiffCov.data.rows(); ++i) {
            for (Eigen::Index j = 0; j < ncols / 2; ++j)
                out << p_FiffCov.data(i, j) << " ";
            out << "... ";
            for (Eigen::Index j = p_FiffCov.data.cols() - ncols / 2; j < p_FiffCov.data.cols(); ++j)
                out << p_FiffCov.data(i, j) << " ";
            out << "\n\t";
        }
        out << "\n";
    }
    //Projectors
    out << "\tprojectors " << p_FiffCov.projs.size() << ":\n";
    for (Eigen::Index i = 0; i < std::ssize(p_FiffCov.projs); ++i)
        out << "\t" << p_FiffCov.projs[i];

    //Bads
    out << "\tbads " << p_FiffCov.bads.size() << ":\n\t";
    for (Eigen::Index i = 0; i < static_cast<int>(p_FiffCov.bads.size()); ++i)
        out << p_FiffCov.bads[i].c_str() << " ";

    out << "\n\tfree: " << p_FiffCov.nfree << std::endl;

    out << "\teig " << p_FiffCov.eig.size() << ":\n\t";
    if (t_bIsShort) {
        Eigen::Index nrows = p_FiffCov.eig.size() > 6 ? 6 : p_FiffCov.eig.size();
        for (Eigen::Index i = 0; i < nrows / 2; ++i)
            out << p_FiffCov.eig[i] << " ";
        out << "... ";
        for (Eigen::Index i = p_FiffCov.eig.size() - nrows / 2; i < p_FiffCov.eig.size(); ++i)
            out << p_FiffCov.eig[i] << " ";
        out << "\n\t";
    }

    out << "\n\teigvec " << p_FiffCov.eigvec.rows() << " x " << p_FiffCov.eigvec.cols() << ":\n\t";
    if (t_bIsShort) {
        Eigen::Index nrows = p_FiffCov.eigvec.rows() > 6 ? 6 : p_FiffCov.eigvec.rows();
        Eigen::Index ncols = p_FiffCov.eigvec.cols() > 6 ? 6 : p_FiffCov.eigvec.cols();
        for (Eigen::Index i = 0; i < nrows / 2; ++i) {
            for (Eigen::Index j = 0; j < ncols / 2; ++j)
                out << p_FiffCov.eigvec(i, j) << " ";
            out << "... ";
            for (Eigen::Index j = p_FiffCov.eigvec.cols() - ncols / 2; j < p_FiffCov.eigvec.cols(); ++j)
                out << p_FiffCov.eigvec(i, j) << " ";
            out << "\n\t";
        }
        out << "...\n\t";
        for (Eigen::Index i = p_FiffCov.eigvec.rows() - nrows / 2; i < p_FiffCov.eigvec.rows(); ++i) {
            for (Eigen::Index j = 0; j < ncols / 2; ++j)
                out << p_FiffCov.eigvec(i, j) << " ";
            out << "... ";
            for (Eigen::Index j = p_FiffCov.eigvec.cols() - ncols / 2; j < p_FiffCov.eigvec.cols(); ++j)
                out << p_FiffCov.eigvec(i, j) << " ";
            out << "\n\t";
        }
        out << "\n";
    }
    return out;
}
} // NAMESPACE

#endif // FIFF_COV_H
