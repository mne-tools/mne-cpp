//=============================================================================================================
/**
* @file     mne_forwardsolution.h
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
* @brief     MNEForwardSolution class declaration, which provides the forward solution including
*           the source space (MNESourceSpace).
*
*/

#ifndef MNE_FORWARDSOLUTION_H
#define MNE_FORWARDSOLUTION_H


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// FS INCLUDES
//=============================================================================================================

#include <fs/annotation.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_sourcespace.h"

#include <fiff/fiff_info_base.h>
#include <mneMath/mnemath.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/Sparse>
#include <Eigen/unsupported/KroneckerProduct>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>


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

using namespace Eigen;
using namespace MNEMATHLIB;
using namespace FIFFLIB;
using namespace FSLIB;


//=============================================================================================================
/**
* Forward operator
*
* @brief Forward operator
*/
class MNESHARED_EXPORT MNEForwardSolution
{
public:
    typedef QSharedPointer<MNEForwardSolution> SPtr;            /**< Shared pointer type for MNEForwardSolution. */
    typedef QSharedPointer<const MNEForwardSolution> ConstSPtr; /**< Const shared pointer type for MNEForwardSolution. */

    //=========================================================================================================
    /**
    * Default constructor.
    */
    MNEForwardSolution();

    //=========================================================================================================
    /**
    * Constructs a forward operator, by reading from a IO device.
    *
    * @param[in] p_IODevice     IO device to read from the forward operator.
    * @param[in] force_fixed   Force fixed source orientation mode? (optional)
    * @param[in] surf_ori      Use surface based source coordinate system? (optional)
    * @param[in] include       Include these channels (optional)
    * @param[in] exclude       Exclude these channels (optional)
    * @param[in] bExcludeBads  If true bads are also read; default = false (optional)
    *
    */
    MNEForwardSolution(QIODevice &p_IODevice, bool force_fixed = false, bool surf_ori = false, const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList, bool bExcludeBads = false);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MNEForwardSolution   MNE forward solution
    */
    MNEForwardSolution(const MNEForwardSolution &p_MNEForwardSolution);

    //=========================================================================================================
    /**
    * Destroys the MNEForwardSolution.
    */
    ~MNEForwardSolution();

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
    * @param[out] p_fwdOut          clustered MNE forward solution
    * @param[in] p_LHAnnotation     Annotation of the left hemisphere
    * @param[in] p_RHAnnotation     Annotation of the right hemisphere
    * @param[in] p_iClusterSize     Maximal cluster size per roi
    *
    * @return true if succeeded, false otherwise
    */
    bool cluster_forward_solution(MNEForwardSolution &p_fwdOut, const Annotation &p_LHAnnotation, const Annotation &p_RHAnnotation, qint32 p_iClusterSize);

    //=========================================================================================================
    /**
    * Compute weighting for depth prior
    *
    * @param[in] Gain               gain matrix
    * @param[in] gain_info          The measurement info to specify the channels to include.
    * @param[in] is_fixed_ori       Fixed orientation?
    * @param[in] exp                float in [0, 1]. Depth weighting coefficients. If None, no depth weighting is performed. (optional; default = 0.8)
    * @param[in] limit              (optional; default = 10.0)
    * @param[in] patch_areas        (optional)
    * @param[in] limit_depth_chs    If True, use only grad channels in depth weighting (equivalent to MNE C code). If grad chanels aren't present, only mag channels will be used (if no mag, then eeg). If False, use all channels. (optional)
    *
    * @return the depth prior
    */
    static MatrixXd compute_depth_prior(const MatrixXd &Gain, const FiffInfo &gain_info, bool is_fixed_ori, double exp = 0.8, double limit = 10.0, MatrixXd &patch_areas = defaultMatrixXd, bool limit_depth_chs = false);

