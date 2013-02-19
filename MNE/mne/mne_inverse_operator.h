//=============================================================================================================
/**
* @file     mne_inverse_operator.h
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
* @brief     MNEInverseOperator class declaration.
*
*/

#ifndef MNE_INVERSE_OPERATOR_H
#define MNE_INVERSE_OPERATOR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_sourcespace.h"
#include "mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mneMath/mnemath.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;


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
    * Copy constructor.
    *
    * @param[in] p_MNEInverseOperator   MNE forward solution
    */
    MNEInverseOperator(const MNEInverseOperator &p_MNEInverseOperator);

    //=========================================================================================================
    /**
    * Destroys the MNEInverseOperator.
    */
    ~MNEInverseOperator();

    //=========================================================================================================
    /**
    * Assembles the inverse operator.
    *
    * @param[in] info               The measurement info to specify the channels to include. Bad channels in info['bads'] are not used.
    * @param[in,out] forward        Forward operator.
    * @param[in] noise_cov          The noise covariance matrix.
    * @param[in] loose              float in [0, 1]. Value that weights the source variances of the dipole components defining the tangent space of the cortical surfaces.
    * @param[in] depth              float in [0, 1]. Depth weighting coefficients. If None, no depth weighting is performed.
    * @param[in] fixed              Use fixed source orientations normal to the cortical mantle. If True, the loose parameter is ignored.
    * @param[in] limit_depth_chs    If True, use only grad channels in depth weighting (equivalent to MNE C code). If grad chanels aren't present, only mag channels will be used (if no mag, then eeg). If False, use all channels.
    *
    * @return the assembled inverse operator
    */
    static MNEInverseOperator make_inverse_operator(FiffInfo &info, MNEForwardSolution &forward, FiffCov& noise_cov, float loose = 0.2f, float depth = 0.8f, bool fixed = false, bool limit_depth_chs = true);

    //=========================================================================================================
    /**
    * mne_prepare_inverse_operator
    *
    * ### MNE toolbox root function ###
    *
    * Prepare for actually computing the inverse
    *
    * @param[in] nave      Number of averages (scales the noise covariance)
    * @param[in] lambda2   The regularization factor
    * @param[in] dSPM      Compute the noise-normalization factors for dSPM?
    * @param[in] sLORETA   Compute the noise-normalization factors for sLORETA?
    *
    * @return the prepared inverse operator
    */
    MNEInverseOperator prepare_inverse_operator(qint32 nave ,float lambda2, bool dSPM, bool sLORETA = false);

    //=========================================================================================================
    /**
    * mne_read_inverse_operator
    *
    * ### MNE toolbox root function ###
    *
    * Reads the inverse operator decomposition from a fif file
    *
    * @param [in] p_pIODevice   A fiff IO device like a fiff QFile or QTCPSocket
    * @param [out] inv          The read inverse operator
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_inverse_operator(QIODevice& p_pIODevice, MNEInverseOperator& inv);

public:
    fiff_int_t methods;                     /**< MEG, EEG or both */
    fiff_int_t source_ori;                  /**< Source orientation: f */
    fiff_int_t nsource;                     /**< Number of source points. */
    fiff_int_t nchan;                       /**< Number of channels. */
    fiff_int_t coord_frame;                 /**< Coordinate system definition */
    MatrixXd  source_nn;                    /**< Source normals. */
    VectorXd  sing;                         /**< Singular values */
    bool    eigen_leads_weighted;           /**< If eigen lead are weighted. */
    FiffNamedMatrix::SDPtr eigen_leads;     /**< Eigen leads */
    FiffNamedMatrix::SDPtr eigen_fields;    /**< Eigen fields */
    FiffCov::SDPtr noise_cov;               /**< Noise covariance matrix. */
    FiffCov::SDPtr source_cov;              /**< Source covariance matrix. */
    FiffCov::SDPtr orient_prior;            /**< Orientation priors */
    FiffCov::SDPtr depth_prior;             /**< Depth priors */
    FiffCov::SDPtr fmri_prior;              /**< fMRI priors */
    MNESourceSpace src;                     /**< Source Space */
    FiffCoordTrans mri_head_t;              /**< MRI head coordinate transformation */
    fiff_int_t nave;                        /**< Number of averages used to regularize the solution. Set to 1 on single Epoch by default.*/
    QList<FiffProj> projs;                  /**< SSP operator */
    MatrixXd proj;                          /**< The projector to apply to the data. */
    MatrixXd whitener;                      /**< Whitens the data */
    VectorXd reginv;                        /**< The diagonal matrix implementing. regularization and the inverse */
    SparseMatrix<double> noisenorm;         /**< These are the noise-normalization factors */
};

} // NAMESPACE

#endif // MNE_INVERSE_OPERATOR_H
