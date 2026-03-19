//=============================================================================================================
/**
 * @file     mne_inverse_operator.h
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     MNEInverseOperator class declaration.
 *
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
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <math/mnemath.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB
{
    class FsLabel;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=========================================================================================================
/**
 * @brief Output of a multi-threaded KMeans clustering step for a single cortical region.
 */
struct RegionMTOut
{
    Eigen::VectorXi roiIdx;         /**< Per-source cluster assignment indices. */
    Eigen::MatrixXd ctrs;           /**< Cluster centroids (nClusters x features). */
    Eigen::VectorXd sumd;           /**< Sum of distances to the assigned centroid. */
    Eigen::MatrixXd D;              /**< Distance matrix (nSources x nClusters). */
    qint32          iLabelIdxOut;   /**< Label index carried through from the input. */
};

//=========================================================================================================
/**
 * @brief Input parameters for multi-threaded KMeans clustering on a single cortical region.
 */
struct RegionMT
{
    Eigen::MatrixXd matRoiMT;       /**< Reshaped gain matrix (sources x sensors*3). */
    Eigen::MatrixXd matRoiMTOrig;   /**< Original gain matrix (sensors x sources*3). */
    qint32          nClusters;      /**< Number of clusters for this region. */
    Eigen::VectorXi idcs;           /**< Source-space indices for this region. */
    qint32          iLabelIdxIn;    /**< Label index passed through to the output. */
    QString         sDistMeasure;   /**< Distance metric: "cityblock" or "sqeuclidean". */

