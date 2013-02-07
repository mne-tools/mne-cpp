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
    MNEForwardSolution pick_channels_forward(const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList) const;

    //=========================================================================================================
    /**
    * mne.fiff.pick_types_forward
    *
    * Pick by channel type and names from a forward operator
    *
    * @param[in] info       Fiff measurement info
    * @param[in] meg        Include MEG channels
    * @param[in] eeg        Include EEG channels
    * @param[in] include    Additional channels to include (if empty, do not add any)
    * @param[in] exclude    Channels to exclude (if empty, do not exclude any)
    *
    * @return Forward solution restricted to selected channel types.
    */
    MNEForwardSolution pick_types_forward(const FiffInfo &info, bool meg, bool eeg, const QStringList& include = defaultQStringList, const QStringList& exclude = defaultQStringList) const;

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
    * @param [in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket
    * @param [out] fwd          A forward solution from a fif file
    * @param [in] force_fixed   Force fixed source orientation mode? (optional)
    * @param [in] surf_ori      Use surface based source coordinate system? (optional)
    * @param [in] include       Include these channels (optional)
    * @param [in] exclude       Exclude these channels (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_forward_solution(QIODevice& p_IODevice, MNEForwardSolution& fwd, bool force_fixed = false, bool surf_ori = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);


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
