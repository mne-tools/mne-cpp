//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_inverse_operator.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Pre-computed inverse operator (whitened SVD of the forward model) for MNE/dSPM/sLORETA.
 *
 * @ref mne::MNEInverseOperator is the C++ port of @c -inv.fif: it
 * stores the noise covariance, the source covariance, the orientation
 * priors and the left/right singular vectors needed to assemble the
 * Tikhonov-regularised inverse for a given SNR and method (MNE, dSPM,
 * sLORETA). It composes the forward solution (@ref mne::MNEForwardSolution),
 * covariance (@ref mne::FiffCov) and source space
 * (@ref mne::MNESourceSpaces) into a single object that
 * @c mne::MinimumNorm in mne_inv consumes to produce
 * @c MNESourceEstimate time courses.
 */

#ifndef MNE_INVERSE_OPERATOR_H
#define MNE_INVERSE_OPERATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_source_spaces.h"
#include "mne_forward_solution.h"
#include <fiff/fiff_types.h>
#include <base/error.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FsLabel;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=========================================================================================================
/**
 * @brief Output of a multi-threaded KMeans clustering step for a single cortical region.
 */
struct RegionMTOut
{
    Eigen::VectorXi roiIdx;    /**< Per-source cluster assignment indices. */
    Eigen::MatrixXd ctrs;      /**< Cluster centroids (nClusters x features). */
    Eigen::VectorXd sumd;      /**< Sum of distances to the assigned centroid. */
    Eigen::MatrixXd D;         /**< Distance matrix (nSources x nClusters). */
    std::int32_t iLabelIdxOut; /**< Label index carried through from the input. */
};

//=========================================================================================================
/**
 * @brief Input parameters for multi-threaded KMeans clustering on a single cortical region.
 */
struct RegionMT
{
    Eigen::MatrixXd matRoiMT;     /**< Reshaped gain matrix (sources x sensors*3). */
    Eigen::MatrixXd matRoiMTOrig; /**< Original gain matrix (sensors x sources*3). */
    std::int32_t nClusters;       /**< Number of clusters for this region. */
    Eigen::VectorXi idcs;         /**< Source-space indices for this region. */
    std::int32_t iLabelIdxIn;     /**< Label index passed through to the output. */
    std::string sDistMeasure;     /**< Distance metric: "cityblock" or "sqeuclidean". */

    /**
     * @brief Run KMeans clustering on this region.
     * @return Clustering result including centroid positions and assignments.
     */
    RegionMTOut cluster() const
    {
        const std::string distMeasure = sDistMeasure.empty() ? std::string("cityblock") : sDistMeasure;

        RegionMTOut out;
        mne::KMeans kMeans(distMeasure, "sample", 5);
        kMeans.calculate(matRoiMT, nClusters,
                         out.roiIdx, out.ctrs, out.sumd, out.D);
        out.iLabelIdxOut = iLabelIdxIn;
        return out;
    }
};

//=============================================================================================================
/**
 * @brief MNE-style inverse operator.
 *
 * Encapsulates the SVD decomposition of the whitened and weighted lead-field
 * matrix together with noise and source covariance, priors, and the source
 * space.  Supports reading/writing FIFF files, preparation for application
 * (regularisation, whitening, noise normalisation), and gain-matrix clustering.
 *
 * @snippet ex_inv_api/main.cpp inv_minimum_norm_usage
 *
 * @snippet ex_inv_api/main.cpp mne_inverse_cluster_kernel
 */
class MNESHARED_EXPORT MNEInverseOperator
{
public:
    //=========================================================================================================
    /**
     * @brief Constructs an empty inverse operator with invalid sentinel values.
     */
    MNEInverseOperator() = default;

