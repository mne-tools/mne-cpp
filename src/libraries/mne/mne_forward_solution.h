//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_forward_solution.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Forward solution (gain matrix mapping source dipoles to sensor measurements).
 *
 * @ref mne::MNEForwardSolution is the in-memory representation of an
 * @c -fwd.fif file: the leadfield matrix, the source spaces it was
 * computed on, the sensor coordinate transforms, the projection vectors
 * and the source-orientation flags. It is the central input to inverse
 * modelling (@ref mne::MNEInverseOperator), to cluster-based dimensionality
 * reduction (@c cluster_forward_solution) and to simulation routines.
 * FIFF tags involved: @c FIFFB_MNE_FORWARD_SOLUTION,
 * @c FIFF_MNE_FORWARD_SOLUTION_GRAD, @c FIFF_MNE_SOURCE_ORIENTATION,
 * @c FIFF_MNE_COORD_FRAME.
 */

#ifndef MNE_FORWARD_SOLUTION_H
#define MNE_FORWARD_SOLUTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <cstdint>
#include <string>
#include <vector>

#include <mne/mne_source_spaces.h>

#include <math/kmeans.h>

#include <fs/fs_annotationset.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_types.h>
#include <fiff/fiff_info_base.h>
#include <fiff/fiff_cov.h>

#include <base/error.h>

#include <math.h>
#include <filesystem>
#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=========================================================================================================
/**
 * Gain matrix output data for one region, used for clustering
 *
 * @brief Output of a cluster-based forward solution computation for a single cortical region
 */
struct RegionDataOut
{
    Eigen::VectorXi roiIdx; /**< Region cluster indices. */
    Eigen::MatrixXd ctrs;   /**< Cluster centers. */
    Eigen::VectorXd sumd;   /**< Sums of the distances to the centroid. */
    Eigen::MatrixXd D;      /**< Distances to the centroid. */

    std::int32_t iLabelIdxOut; /**< FsLabel ID. */
};

//=========================================================================================================
/**
 * Gain matrix input data for one region, used for clustering
 *
 * @brief Input parameters for cluster-based forward solution computation on a single cortical region
 */
struct RegionData
{
    Eigen::MatrixXd matRoiG;         /**< Reshaped region gain matrix sources x sensors(x,y,z)*/
    Eigen::MatrixXd matRoiGWhitened; /**< Reshaped whitened region gain matrix sources x sensors(x,y,z)*/
    bool bUseWhitened;               /**< Wheather indeces of whitened gain matrix should be used to calculate centroids. */

    Eigen::MatrixXd matRoiGOrig; /**< Region gain matrix sensors x sources(x,y,z)*/

    std::int32_t nClusters; /**< Number of clusters within this region. */

    Eigen::VectorXi idcs;     /**< Get source space indeces. */
    std::int32_t iLabelIdxIn; /**< FsLabel ID. */
    std::string sDistMeasure; /**< "cityblock" or "sqeuclidean". */

    RegionDataOut cluster() const
    {
        const std::string t_sDistMeasure = sDistMeasure.empty() ? std::string("cityblock") : sDistMeasure;

        // Kmeans Reduction
        RegionDataOut p_RegionDataOut;

        mne::KMeans t_kMeans(t_sDistMeasure, "sample", 5);

        if (bUseWhitened) {
            t_kMeans.calculate(this->matRoiGWhitened, this->nClusters, p_RegionDataOut.roiIdx, p_RegionDataOut.ctrs, p_RegionDataOut.sumd, p_RegionDataOut.D);

            // Centroids of the original gain: all channels, not only those the whitener kept
            Eigen::MatrixXd newCtrs = Eigen::MatrixXd::Zero(p_RegionDataOut.ctrs.rows(), this->matRoiG.cols());
            for (std::int32_t c = 0; c < p_RegionDataOut.ctrs.rows(); ++c) {
                std::int32_t num = 0;

                for (std::int32_t idx = 0; idx < p_RegionDataOut.roiIdx.size(); ++idx) {
                    if (c == p_RegionDataOut.roiIdx[idx]) {
                        newCtrs.row(c) += this->matRoiG.row(idx); //just take whitened to get indeces calculate centroids using the original matrix
                        ++num;
                    }
                }

                if (num > 0)
                    newCtrs.row(c) /= num;
            }
            p_RegionDataOut.ctrs = newCtrs; //Replace whitened with original
        } else
            t_kMeans.calculate(this->matRoiG, this->nClusters, p_RegionDataOut.roiIdx, p_RegionDataOut.ctrs, p_RegionDataOut.sumd, p_RegionDataOut.D);

        p_RegionDataOut.iLabelIdxOut = this->iLabelIdxIn;

        return p_RegionDataOut;
    }
};

