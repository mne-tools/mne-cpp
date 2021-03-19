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
#include "mne_sourcespace.h"
#include "mne_forwardsolution.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <utils/mnemath.h>

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
    class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=========================================================================================================
/**
 * Gain matrix output data for one region, used for clustering
 */
struct RegionMTOut
{
    Eigen::VectorXi    roiIdx;     /**< Region cluster indices. */
    Eigen::MatrixXd    ctrs;       /**< Cluster centers. */
    Eigen::VectorXd    sumd;       /**< Sums of the distances to the centroid. */
    Eigen::MatrixXd    D;          /**< Distances to the centroid. */

    qint32      iLabelIdxOut;   /**< Label ID. */
};

//=========================================================================================================
/**
 * Gain matrix input data for one region, used for clustering
 */
struct RegionMT
{
    Eigen::MatrixXd    matRoiMT;           /**< Reshaped region gain matrix sources x sensors(x,y,z)*/
    Eigen::MatrixXd    matRoiMTOrig;       /**< Region gain matrix sensors x sources(x,y,z)*/

    qint32      nClusters;      /**< Number of clusters within this region. */
    Eigen::VectorXi    idcs;           /**< Get source space indeces. */
    qint32      iLabelIdxIn;    /**< Label ID. */

    QString     sDistMeasure;   /**< "cityblock" or "sqeuclidean". */

    RegionMTOut cluster() const
    {
        QString t_sDistMeasure;
         if(sDistMeasure.isEmpty())
             t_sDistMeasure = QString("cityblock");
         else
             t_sDistMeasure = sDistMeasure;

        // Kmeans Reduction
        RegionMTOut p_RegionMTOut;

        UTILSLIB::KMeans t_kMeans(t_sDistMeasure, QString("sample"), 5);

        t_kMeans.calculate(this->matRoiMT, this->nClusters, p_RegionMTOut.roiIdx, p_RegionMTOut.ctrs, p_RegionMTOut.sumd, p_RegionMTOut.D);

        p_RegionMTOut.iLabelIdxOut = this->iLabelIdxIn;

        return p_RegionMTOut;
    }
};

//=============================================================================================================
/**
 * Inverse operator
 *
 * @brief Inverse operator
 */
