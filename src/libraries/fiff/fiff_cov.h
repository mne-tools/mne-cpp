//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_cov.h
 * @since October 2012
 * @brief Noise / data covariance matrix as stored under FIFFB_MNE_COV, with channel names, kind, projector list and rank info.
 *
 * @ref FiffCov is the C++ counterpart of MNE-Python's @c mne.Covariance.
 * It holds the (typically channel × channel) covariance matrix, the
 * channel-name list it is indexed by, the kind tag
 * (@c FIFFV_MNE_NOISE_COV / @c FIFFV_MNE_SOURCE_COV / ...), the diagonal
 * flag, the projector list that was active when the covariance was
 * estimated (@ref FiffProj), the bad-channel list, the nfree degrees of
 * freedom and an optional pre-whitening eigendecomposition. The class is
 * what @ref FiffStream reads from / writes to ``*-cov.fif`` files and
 * what the source-localization pipeline consumes to whiten the forward
 * gain matrix and the measurement data.
 */

#ifndef FIFF_COV_H
#define FIFF_COV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_proj.h"
#include "fiff_types.h"
#include "fiff_info.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawData;

//=============================================================================================================
/**
 * @brief FIFF noise / data covariance: matrix, channel names, kind, applied projectors, bads, dof and optional whitening eigendecomposition.
 *
 * Round-trips with ``*-cov.fif`` files and with @c mne.Covariance in
 * MNE-Python. Carries both the raw matrix and the metadata needed to
 * regularize, project away SSP subspaces, and pre-whiten downstream
 * forward / inverse computations.
 */
class FIFFSHARED_EXPORT FiffCov : public QSharedData
{
public:
    using SPtr = QSharedPointer<FiffCov>;            /**< Shared pointer type for FiffCov. */
    using ConstSPtr = QSharedPointer<const FiffCov>; /**< Const shared pointer type for FiffCov. */
    using UPtr = std::unique_ptr<FiffCov>;             /**< Unique pointer type for FiffCov. */
    using ConstUPtr = std::unique_ptr<const FiffCov>;  /**< Const unique pointer type for FiffCov. */
    using SDPtr = QSharedDataPointer<FiffCov>;       /**< Shared data pointer type for FiffCov. */

    //=========================================================================================================
    /**
     * Constructs the covariance data matrix.
     */
    FiffCov();

    //=========================================================================================================
    /**
     * Constructs a covariance data matrix, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the evoked data set.
     */
    FiffCov(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffCov   Covariance data matrix which should be copied.
     */
    FiffCov(const FiffCov &p_FiffCov);

    //=========================================================================================================
    /**
     * Destroys the covariance data matrix.
     */
    ~FiffCov();

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
    inline bool isEmpty() const;

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
    FiffCov pick_channels(const QStringList &p_include = defaultQStringList, const QStringList &p_exclude = defaultQStringList);

    //=========================================================================================================
    /**
     * Prepare noise covariance matrix. Before creating inverse operator.
     *
     * @param[in] p_info     measurement info.
     * @param[in] p_chNames  Channels which should be taken into account.
     *
     * @return the prepared noise covariance matrix.
     */
    FiffCov prepare_noise_cov(const FiffInfo& p_info, const QStringList& p_chNames) const;

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
    FiffCov regularize(const FiffInfo& p_info, double p_fMag = 0.1, double p_fGrad = 0.1, double p_fEeg = 0.1, bool p_bProj = true, QStringList p_exclude = defaultQStringList) const;

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
     * @param[in] removeMean    Whether to remove sample mean from the covariance estimate.
     * @param[in] ignoreMask    Bit mask ANDed away from event codes before matching (default: 0 = no masking).
     * @param[in] delay         Delay in seconds applied to the event sample before extracting the epoch (default: 0).
     *
     * @return The computed noise covariance matrix, or empty FiffCov on failure.
     */
    static FiffCov compute_from_epochs(const FiffRawData &raw,
                                       const Eigen::MatrixXi &events,
                                       const QList<int> &eventCodes,
                                       float tmin,
                                       float tmax,
                                       float bmin = 0.0f,
                                       float bmax = 0.0f,
                                       bool doBaseline = false,
                                       bool removeMean = true,
                                       unsigned int ignoreMask = 0,
                                       float delay = 0.0f);

    //=========================================================================================================
    /**
     * Save this covariance matrix to a FIFF file.
     *
     * @param[in] fileName  Output file path.
     * @return true on success.
     */
    bool save(const QString &fileName) const;

    //=========================================================================================================
    /**
     * Compute a weighted grand-average covariance from multiple covariance matrices,
     * weighting each by its degrees of freedom (nfree).
     *
     * @param[in] covs      List of covariance matrices to combine.
     * @return The grand-average covariance matrix, or empty FiffCov if covs is empty.
     */
    static FiffCov computeGrandAverage(const QList<FiffCov> &covs);

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     FiffCov which should be assigned.
     *
     * @return the copied covariance matrix.
     */
    FiffCov& operator= (const FiffCov &rhs);

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out           The stream to which the fiff covariance should be assigned to.
     * @param[in] p_FiffCov     FiffCov which should be assigned to the stream.
     *
     * @return the stream with the attached fiff covariance matrix.
     */
    friend std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffCov &p_FiffCov);

public:
    fiff_int_t  kind;       /**< Covariance kind -> fiff_constants.h. */
    Eigen::VectorXi chClass;    /**< Channel classification vector. */
    bool diag;              /**< If the covariance is stored in a diagonal order. */
    fiff_int_t dim;         /**< Dimension of the covariance (dim x dim). */
    QStringList names;      /**< Channel names. */
    Eigen::MatrixXd data;   /**< Covariance data. */
    QList<FiffProj> projs;  /**< List of available ssp projectors. */
    QStringList bads;       /**< List of bad channels. */
    fiff_int_t nfree;       /**< Number of degrees of freedom. */
    Eigen::VectorXd eig;    /**< Vector of eigenvalues. */
    Eigen::MatrixXd eigvec; /**< Matrix of eigenvectors (each row represents an eigenvector). */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffCov::isEmpty() const
{
    return this->dim <= -1;
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffCov &p_FiffCov)
{
    bool t_bIsShort = true;
    out << "#### Fiff Covariance ####\n";
    out << "\tKind: " << p_FiffCov.kind << std::endl;
    out << "\tdiag: " << p_FiffCov.diag << std::endl;
    out << "\tdim: " << p_FiffCov.dim << std::endl;
    out << "\tnames " << p_FiffCov.names.size() << ":\n\t";

    if(t_bIsShort)
    {
        qint32 nchan = p_FiffCov.names.size() > 6 ? 6 : p_FiffCov.names.size();
        for(qint32 i = 0; i < nchan/2; ++i)
            out << p_FiffCov.names[i].toUtf8().constData() << " ";
        out << "... ";
        for(qint32 i = p_FiffCov.names.size() - nchan/2; i < p_FiffCov.names.size(); ++i)
            out << p_FiffCov.names[i].toUtf8().constData() << " ";
        out << std::endl;
    }

    out << "\tdata " << p_FiffCov.data.rows() << " x " << p_FiffCov.data.cols() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nrows = p_FiffCov.data.rows() > 6 ? 6 : p_FiffCov.data.rows();
        qint32 ncols = p_FiffCov.data.cols() > 6 ? 6 : p_FiffCov.data.cols();
        for(qint32 i = 0; i < nrows/2; ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffCov.data(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffCov.data.cols() - ncols/2; j < p_FiffCov.data.cols(); ++j)
                out << p_FiffCov.data(i,j) << " ";
            out << "\n\t";
        }
        out << "...\n\t";
        for(qint32 i = p_FiffCov.data.rows()-nrows/2; i < p_FiffCov.data.rows(); ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffCov.data(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffCov.data.cols() - ncols/2; j < p_FiffCov.data.cols(); ++j)
                out << p_FiffCov.data(i,j) << " ";
            out << "\n\t";
        }
        out << "\n";
    }
    //Projectors
    out << "\tprojectors " << p_FiffCov.projs.size() << ":\n";
    for(qint32 i = 0; i < p_FiffCov.projs.size(); ++i)
        out << "\t" << p_FiffCov.projs[i];