    //=========================================================================================================
    /**
     * Reads an inverse operator file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The -inv.fif file.
     */
    explicit MNEInverseOperator(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * @brief Constructs an inverse operator from measurement info, forward solution, and noise covariance.
     *
     * @param[in] info               Measurement info specifying channels; bad channels in info.bads are excluded.
     * @param[in] forward            Forward operator.
     * @param[in] noiseCov           Noise covariance matrix.
     * @param[in] loose              Tangential-component weight in [0, 1] (ignored when @p fixed is true).
     * @param[in] depth              Depth-weighting exponent in [0, 1]; 0 disables depth weighting.
     * @param[in] fixed              If true, use fixed source orientations normal to the cortical mantle.
     * @param[in] limit_depth_chs    If true, restrict depth weighting to gradiometers (or magnetometers/EEG as fallback).
     */
    MNEInverseOperator(const mne::FiffInfo& info,
                       const MNEForwardSolution& forward,
                       const mne::FiffCov& noiseCov,
                       float loose = 0.2f,
                       float depth = 0.8f,
                       bool fixed = false,
                       bool limit_depth_chs = true);

    //=========================================================================================================
    /**
     * @brief Assemble the inverse kernel matrix.
     *
     * Applies eigenfield/eigenlead decomposition, regularisation, whitening,
     * projection, and optional noise normalisation to produce the kernel
     * matrix that maps sensor data to source estimates.
     *
     * @param[in]  label          Cortical label restricting the source space (empty for all sources).
     * @param[in]  method         Inverse method: "MNE", "dSPM", or "sLORETA".
     * @param[in]  pick_normal    If true, keep only the surface-normal component (requires free orientation).
     * @param[out] K              Assembled kernel matrix (nSources x nChannels).
     * @param[out] noise_norm     Diagonal noise-normalisation matrix (empty for MNE).
     * @param[out] vertno         Vertex numbers per hemisphere.
     *
     * @return true on success.
     */
    bool assemble_kernel(const mne::FsLabel& label,
                         const std::string& method,
                         bool pick_normal,
                         Eigen::MatrixXd& K,
                         Eigen::SparseMatrix<double>& noise_norm,
                         std::vector<Eigen::VectorXi>& vertno);

    //=========================================================================================================
    /**
     * @brief Verify that inverse-operator channels are present in the measurement info.
     *
     * @param[in] info   Measurement info whose channel names are checked.
     *
     * @return true if all channels match, false otherwise.
     */
    bool check_ch_names(const mne::FiffInfo& info) const;

    //=========================================================================================================
    /**
     * @brief Cluster the inverse kernel by cortical parcellation.
     *
     * Groups source-space points within each parcellation label using KMeans
     * and returns a reduced kernel together with the cluster operator.
     *
     * @param[in]  annotationSet   Annotation set for left and right hemisphere.
     * @param[in]  clusterSize     Maximum number of sources per cluster.
     * @param[out] D               Cluster operator matrix (nSources x nClusters).
     * @param[in]  method          Distance metric: "cityblock" or "sqeuclidean".
     *
     * @return Clustered kernel matrix.
     */
    Eigen::MatrixXd cluster_kernel(const mne::FsAnnotationSet& annotationSet,
                                   std::int32_t clusterSize,
                                   Eigen::MatrixXd& D,
                                   const std::string& method = "cityblock") const;

    //=========================================================================================================
    /**
     * @brief Access the most recently assembled kernel (mutable).
     * @return Reference to the kernel matrix.
     */
    inline Eigen::MatrixXd& getKernel();

    //=========================================================================================================
    /**
     * @brief Access the most recently assembled kernel (const).
     * @return Copy of the kernel matrix.
     */
    inline Eigen::MatrixXd getKernel() const;

    //=========================================================================================================
    /**
     * @brief Check whether the inverse operator uses fixed source orientations.
     * @return true if orientation is fixed-normal.
     */
    inline bool isFixedOrient() const;

    //=========================================================================================================
    /**
     * @brief Assemble an inverse operator from a forward solution and noise covariance.
     *
     * Performs depth weighting, source-covariance construction, whitening,
     * and SVD decomposition of the weighted lead-field matrix.
     *
     * @param[in] info               Measurement info; bad channels in info.bads are excluded.
     * @param[in] forward            Forward operator (modified internally for fixed orientation if needed).
     * @param[in] noiseCov           Noise covariance matrix.
     * @param[in] loose              Tangential-component weight in [0, 1].
     * @param[in] depth              Depth-weighting exponent in [0, 1]; 0 disables.
     * @param[in] fixed              Use fixed surface-normal orientations.
     * @param[in] limit_depth_chs    Restrict depth weighting to gradiometers (or fallback).
     *
     * @return Assembled inverse operator.
     */
    static MNEInverseOperator make_inverse_operator(const mne::FiffInfo& info,
                                                    MNEForwardSolution forward,
                                                    const mne::FiffCov& noiseCov,
                                                    float loose = 0.2f,
                                                    float depth = 0.8f,
                                                    bool fixed = false,
                                                    bool limit_depth_chs = true);

    //=========================================================================================================
    /**
     * @brief Prepare the inverse operator for source estimation.
     *
     * Scales covariances by the number of averages, builds the regularised
     * pseudo-inverse, the whitener, the SSP projector, and (optionally)
     * the noise-normalisation factors for dSPM or sLORETA.
     *
     * @param[in] nave      Number of averages (scales the noise covariance).
     * @param[in] lambda2   Regularisation parameter (SNR^{-2}).
     * @param[in] dSPM      Compute dSPM noise-normalisation factors.
     * @param[in] sLORETA   Compute sLORETA noise-normalisation factors.
     *
     * @return A prepared copy of the inverse operator ready for apply_inverse.
     */
    MNEInverseOperator prepare_inverse_operator(std::int32_t nave,
                                                float lambda2,
                                                bool dSPM,
                                                bool sLORETA = false) const;

    //=========================================================================================================
    /**
     * Reads an inverse operator file, as mne.minimum_norm.read_inverse_operator does.
     *
     * @param[in] path   The -inv.fif file.
     *
     * @return The inverse operator, or why the file cannot be read: Io if it cannot be opened, NotFound if
     *         it holds no inverse solution, Format if a required part is missing or malformed.
     */
    [[nodiscard]] static mne::Expected<MNEInverseOperator> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the inverse operator, as mne.minimum_norm.write_inverse_operator does.
     *
     * @param[in] path   The -inv.fif file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the inverse solution block into an open FIFF stream.
     *
     * @param[in] stream  Open FIFF stream.
     */
    void write_to_stream(mne::FiffStream& stream) const;

    //=========================================================================================================
    /**
     * @brief Stream-output operator for diagnostic printing.
     *
     * @param[in] out   Output stream.
     * @param[in] inv   Inverse operator to print.
     *
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& out, const MNEInverseOperator& inv);

public:
    mne::FiffInfoBase info;                /**< Lightweight measurement info (channel names, types, etc.). */
    mne::fiff_int_t methods = -1;          /**< Modality flag: FIFFV_MNE_MEG, _EEG, or _MEG_EEG. */
    mne::fiff_int_t source_ori = -1;       /**< Source orientation constraint (FIFFV_MNE_FREE_ORI / FIXED_ORI). */
    mne::fiff_int_t nsource = -1;          /**< Number of source-space points. */
    mne::fiff_int_t nchan = -1;            /**< Number of channels (= number of singular values). */
    mne::fiff_int_t coord_frame = -1;      /**< Coordinate frame of the inverse (MRI or head). */
    mne::fiff_int_t units = -1;            /**< Unit of the source estimates, FIFF_UNIT_AM, FIFF_UNIT_AM_M2 or FIFF_UNIT_AM_M3. -1 when the file did not state one. */
    Eigen::MatrixXf source_nn;             /**< Source-normal vectors (nsource x 3, or nsource*3 x 3 for free). */
    Eigen::VectorXd sing;                  /**< Singular values of the whitened lead field. */
    bool eigen_leads_weighted = false;     /**< True if eigenleads already include R^{0.5} weighting. */
    mne::FiffNamedMatrix eigen_leads;      /**< Right singular vectors (eigenleads), sources x components. */
    mne::FiffNamedMatrix eigen_fields;     /**< Left singular vectors (eigenfields), channels x components. */
    mne::FiffCov noise_cov;                /**< Noise covariance matrix. */
    mne::FiffCov source_cov;               /**< Source covariance matrix (depth + orientation weighting). */
    mne::FiffCov orient_prior;             /**< Orientation prior (loose-constraint weighting). */
    mne::FiffCov depth_prior;              /**< Depth prior (depth-weighting coefficients). */
    mne::FiffCov fmri_prior;               /**< fMRI prior (if available). */
    MNESourceSpaces src;                   /**< Source space (surfaces and/or volume). */
    mne::FiffCoordTrans mri_head_t;        /**< MRI-to-head coordinate transformation. */
    mne::fiff_int_t nave = -1;             /**< Number of averages (default 1). */
    std::vector<mne::FiffProj> projs;      /**< SSP projectors read from the file. */
    Eigen::MatrixXd proj;                  /**< SSP projector matrix applied to data. */
    Eigen::MatrixXd whitener;              /**< Whitening matrix (derived from noise covariance). */
    Eigen::VectorXd reginv;                /**< Diagonal regularised inverse of the singular values. */
    Eigen::SparseMatrix<double> noisenorm; /**< Diagonal noise-normalisation matrix (dSPM / sLORETA). */

private:
    Eigen::MatrixXd m_K; /**< Most recently assembled kernel matrix. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Eigen::MatrixXd& MNEInverseOperator::getKernel()
{
    return m_K;
}

//=============================================================================================================

inline Eigen::MatrixXd MNEInverseOperator::getKernel() const
{
    return m_K;
}

//=============================================================================================================

inline bool MNEInverseOperator::isFixedOrient() const
{
    return this->source_ori == FIFFV_MNE_FIXED_ORI;
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const MNEInverseOperator& inv)
{
    out << "#### MNE Inverse Operator ####\n"
        << "  methods:     " << inv.methods << '\n'
        << "  source_ori:  " << inv.source_ori << '\n'
        << "  nsource:     " << inv.nsource << '\n'
        << "  nchan:       " << inv.nchan << '\n'
        << "  coord_frame: " << inv.coord_frame << '\n'
        << "  eigen_leads: " << inv.eigen_leads << '\n'
        << "  eigen_fields:" << inv.eigen_fields << '\n';
    return out;
}
} // NAMESPACE

#endif // MNE_INVERSE_OPERATOR_H