    //=========================================================================================================
    /**
    * True if FIFF measurement file information is empty.
    *
    * @return true if FIFF measurement file information is empty
    */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
    * Has forward operator fixed orientation?
    *
    * @return true if forward operator has fixed orientation, false otherwise
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
    MNEForwardSolution pick_channels(const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
    * mne.fiff.pick_types_forward
    *
    * Pick by channel type and names from a forward operator
    *
    * @param[in] meg        Include MEG channels
    * @param[in] eeg        Include EEG channels
    * @param[in] include    Additional channels to include (if empty, do not add any)
    * @param[in] exclude    Channels to exclude (if empty, do not exclude any)
    *
    * @return Forward solution restricted to selected channel types.
    */
    MNEForwardSolution pick_types(bool meg, bool eeg, const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
    *
    */
//    VectorXi tripletSelection(VectorXi& p_vecIdxSelection)
//    {
//        MatrixXi triSelect = p_vecIdxSelection.transpose().replicate(3,1).array() * 3;//repmat((p_vecIdxSelection - 1) * 3 + 1, 3, 1);
//        triSelect.row(1).array() += 1;
//        triSelect.row(2).array() += 2;
//        VectorXi retTriSelect(triSelect.cols()*3);
//        for(int i = 0; i < triSelect.cols(); ++i)
//            retTriSelect.block(i*3,0,3,1) = triSelect.col(i);
//        return retTriSelect;
//    } // tripletSelection


    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_forward_solution function
    *
    * Reads a forward solution from a fif file
    *
    * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket
    * @param[out] fwd          A forward solution from a fif file
    * @param[in] force_fixed   Force fixed source orientation mode? (optional)
    * @param[in] surf_ori      Use surface based source coordinate system? (optional)
    * @param[in] include       Include these channels (optional)
    * @param[in] exclude       Exclude these channels (optional)
    * @param[in] bExcludeBads  If true bads are also read; default = false (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_forward_solution(QIODevice& p_IODevice, MNEForwardSolution& fwd, bool force_fixed = false, bool surf_ori = false, const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList, bool bExcludeBads = true);

//    //=========================================================================================================
//    /**
//    * Prepares a forward solution, Bad channels, after clustering etc ToDo...
//    *
//    * @param [in] p_FiffInfo   Fif measurement info
//    *
//    */
//    void prepare_forward(const FiffInfo& p_FiffInfo)
//    {
//        QStringList fwdChNames = this->sol->row_names;
//        QStringList chNames;
//        for(qint32 i = 0; i < p_FiffInfo.ch_names.size(); ++i)
//        {
//            bool inBads = false;
//            bool inFwd = false;

//            for(qint32 j = 0; j < p_FiffInfo.bads.size(); ++j)
//            {
//                if(QString::compare(p_FiffInfo.bads[j], p_FiffInfo.ch_names[i]) == 0)
//                {
//                    inBads = true;
//                    break;
//                }
//            }

//            for(qint32 j = 0; j < fwdChNames.size(); ++j)
//            {
//                if(QString::compare(fwdChNames[j], p_FiffInfo.ch_names[i]) == 0)
//                {
//                    inFwd = true;
//                    break;
//                }
//            }

//            if(!inBads && inFwd)
//                chNames.append(p_FiffInfo.ch_names[i]);
//        }

//        qint32 nchan = chNames.size();
//    }

    //=========================================================================================================
    /**
    * Prepare forward for assembling the inverse operator
    *
    * @param[in] p_info             The measurement info to specify the channels to include. Bad channels in info['bads'] are not used.
    * @param[in] p_noise_cov        The noise covariance matrix.
    * @param[in] p_pca              Calculate pca or not.
    * @param[out] ch_names          Selected channel names
    * @param[out] gain              Gain matrix
    * @param[out] p_outNoiseCov     noise covariance matrix
    * @param[out] p_outWhitener     Whitener
    * @param[out] p_outNumNonZero   the rank (non zeros)
    */
    void prepare_forward(const FiffInfo &p_info, const FiffCov &p_noise_cov, bool p_pca, FiffInfo &p_outFwdInfo, MatrixXd &gain, FiffCov &p_outNoiseCov, MatrixXd &p_outWhitener, qint32 &p_outNumNonZero);

    //=========================================================================================================
    /**
    * Restrict gain matrix entries for optimal depth weighting
    *
    * @param[in, out] G     Gain matrix to be restricted; result is stored in place.
    * @param[in] info       Fiff information
    */
    static void restrict_gain_matrix(MatrixXd &G, const FiffInfo &info);

private:
    //=========================================================================================================
    /**
    * Implementation of the read_one function in mne_read_forward_solution.m
    *
    * Reads all interesting stuff for one forward solution
    *
    * @param[in] p_pStream  The opened fif file to read from
    * @param[in] p_Node     The forward solution node
    * @param[out] one       The read forward solution
    *
    * @return True if succeeded, false otherwise
    */
    static bool read_one(FiffStream* p_pStream, const FiffDirTree& p_Node, MNEForwardSolution& one);

public:
    FiffInfoBase info;                  /**< light weighted measurement info */
    fiff_int_t source_ori;              /**< Source orientation: fixed or free */
    fiff_int_t coord_frame;             /**< Coil coordinate system definition */
    fiff_int_t nsource;                 /**< Number of source dipoles */
    fiff_int_t nchan;                   /**< Number of channels */
    FiffNamedMatrix::SDPtr sol;         /**< Forward solution */
    FiffNamedMatrix::SDPtr sol_grad;    /**< ToDo... */
    FiffCoordTrans mri_head_t;          /**< MRI head coordinate transformation */
    MNESourceSpace src;                 /**< Geomertic description of the source spaces (hemispheres) */
    MatrixX3d source_rr;                /**< Source locations */
    MatrixX3d source_nn;                /**< Source normals (number depends on fixed or free orientation) */

    bool isClustered;   /**< Indicates whether fwd conatins a clustered forward solution. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEForwardSolution::isEmpty() const
{
    return this->nchan <= 0;
}


//*************************************************************************************************************

inline bool MNEForwardSolution::isFixedOrient() const
{
    return this->source_ori == FIFFV_MNE_FIXED_ORI;
}

} // NAMESPACE

#endif // MNE_FORWARDSOLUTION_H