const static mne::FiffCov defaultCov;
const static mne::FiffInfo defaultInfo;
static Eigen::MatrixXd defaultD;

//=============================================================================================================
/**
 * Forward solution: leadfield matrix mapping source-space dipoles to sensor
 * measurements, together with the source spaces, coordinate transforms and
 * SSP/CTF state it was computed under.
 *
 * @brief In-memory representation of an @c -fwd.fif forward solution.
 *
 * @snippet ex_inv_api/main.cpp inv_minimum_norm_usage
 */
class MNESHARED_EXPORT MNEForwardSolution
{
public:
    /** How read() prepares the solution it reads. */
    struct ReadOptions
    {
        bool force_fixed = false;  /**< Convert a free-orientation solution to fixed orientations. */
        bool surf_ori = false;     /**< Use surface-based source coordinates. */
        StringList include{};      /**< Keep only these channels; empty keeps all. */
        StringList exclude{};      /**< Drop these channels. */
        bool exclude_bads = false; /**< Drop the channels marked bad in the file; mne.read_forward_solution keeps them. */
    };

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEForwardSolution() = default;

    //=========================================================================================================
    /**
     * Reads a forward solution file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The -fwd.fif file.
     */
    explicit MNEForwardSolution(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads a forward solution file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path      The -fwd.fif file.
     * @param[in] options   How to prepare the solution.
     */
    MNEForwardSolution(const std::filesystem::path& path, const ReadOptions& options);

    //=========================================================================================================
    /**
     * Initializes the MNE forward solution.
     */
    void clear();

    //=========================================================================================================
    /**
     * Cluster the forward solution and stores the result to p_fwdOut.
     * The clustering is done by using the provided annotations
     *
     * @param[in]   p_AnnotationSet     FsAnnotation set containing the annotation of left & right hemisphere.
     * @param[in]   p_iClusterSize      Maximal cluster size per roi.
     * @param[out]   p_D                 The cluster operator.
     * @param[in] p_pNoise_cov Noise covariance used to whiten the gain matrix before clustering (optional).
     * @param[in] p_pInfo Measurement info matching the noise covariance (optional).
     * @param[in]   p_sMethod           "cityblock" or "sqeuclidean".
     *
     * @return clustered MNE forward solution.
     */
    MNEForwardSolution cluster_forward_solution(const mne::FsAnnotationSet& p_AnnotationSet,
                                                std::int32_t p_iClusterSize,
                                                Eigen::MatrixXd& p_D = defaultD,
                                                const mne::FiffCov& p_pNoise_cov = defaultCov,
                                                const mne::FiffInfo& p_pInfo = defaultInfo,
                                                const std::string& p_sMethod = "cityblock") const;

    //=========================================================================================================
    /**
     * Compute orientation prior
     *
     * @param[in] loose      The loose orientation parameter.
     *
     * @return Orientation priors.
     */
    mne::FiffCov compute_orient_prior(float loose = 0.2f);

    //=========================================================================================================
    /**
     * Compute weighting for depth prior. ToDo move this to FiffCov
     *
     * @param[in] Gain               gain matrix.
     * @param[in] gain_info          The measurement info to specify the channels to include.
     * @param[in] is_fixed_ori       Fixed orientation?.
     * @param[in] exp                float in [0, 1]. Depth weighting coefficients. If None, no depth weighting is performed. (optional; default = 0.8).
     * @param[in] limit              (optional; default = 10.0).
     * @param[in] patch_areas        (optional) Cortical patch area of each source position (m²); the source
     *                               gain is divided by it, as in mne.forward.compute_depth_prior.
     * @param[in] limit_depth_chs    If True, use only grad channels in depth weighting (equivalent to MNE C code). If grad chanels aren't present, only mag channels will be used (if no mag, then eeg). If False, use all channels. (optional).
     *
     * @return the depth prior.
     */
    static mne::FiffCov compute_depth_prior(const Eigen::MatrixXd& Gain,
                                            const mne::FiffInfo& gain_info,
                                            bool is_fixed_ori,
                                            double exp = 0.8,
                                            double limit = 10.0,
                                            const Eigen::MatrixXd& patch_areas = mne::defaultConstMatrixXd,
                                            bool limit_depth_chs = false);

    //=========================================================================================================
    /**
     * Indicates whether fwd conatins a clustered forward solution.
     *
     * @return true if forward solution is clustered, false otherwise.
     */
    bool isClustered() const;

    //=========================================================================================================
    /**
     * True if FIFF measurement file information is empty.
     *
     * @return true if FIFF measurement file information is empty.
     */
    inline bool empty() const;

    //=========================================================================================================
    /**
     * Has forward operator fixed orientation?
     *
     * @return true if forward operator has fixed orientation, false otherwise.
     */
    inline bool isFixedOrient() const;

    //=========================================================================================================
    /**
     * mne.fiff.pick_channels_forward
     *
     * Pick channels from forward operator
     *
     * @param[in] include    List of channels to include. (if None, include all available).
     * @param[in] exclude    Channels to exclude (if None, do not exclude any).
     *
     * @return Forward solution restricted to selected channel types.
     */
    MNEForwardSolution pick_channels(const StringList& include = {},
                                     const StringList& exclude = {}) const;

    //=========================================================================================================
    /**
     * Reduces a forward solution to selected regions
     *
     * @param[in] p_qListLabels  ROIs.
     *
     * @return the reduced forward solution.
     */
    MNEForwardSolution pick_regions(const std::vector<mne::FsLabel>& p_qListLabels) const;

    //=========================================================================================================
    /**
     * mne.fiff.pick_types_forward
     *
     * Pick by channel type and names from a forward operator
     *
     * @param[in] meg        Include MEG channels.
     * @param[in] eeg        Include EEG channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return Forward solution restricted to selected channel types.
     */
    MNEForwardSolution pick_types(bool meg,
                                  bool eeg,
                                  const StringList& include = {},
                                  const StringList& exclude = {}) const;

    //=========================================================================================================
    /**
     * Prepare forward for assembling the inverse operator
     *
     * @param[in] p_info             The measurement info to specify the channels to include. Bad channels in info['bads'] are not used.
     * @param[in] p_noise_cov        The noise covariance matrix.
     * @param[in] p_pca              Calculate pca or not.
     * @param[out] p_outFwdInfo          Selected channel names.
     * @param[out] gain              Gain matrix.
     * @param[out] p_outNoiseCov     noise covariance matrix.
     * @param[out] p_outWhitener     Whitener.
     * @param[out] p_outNumNonZero   the rank (non zeros).
     */
    void prepare_forward(const mne::FiffInfo& p_info,
                         const mne::FiffCov& p_noise_cov,
                         bool p_pca,
                         mne::FiffInfo& p_outFwdInfo,
                         Eigen::MatrixXd& gain,
                         mne::FiffCov& p_outNoiseCov,
                         Eigen::MatrixXd& p_outWhitener,
                         std::int32_t& p_outNumNonZero) const;

    //=========================================================================================================
    /**
     *
     */
    Eigen::VectorXi tripletSelection(const Eigen::VectorXi& p_vecIdxSelection) const
    {
        Eigen::MatrixXi triSelect = p_vecIdxSelection.transpose().replicate(3, 1).array() * 3; //repmat((p_vecIdxSelection - 1) * 3 + 1, 3, 1);
        triSelect.row(1).array() += 1;
        triSelect.row(2).array() += 2;
        Eigen::VectorXi retTriSelect(triSelect.cols() * 3);
        for (int i = 0; i < triSelect.cols(); ++i)
            retTriSelect.block(i * 3, 0, 3, 1) = triSelect.col(i);
        return retTriSelect;
    } // tripletSelection

    //=========================================================================================================
    /**
     * Reads a forward solution file, as mne.read_forward_solution does.
     *
     * @param[in] path   The -fwd.fif file.
     *
     * @return The forward solution, or why the file cannot be read: Io if it cannot be opened, NotFound if
     *         it holds no forward solution, Format if a required part is missing or inconsistent.
     */
    [[nodiscard]] static mne::Expected<MNEForwardSolution> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads a forward solution file and prepares it as @p options ask, e.g.
     * `MNEForwardSolution::read(path, {.surf_ori = true, .exclude_bads = true})`.
     *
     * @param[in] path      The -fwd.fif file.
     * @param[in] options   How to prepare the solution.
     *
     * @return The forward solution, or why the file cannot be read (see read(path)).
     */
    [[nodiscard]] static mne::Expected<MNEForwardSolution> read(const std::filesystem::path& path, const ReadOptions& options);

    //=========================================================================================================
    /**
     * Write the forward solution to a FIFF file.
     *
     * Formerly write_solution (SVN MNE).
     *
     * Splits the combined MEG/EEG forward matrix back into separate blocks
     * for FIFF storage, writes source spaces, coordinate transforms, and
     * parent file provenance.
     *
     * @param[in] path   The -fwd.fif file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * reduces the forward solution and stores the result to p_fwdOut.
     *
     * @param[in]   p_iNumDipoles   Desired number of dipoles.
     * @param[out]   p_D             The reduction operator.
     *
     * @return reduced MNE forward solution.
     */
    MNEForwardSolution reduce_forward_solution(std::int32_t p_iNumDipoles, Eigen::MatrixXd& p_D) const;

    //=========================================================================================================
    /**
     * Restrict gain matrix entries for optimal depth weighting
     *
     * @param[in, out] G     Gain matrix to be restricted; result is stored in place.
     * @param[in] info       Fiff information.
     */
    static void restrict_gain_matrix(Eigen::MatrixXd& G, const mne::FiffInfo& info);

    //=========================================================================================================
    /**
     * Helper to convert the forward solution to fixed ori from free
     */
    void to_fixed_ori();

    //=========================================================================================================
    /**
     * Rotates a free-orientation solution into local surface coordinates (two tangential components, then the
     * normal), using average patch normals when the source spaces carry them. Equivalent of
     * mne.convert_forward_solution(fwd, surf_ori=True). Does nothing if already surface-oriented or fixed.
     */
    void convert_to_surf_ori();

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out                    The stream to which the MNE forward solution should be assigned to.
     * @param[in] p_MNEForwardSolution   MNE forward solution which should be assigned to the stream.
     *
     * @return the stream with the attached fiff projector.
     */
    friend std::ostream& operator<<(std::ostream& out, const mne::MNEForwardSolution& p_MNEForwardSolution);

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand forward solution.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const MNEForwardSolution& a, const MNEForwardSolution& b);