    //Bads
    out << "\tbads " << p_FiffCov.bads.size() << ":\n\t";
    for(qint32 i = 0; i < p_FiffCov.bads.size(); ++i)
        out << p_FiffCov.bads[i].toUtf8().constData() << " ";

    out << "\n\tfree: " << p_FiffCov.nfree << std::endl;

    out << "\teig " << p_FiffCov.eig.size() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nrows = p_FiffCov.eig.size() > 6 ? 6 : p_FiffCov.eig.size();
        for(qint32 i = 0; i < nrows/2; ++i)
            out << p_FiffCov.eig[i] << " ";
        out << "... ";
        for(qint32 i = p_FiffCov.eig.size() - nrows/2; i < p_FiffCov.eig.size(); ++i)
            out << p_FiffCov.eig[i] << " ";
        out << "\n\t";
    }

    out << "\n\teigvec " << p_FiffCov.eigvec.rows() << " x " << p_FiffCov.eigvec.cols() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nrows = p_FiffCov.eigvec.rows() > 6 ? 6 : p_FiffCov.eigvec.rows();
        qint32 ncols = p_FiffCov.eigvec.cols() > 6 ? 6 : p_FiffCov.eigvec.cols();
        for(qint32 i = 0; i < nrows/2; ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffCov.eigvec(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffCov.eigvec.cols() - ncols/2; j < p_FiffCov.eigvec.cols(); ++j)
                out << p_FiffCov.eigvec(i,j) << " ";
            out << "\n\t";
        }
        out << "...\n\t";
        for(qint32 i = p_FiffCov.eigvec.rows() - nrows/2; i < p_FiffCov.eigvec.rows(); ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffCov.eigvec(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffCov.eigvec.cols() - ncols/2; j < p_FiffCov.eigvec.cols(); ++j)
                out << p_FiffCov.eigvec(i,j) << " ";
            out << "\n\t";
        }
        out << "\n";
    }
    return out;
}
} // NAMESPACE

#ifndef metatype_fiffcovsptr
#define metatype_fiffcovsptr
Q_DECLARE_METATYPE(QSharedPointer<FIFFLIB::FiffCov>); /**< Provides QT META type declaration of the QSharedPointer<FIFFLIB::FiffCov> type. For signal/slot usage.*/
#endif

#ifndef metatype_fiffcov
#define metatype_fiffcov
Q_DECLARE_METATYPE(FIFFLIB::FiffCov); /**< Provides QT META type declaration of the FIFFLIB::FiffCov type. For signal/slot usage.*/
#endif

#endif // FIFF_COV_H