class MNESHARED_EXPORT MNEInverseOperator
{
public:
    typedef QSharedPointer<MNEInverseOperator> SPtr;            /**< Shared pointer type for MNEInverseOperator. */
    typedef QSharedPointer<const MNEInverseOperator> ConstSPtr; /**< Const shared pointer type for MNEInverseOperator. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNEInverseOperator();

    //=========================================================================================================
    /**
     * Constructs an inverse operator, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the evoked data set.
     */
    MNEInverseOperator(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * Constructs an inverse operator by making one.
     *
     * @param[in] info               The measurement info to specify the channels to include. Bad channels in info['bads'] are not used.
     * @param[in] forward            Forward operator.
     * @param[in] p_noise_cov        The noise covariance matrix.
     * @param[in] loose              float in [0, 1]. Value that weights the source variances of the dipole components defining the tangent space of the cortical surfaces.
     * @param[in] depth              float in [0, 1]. Depth weighting coefficients. If None, no depth weighting is performed.
     * @param[in] fixed              Use fixed source orientations normal to the cortical mantle. If True, the loose parameter is ignored.
     * @param[in] limit_depth_chs    If True, use only grad channels in depth weighting (equivalent to MNE C code). If grad chanels aren't present, only mag channels will be used (if no mag, then eeg). If False, use all channels.
     */
    MNEInverseOperator(const FIFFLIB::FiffInfo &info,
                       const MNEForwardSolution& forward,
                       const FIFFLIB::FiffCov& p_noise_cov,
                       float loose = 0.2f,
                       float depth = 0.8f,
                       bool fixed = false,
                       bool limit_depth_chs = true);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MNEInverseOperator   MNE forward solution.
     */
    MNEInverseOperator(const MNEInverseOperator &p_MNEInverseOperator);

    //=========================================================================================================
    /**
     * Destroys the MNEInverseOperator.
     */
    ~MNEInverseOperator();

    //=========================================================================================================
    /**
     * Simple matrix multiplication followed by combination of the
     * current components
     *
     * This does all the data transformations to compute the weights for the
     * eigenleads
     *
     * @param[in] label          labels.
     * @param[in] method         The applied normals. ("MNE" | "dSPM" | "sLORETA").
     * @param[in] pick_normal    Pick normals.
     * @param[out] K             Kernel.
     * @param[out] noise_norm    Noise normals.
     * @param[out] vertno        Vertices of the hemispheres.
     *
     * @return the assembled kernel.
     */
    bool assemble_kernel(const FSLIB::Label &label,
                         QString method,
                         bool pick_normal,
                         Eigen::MatrixXd &K,
                         Eigen::SparseMatrix<double> &noise_norm,
                         QList<Eigen::VectorXi> &vertno);

    //=========================================================================================================
    /**
     * Check that channels in inverse operator are measurements.
     *
     * @param[in] info   The measurement info.
     *
     * @return true when successful, false otherwise.
     */
    bool check_ch_names(const FIFFLIB::FiffInfo &info) const;

    //=========================================================================================================
    /**
     * Clusters the current kernel
     *
     * @param[in]   p_AnnotationSet     Annotation set containing the annotation of left & right hemisphere.
     * @param[in]   p_iClusterSize      Maximal cluster size per roi.
     * @param[out]   p_D                 The cluster operator.
     * @param[in]   p_sMethod           "cityblock" or "sqeuclidean".
     *
     * @return the clustered kernel.
     */
    Eigen::MatrixXd cluster_kernel(const FSLIB::AnnotationSet &p_AnnotationSet,
                                   qint32 p_iClusterSize,
                                   Eigen::MatrixXd& p_D,
                                   QString p_sMethod = "cityblock") const;

    //=========================================================================================================
    /**
     * Returns the current kernel
     *
     * @return the current kernel.
     */
    inline Eigen::MatrixXd& getKernel();

    //=========================================================================================================
    /**
     * Returns the current kernel
     *
     * @return the current kernel.
     */
    inline Eigen::MatrixXd getKernel() const;

    //=========================================================================================================
    /**
     * Has inverse operator fixed orientation?
     *
     * @return true if inverse operator has fixed orientation, false otherwise.
     */
    inline bool isFixedOrient() const;

    //=========================================================================================================
    /**
     * Assembles the inverse operator.
     *
     * @param[in] info               The measurement info to specify the channels to include. Bad channels in info['bads'] are not used.
     * @param[in] forward            Forward operator.
     * @param[in] p_noise_cov        The noise covariance matrix.
     * @param[in] loose              float in [0, 1]. Value that weights the source variances of the dipole components defining the tangent space of the cortical surfaces.
     * @param[in] depth              float in [0, 1]. Depth weighting coefficients. If None, no depth weighting is performed.
     * @param[in] fixed              Use fixed source orientations normal to the cortical mantle. If True, the loose parameter is ignored.
     * @param[in] limit_depth_chs    If True, use only grad channels in depth weighting (equivalent to MNE C code). If grad chanels aren't present, only mag channels will be used (if no mag, then eeg). If False, use all channels.
     *
     * @return the assembled inverse operator.
     */
    static MNEInverseOperator make_inverse_operator(const FIFFLIB::FiffInfo &info,
                                                    MNEForwardSolution forward,
                                                    const FIFFLIB::FiffCov& p_noise_cov,
                                                    float loose = 0.2f,
                                                    float depth = 0.8f,
                                                    bool fixed = false,
                                                    bool limit_depth_chs = true);

    //=========================================================================================================
    /**
     * mne_prepare_inverse_operator
     *
     * ### MNE toolbox root function ###
     *
     * Prepare for actually computing the inverse
     *
     * @param[in] nave      Number of averages (scales the noise covariance).
     * @param[in] lambda2   The regularization factor.
     * @param[in] dSPM      Compute the noise-normalization factors for dSPM?.
     * @param[in] sLORETA   Compute the noise-normalization factors for sLORETA?.
     *
     * @return the prepared inverse operator.
     */
    MNEInverseOperator prepare_inverse_operator(qint32 nave,
                                                float lambda2,
                                                bool dSPM,
                                                bool sLORETA = false) const;

    //=========================================================================================================
    /**
     * mne_read_inverse_operator
     *
     * ### MNE toolbox root function ###
     *
     * Reads the inverse operator decomposition from a fif file
     *
     * @param[in] p_IODevice   A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in, out] inv          The read inverse operator.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_inverse_operator(QIODevice &p_IODevice, MNEInverseOperator& inv);

    //=========================================================================================================
    /**
     * write_inverse_operator
     *
     * Writes an inverse operator to a fif file
     *
     * @param[in] p_IODevice   IO device to write the inverse operator to.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Writes the inverse operator to a FIFF stream
     *
     * @param[in] p_pStream  The stream to write to.
     */
    void writeToStream(FIFFLIB::FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out                    The stream to which the fiff covariance should be assigned to.
     * @param[in] p_MNEInverseOperator   MNEInverseOperator which should be assigned to the stream.
     *
     * @return the stream with the attached fiff covariance matrix.
     */
    friend std::ostream& operator<<(std::ostream& out, const MNELIB::MNEInverseOperator &p_MNEInverseOperator);

public:
    FIFFLIB::FiffInfoBase info;                     /**< light weighted measurement info. */
    FIFFLIB::fiff_int_t methods;                    /**< MEG, EEG or both. */
    FIFFLIB::fiff_int_t source_ori;                 /**< Source orientation: f. */
    FIFFLIB::fiff_int_t nsource;                    /**< Number of source points. */
    FIFFLIB::fiff_int_t nchan;                      /**< Number of channels. */
    FIFFLIB::fiff_int_t coord_frame;                /**< Coordinate system definition. */
    Eigen::MatrixXf  source_nn;                     /**< Source normals. */
    Eigen::VectorXd  sing;                          /**< Singular values. */
    bool    eigen_leads_weighted;                   /**< If eigen lead are weighted. */
    FIFFLIB::FiffNamedMatrix::SDPtr eigen_leads;    /**< Eigen leads. */
    FIFFLIB::FiffNamedMatrix::SDPtr eigen_fields;   /**< Eigen fields. */
    FIFFLIB::FiffCov::SDPtr noise_cov;              /**< Noise covariance matrix. */
    FIFFLIB::FiffCov::SDPtr source_cov;             /**< Source covariance matrix. */
    FIFFLIB::FiffCov::SDPtr orient_prior;           /**< Orientation priors. */
    FIFFLIB::FiffCov::SDPtr depth_prior;            /**< Depth priors. */
    FIFFLIB::FiffCov::SDPtr fmri_prior;             /**< fMRI priors. */
    MNESourceSpace src;                             /**< Source Space. */
    FIFFLIB::FiffCoordTrans mri_head_t;             /**< MRI head coordinate transformation. */
    FIFFLIB::fiff_int_t nave;                       /**< Number of averages used to regularize the solution. Set to 1 on single Epoch by default.*/
    QList<FIFFLIB::FiffProj> projs;                 /**< SSP operator. */
    Eigen::MatrixXd proj;                           /**< The projector to apply to the data. */
    Eigen::MatrixXd whitener;                       /**< Whitens the data. */
    Eigen::VectorXd reginv;                         /**< The diagonal matrix implementing. regularization and the inverse. */
    Eigen::SparseMatrix<double> noisenorm;          /**< These are the noise-normalization factors. */

private:
    Eigen::MatrixXd m_K;                            /**< Everytime a new kernel is assamebled a copy is stored here. */
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

inline std::ostream& operator<<(std::ostream& out, const MNELIB::MNEInverseOperator &p_MNEInverseOperator)
{
    out << "#### MNE Inverse Operator ####\n";

    out << "\n methods: " << p_MNEInverseOperator.methods << std::endl;
    out << "\n source_ori: " << p_MNEInverseOperator.source_ori << std::endl;
    out << "\n nsource: " << p_MNEInverseOperator.nsource << std::endl;
    out << "\n nchan: " << p_MNEInverseOperator.nchan << std::endl;
    out << "\n coord_frame:\n\t" << p_MNEInverseOperator.coord_frame << std::endl;

    out << "\n eigen_leads: " << p_MNEInverseOperator.eigen_leads << std::endl;
    out << "\n eigen_fields:\n\t" << p_MNEInverseOperator.eigen_fields << std::endl;

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