    //=========================================================================================================
    /**
     * Returns the positions of the specified sources based on their beloning labels
     *
     * @param[in] lPickedLabels                    The stream to which the MNE forward solution should be assigned to.
     * @param[in] tSurfSetInflated                 The surface used to pick the source from, based on their index specified bzy this forward solution.
     *
     * @return the source position in 3D space.
     */
    Eigen::MatrixX3f getSourcePositionsByLabel(const std::vector<mne::FsLabel>& lPickedLabels,
                                               const mne::FsSurfaceSet& tSurfSetInflated);

private:
    //=========================================================================================================
    /**
     * Definition of the read_one function in mne_read_forward_solution.m
     *
     * Reads all interesting stuff for one forward solution
     *
     * @param[in] stream  The opened fif file to read from.
     * @param[in] p_Node     The forward solution node.
     * @param[out] one       The read forward solution.
     *
     * @return True if succeeded, false otherwise.
     */
    static bool read_one(mne::FiffStream& stream,
                         const std::shared_ptr<mne::FiffDirNode>& p_Node,
                         MNEForwardSolution& one);

public:
    mne::FiffInfoBase info;           /**< light weighted measurement info. */
    mne::fiff_int_t source_ori = -1;  /**< Source orientation: fixed or free. */
    bool surf_ori = false;            /**< If surface oriented. */
    mne::fiff_int_t coord_frame = -1; /**< Coil coordinate system definition. */
    mne::fiff_int_t nsource = -1;     /**< Number of source dipoles. */
    mne::fiff_int_t nchan = -1;       /**< Number of channels. */
    mne::FiffNamedMatrix sol;         /**< Forward solution. */
    mne::FiffNamedMatrix sol_grad;    /**< ToDo... */
    mne::FiffCoordTrans mri_head_t;   /**< MRI head coordinate transformation. */
    std::string mri_filename;         /**< MRI file name (parent provenance). */
    mne::FiffId mri_id;               /**< MRI file ID (parent provenance). */
    mne::MNESourceSpaces src;         /**< Geometric description of the source spaces (hemispheres). */
    Eigen::MatrixX3f source_rr;       /**< Source locations. */
    Eigen::MatrixX3f source_nn;       /**< Source normals (number depends on fixed or free orientation). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEForwardSolution::empty() const
{
    return this->nchan <= 0;
}

//=============================================================================================================

inline bool MNEForwardSolution::isFixedOrient() const
{
    return this->source_ori == FIFFV_MNE_FIXED_ORI;
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const mne::MNEForwardSolution& p_MNEForwardSolution)
{
    out << "#### MNE Forward Solution ####\n";

    out << "\n source_ori: " << p_MNEForwardSolution.source_ori << std::endl;
    out << "\n coord_frame: " << p_MNEForwardSolution.coord_frame << std::endl;
    out << "\n nsource: " << p_MNEForwardSolution.nsource << std::endl;
    out << "\n nchan: " << p_MNEForwardSolution.nchan << std::endl;
    out << "\n sol:\n\t" << p_MNEForwardSolution.sol << std::endl;
    out << "\n sol_grad:\n\t" << p_MNEForwardSolution.sol_grad << std::endl;

    return out;
}

//=============================================================================================================

inline bool operator==(const MNEForwardSolution& a, const MNEForwardSolution& b)
{
    return (a.info == b.info &&
            a.source_ori == b.source_ori &&
            a.surf_ori == b.surf_ori &&
            a.coord_frame == b.coord_frame &&
            a.nsource == b.nsource &&
            a.nchan == b.nchan &&
            a.sol == b.sol &&
            a.sol_grad == b.sol_grad &&
            a.mri_head_t == b.mri_head_t &&
            a.src == b.src &&
            a.source_rr.isApprox(b.source_rr, 0.0001f) &&
            a.source_nn.isApprox(b.source_nn, 0.0001f));
}
} // NAMESPACE

#endif // MNE_FORWARD_SOLUTION_H