    /**
     * @brief Run KMeans clustering on this region.
     * @return Clustering result including centroid positions and assignments.
     */
    RegionMTOut cluster() const
    {
        const QString distMeasure = sDistMeasure.isEmpty()
                                    ? QStringLiteral("cityblock")
                                    : sDistMeasure;

        RegionMTOut out;
        UTILSLIB::KMeans kMeans(distMeasure, QStringLiteral("sample"), 5);
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
 */
class MNESHARED_EXPORT MNEInverseOperator
{
public:
    typedef QSharedPointer<MNEInverseOperator> SPtr;            /**< Shared pointer type for MNEInverseOperator. */
    typedef QSharedPointer<const MNEInverseOperator> ConstSPtr; /**< Const shared pointer type for MNEInverseOperator. */

    //=========================================================================================================
    /**
     * @brief Constructs an empty inverse operator with invalid sentinel values.
     */
    MNEInverseOperator();

    //=========================================================================================================
    /**
     * @brief Constructs an inverse operator by reading from a FIFF IO device.
     *
     * @param[in] p_IODevice     IO device (e.g. QFile) containing the inverse operator.
     */
    explicit MNEInverseOperator(QIODevice& p_IODevice);

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
    MNEInverseOperator(const FIFFLIB::FiffInfo &info,
                       const MNEForwardSolution& forward,
                       const FIFFLIB::FiffCov& noiseCov,
                       float loose = 0.2f,
                       float depth = 0.8f,
                       bool fixed = false,
                       bool limit_depth_chs = true);

    //=========================================================================================================
    /**
     * @brief Copy constructor.
     *
     * @param[in] other   Inverse operator to copy.
     */
    MNEInverseOperator(const MNEInverseOperator &other);

    //=========================================================================================================
    /**
     * @brief Destructor.
     */
    ~MNEInverseOperator();

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
    bool assemble_kernel(const FSLIB::FsLabel &label,
                         const QString &method,
                         bool pick_normal,
                         Eigen::MatrixXd &K,
                         Eigen::SparseMatrix<double> &noise_norm,
                         QList<Eigen::VectorXi> &vertno);

    //=========================================================================================================
    /**
     * @brief Verify that inverse-operator channels are present in the measurement info.
     *
     * @param[in] info   Measurement info whose channel names are checked.
     *
     * @return true if all channels match, false otherwise.
     */
    bool check_ch_names(const FIFFLIB::FiffInfo &info) const;

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
    Eigen::MatrixXd cluster_kernel(const FSLIB::FsAnnotationSet &annotationSet,
                                   qint32 clusterSize,
                                   Eigen::MatrixXd &D,
                                   const QString &method = QStringLiteral("cityblock")) const;

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
    static MNEInverseOperator make_inverse_operator(const FIFFLIB::FiffInfo &info,
                                                    MNEForwardSolution forward,
                                                    const FIFFLIB::FiffCov& noiseCov,
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
    MNEInverseOperator prepare_inverse_operator(qint32 nave,
                                                float lambda2,
                                                bool dSPM,
                                                bool sLORETA = false) const;

    //=========================================================================================================
    /**
     * @brief Read an inverse operator from a FIFF file.
     *
     * @param[in]      p_IODevice   FIFF IO device (e.g. QFile or QTcpSocket).
     * @param[in,out]  inv          Receives the loaded inverse operator.
     *
     * @return true on success, false on failure.
     */
    static bool read_inverse_operator(QIODevice &p_IODevice, MNEInverseOperator& inv);

    //=========================================================================================================
    /**
     * @brief Write the inverse operator to a FIFF file.
     *
     * @param[in] p_IODevice   IO device to write to.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * @brief Write the inverse operator into an already-open FIFF stream.
     *
     * @param[in] p_pStream  Open FIFF stream.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * @brief Stream-output operator for diagnostic printing.
     *
     * @param[in] out   Output stream.
     * @param[in] inv   Inverse operator to print.
     *
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& out, const MNEInverseOperator &inv);

public:
    FIFFLIB::FiffInfoBase info;                     /**< Lightweight measurement info (channel names, types, etc.). */
    FIFFLIB::fiff_int_t methods;                    /**< Modality flag: FIFFV_MNE_MEG, _EEG, or _MEG_EEG. */
    FIFFLIB::fiff_int_t source_ori;                 /**< Source orientation constraint (FIFFV_MNE_FREE_ORI / FIXED_ORI). */
    FIFFLIB::fiff_int_t nsource;                    /**< Number of source-space points. */
    FIFFLIB::fiff_int_t nchan;                      /**< Number of channels (= number of singular values). */
    FIFFLIB::fiff_int_t coord_frame;                /**< Coordinate frame of the inverse (MRI or head). */
    Eigen::MatrixXf  source_nn;                     /**< Source-normal vectors (nsource x 3, or nsource*3 x 3 for free). */
    Eigen::VectorXd  sing;                          /**< Singular values of the whitened lead field. */
    bool    eigen_leads_weighted;                   /**< True if eigenleads already include R^{0.5} weighting. */
    FIFFLIB::FiffNamedMatrix::SDPtr eigen_leads;    /**< Right singular vectors (eigenleads), sources x components. */
    FIFFLIB::FiffNamedMatrix::SDPtr eigen_fields;   /**< Left singular vectors (eigenfields), channels x components. */
    FIFFLIB::FiffCov::SDPtr noise_cov;              /**< Noise covariance matrix. */
    FIFFLIB::FiffCov::SDPtr source_cov;             /**< Source covariance matrix (depth + orientation weighting). */
    FIFFLIB::FiffCov::SDPtr orient_prior;           /**< Orientation prior (loose-constraint weighting). */
    FIFFLIB::FiffCov::SDPtr depth_prior;            /**< Depth prior (depth-weighting coefficients). */
    FIFFLIB::FiffCov::SDPtr fmri_prior;             /**< fMRI prior (if available). */
    MNESourceSpaces src;                    /**< Source space (surfaces and/or volume). */
    FIFFLIB::FiffCoordTrans mri_head_t;             /**< MRI-to-head coordinate transformation. */
    FIFFLIB::fiff_int_t nave;                       /**< Number of averages (default 1). */
    QList<FIFFLIB::FiffProj> projs;                 /**< SSP projectors read from the file. */
    Eigen::MatrixXd proj;                           /**< SSP projector matrix applied to data. */
    Eigen::MatrixXd whitener;                       /**< Whitening matrix (derived from noise covariance). */
    Eigen::VectorXd reginv;                         /**< Diagonal regularised inverse of the singular values. */
    Eigen::SparseMatrix<double> noisenorm;          /**< Diagonal noise-normalisation matrix (dSPM / sLORETA). */

private:
    Eigen::MatrixXd m_K;                            /**< Most recently assembled kernel matrix. */
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

inline std::ostream& operator<<(std::ostream& out, const MNEInverseOperator &inv)
{
    out << "#### MNE Inverse Operator ####\n"
        << "  methods:     " << inv.methods << '\n'
        << "  source_ori:  " << inv.source_ori << '\n'
        << "  nsource:     " << inv.nsource << '\n'
        << "  nchan:       " << inv.nchan << '\n'
        << "  coord_frame: " << inv.coord_frame << '\n'
        << "  eigen_leads: " << *inv.eigen_leads << '\n'
        << "  eigen_fields:" << *inv.eigen_fields << '\n';
    return out;
}
} // NAMESPACE

#ifndef metatype_mneinverseoperatorsptr
#define metatype_mneinverseoperatorsptr
Q_DECLARE_METATYPE(QSharedPointer<MNELIB::MNEInverseOperator>); /**< Provides QT META type declaration of the QSharedPointer<MNELIB::MNEInverseOperator> type. For signal/slot usage.*/
#endif

#ifndef metatype_mneinverseoperators
#define metatype_mneinverseoperators
Q_DECLARE_METATYPE(MNELIB::MNEInverseOperator); /**< Provides QT META type declaration of the MNELIB::MNEInverseOperator type. For signal/slot usage.*/
#endif

#endif // MNE_INVERSE_OPERATOR_H
