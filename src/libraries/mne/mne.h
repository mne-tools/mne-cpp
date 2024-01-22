//=============================================================================================================
/**
 * @file     mne.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief     MNE class declaration, which provides static wrapper functions to stay consistent with
 *           mne matlab toolbox
 *
 */

#ifndef MNE_H
#define MNE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "mne_inverse_operator.h"
#include "mne_forwardsolution.h"
#include "mne_hemisphere.h"
#include "mne_sourcespace.h"
#include "mne_surface.h"
#include "mne_bem.h"
#include "mne_bem_surface.h"
#include "mne_epoch_data_list.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_cov.h>

#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE MNE WRAPPER CLASS
 * @brief The MNE class provides wrapper functions to stay consistent with mne matlab toolbox.
 */

class MNESHARED_EXPORT MNE
{

public:

    //=========================================================================================================
    /**
     * dtor
     */
    virtual ~MNE()
    { }

    //=========================================================================================================
    /**
     * mne_combine_xyz
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEMath::combine_xyz static function
     *
     * Compute the three Cartesian components of a vector together
     *
     * @param[in] vec    Input row vector [ x1 y1 z1 ... x_n y_n z_n ].
     *
     * @return Output vector [x1^2+y1^2+z1^2 ... x_n^2+y_n^2+z_n^2 ].
     */
    inline static Eigen::VectorXd* combine_xyz(const Eigen::VectorXd& vec)
    {
        return UTILSLIB::MNEMath::combine_xyz(vec);
    }

    //=========================================================================================================
    /**
     * mne_block_diag - decoding part
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEForwardSolution::extract_block_diag static function
     */
    //    static inline Eigen::MatrixXd extract_block_diag(MatrixXd& A, qint32 n);

    //=========================================================================================================
    /**
     * mne_find_source_space_hemi
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNESourceSpace::find_source_space_hemi static function
     *
     * Returns the hemisphere id (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
     *
     * @param[in] p_Hemisphere   The hemisphere to investigate.
     *
     * @return the deduced hemisphere id.
     */
    inline static qint32 find_source_space_hemi(MNEHemisphere& p_Hemisphere)
    {
        return MNESourceSpace::find_source_space_hemi(p_Hemisphere);
    }

    //=========================================================================================================
    /**
     * mne_get_current_comp
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo get_current_comp member function
     *
     * Get the current compensation in effect in the data
     *
     * @param[in] info   Fiff measurement info.
     *
     * @return the current compensation.
     */
    static inline qint32 get_current_comp(FIFFLIB::FiffInfo* info)
    {
        return info->get_current_comp();
    }

    //ToDo Why is make_block_diag part of MNEForwardSolution - restructure this
    //=========================================================================================================
    /**
     * mne_block_diag - encoding part
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEForwardSolution::make_block_diag static function
     *
     * Make a sparse block diagonal matrix
     *
     * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
     * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
     * the diagonal of the matrix.
     *
     * @param[in, out] A Matrix which should be diagonlized.
     * @param[in, out] n Columns of the submatrices.
     *
     * @return A sparse block diagonal, diagonalized from the elements in "A".
     */
    static inline Eigen::SparseMatrix<double>* make_block_diag(const Eigen::MatrixXd &A, qint32 n)
    {
        return UTILSLIB::MNEMath::make_block_diag(A, n);
    }

    //=========================================================================================================
    /**
     * mne_make_compensator
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo make_compensator member function
     *
     * Create a compensation matrix to bring the data from one compensation state to another
     *
     * @param[in] info               measurement info as returned by the fif reading routines.
     * @param[in] from               compensation in the input data.
     * @param[in] to                 desired compensation in the output.
     * @param[out] ctf_comp          Compensation Matrix.
     * @param[in] exclude_comp_chs   exclude compensation channels from the output (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool make_compensator(const FIFFLIB::FiffInfo& info,
                                        FIFFLIB::fiff_int_t from,
                                        FIFFLIB::fiff_int_t to,
                                        FIFFLIB::FiffCtfComp& ctf_comp,
                                        bool exclude_comp_chs = false)
    {
        return info.make_compensator(from, to, ctf_comp, exclude_comp_chs);
    }

    //=========================================================================================================
    /**
     * make_projector
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo::make_projector static function
     * There exists also a member function which should be preferred:
     * make_projector(MatrixXd& proj, Eigen::MatrixXd& U = defaultUMatrix)
     *
     * Make an SSP operator
     *
     * @param[in] projs      A set of projection vectors.
     * @param[in] ch_names   A cell array of channel names.
     * @param[out] proj      The projection operator to apply to the data.
     * @param[in] bads       Bad channels to exclude.
     * @param[out] U         The orthogonal basis of the projection vectors (optional).
     *
     * @return nproj - How many items in the projector.
     */
    inline static FIFFLIB::fiff_int_t make_projector(const QList<FIFFLIB::FiffProj>& projs,
                                                     const QStringList& ch_names,
                                                     Eigen::MatrixXd& proj,
                                                     const QStringList& bads = FIFFLIB::defaultQStringList,
                                                     Eigen::MatrixXd& U = FIFFLIB::defaultMatrixXd)
    {
        return FIFFLIB::FiffProj::make_projector(projs,
                                                 ch_names,
                                                 proj,
                                                 bads,
                                                 U);
    }

    //=========================================================================================================
    /**
     * mne_make_projector_info
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo make_projector_info(MatrixXd& proj) member function
     *
     * Make a SSP operator using the meas info
     *
     * @param[in] info       Fiff measurement info.
     * @param[out] proj      The projection operator to apply to the data.
     *
     * @return nproj - How many items in the projector.
     */
    static inline qint32 make_projector(FIFFLIB::FiffInfo& info,
                                        Eigen::MatrixXd& proj)
    {
        return info.make_projector(proj);
    }

    //=========================================================================================================
    /**
     * mne_patch_info
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNESourceSpace::patch_info static function
     *
     * @param[in, out] p_Hemisphere  The source space.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool patch_info(MNEHemisphere &p_Hemisphere)
    {
        return MNESourceSpace::patch_info(p_Hemisphere);
    }

    //=========================================================================================================
    /**
     * mne_prepare_inverse_operator
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEInverseOperator prepare_inverse_operator member function
     *
     * Prepare for actually computing the inverse
     *
     * @param[in] orig      The inverse operator structure read from a file.
     * @param[in] nave      Number of averages (scales the noise covariance).
     * @param[in] lambda2   The regularization factor.
     * @param[in] dSPM      Compute the noise-normalization factors for dSPM?.
     * @param[in] sLORETA   Compute the noise-normalization factors for sLORETA?.
     *
     * @return the prepared inverse operator.
     */
    inline static MNEInverseOperator prepare_inverse_operator(MNEInverseOperator& orig,
                                                              qint32 nave,
                                                              float lambda2,
                                                              bool dSPM,
                                                              bool sLORETA = false)
    {
        return orig.prepare_inverse_operator(nave, lambda2, dSPM, sLORETA);
    }

    static bool read_events(QString t_sEventName,
                            QString t_fileRawName,
                            Eigen::MatrixXi& events);

// ToDo Eventlist Class??
    //=========================================================================================================
    /**
     * mne_read_events
     *
     * ### MNE toolbox root function ###
     *
     * Read an event list from a fif file
     *
     * @param[in] p_IODevice   The I/O device to read from.
     * @param[in, out] eventlist    The read eventlist m x 3; with m events; colum: 1 - position in samples, 3 - eventcode.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_events_from_fif(QIODevice &p_IODevice,
                                     Eigen::MatrixXi& eventlist);

    //=========================================================================================================
    /**
     * read_events
     *
     * Read a list of events from an eve file
     *
     * @param[in] p_IODevice   The I/O device to read from.
     * @param[in, out] eventList   List of events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_events_from_ascii(QIODevice &p_IODevice,
                                       Eigen::MatrixXi& eventlist);

    static void setup_compensators(FIFFLIB::FiffRawData& raw,
                                   FIFFLIB::fiff_int_t dest_comp,
                                   bool keep_comp);

    //=========================================================================================================
    /**
     * mne_read_cov
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream read_cov member function
     *
     * Reads a covariance matrix from a fiff file
     *
     * @param[in] p_pStream     an open fiff file.
     * @param[in] p_Node        look for the matrix in here.
     * @param[in] cov_kind      what kind of a covariance matrix do we want?.
     * @param[in, out] p_covData    the read covariance matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool read_cov(FIFFLIB::FiffStream::SPtr& p_pStream,
                                const FIFFLIB::FiffDirNode::SPtr& p_Node,
                                FIFFLIB::fiff_int_t cov_kind,
                                FIFFLIB::FiffCov& p_covData)
    {
        return p_pStream->read_cov(p_Node,
                                   cov_kind,
                                   p_covData);
    }

    //=========================================================================================================
    /**
     * mne_read_inverse_operator
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEInverseOperator::read_inverse_operator static function
     *
     * Reads the inverse operator decomposition from a fif file
     *
     * @param[in] p_pIODevice   A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in, out] inv          The read inverse operator.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_inverse_operator(QIODevice& p_pIODevice,
                                      MNEInverseOperator& inv)
    {
        return MNEInverseOperator::read_inverse_operator(p_pIODevice,
                                                         inv);
    }

    //=========================================================================================================
    /**
     * mne_read_forward_solution
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNEForwardSolution::read_forward_solution static function
     *
     * Reads a forward solution from a fif file
     *
     * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in, out] fwd A forward solution from a fif file.
     * @param[in] force_fixed   Force fixed source orientation mode? (optional).
     * @param[in] surf_ori      Use surface based source coordinate system? (optional).
     * @param[in] include       Include these channels (optional).
     * @param[in] exclude       Exclude these channels (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_forward_solution(QIODevice& p_IODevice,
                                             MNEForwardSolution& fwd,
                                             bool force_fixed = false,
                                             bool surf_ori = false,
                                             const QStringList& include = FIFFLIB::defaultQStringList,
                                             const QStringList& exclude = FIFFLIB::defaultQStringList)
    {
        return MNEForwardSolution::read(p_IODevice,
                                        fwd,
                                        force_fixed,
                                        surf_ori,
                                        include,
                                        exclude);
    }

    //=========================================================================================================
    /**
     * mne_read_forward_solution
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNESourceSpace::read_source_spaces static function
     *
     * Reads source spaces from a fif file
     *
     * @param[in] p_pStream         The open fiff file.
     * @param[in] add_geom          Add geometry information to the source spaces.
     *
     * @param[in, out] p_SourceSpace    The read source spaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_source_spaces(FIFFLIB::FiffStream::SPtr& p_pStream,
                                   bool add_geom,
                                   MNESourceSpace& p_SourceSpace)
    {
        return MNESourceSpace::readFromStream(p_pStream,
                                              add_geom,
                                              p_SourceSpace);
    }

    //=========================================================================================================
    /**
     * mne_read_bem_surface
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNESurface::read static function
     *
     * Reads a BEM surface from a fif stream
     *
     * @param[in] p_pStream         The open fiff file.
     * @param[in] add_geom          Add geometry information to the source spaces.
     * @param[in] p_Tree            Search for the source spaces here.
     *
     * @param[in, out] p_Surfaces       The read bem surfaces.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_bem_surface(FIFFLIB::FiffStream::SPtr& p_pStream,
                                 bool add_geom,
                                 FIFFLIB::FiffDirNode::SPtr& p_Tree,
                                 QList<MNESurface::SPtr>& p_Surfaces)
    {
        return MNESurface::read(p_pStream,
                                add_geom,
                                p_Tree,
                                p_Surfaces);
    }

    //ToDo FiffChInfoList Class
    //=========================================================================================================
    /**
     * mne_set_current_comp
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo::set_current_comp static function
     * Consider taking the member function of a FiffInfo set_current_comp(fiff_int_t value),
     * when compensation should be applied to the channels of FiffInfo
     *
     * Set the current compensation value in the channel info structures
     *
     * @param[in] chs    fiff channel info list.
     * @param[in] value  compensation value.
     *
     * @return the current compensation.
     */
    static QList<FIFFLIB::FiffChInfo> set_current_comp(QList<FIFFLIB::FiffChInfo>& chs,
                                              FIFFLIB::fiff_int_t value)
    {
        return FIFFLIB::FiffInfo::set_current_comp(chs,
                                                   value);
    }

    //=========================================================================================================
    /**
     * mne_transform_source_space_to
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the MNESourceSpace transform_source_space_to member function
     *
     * Transforms source space data to the desired coordinate system
     *
     * @param[in, out] p_pMNESourceSpace the source space which is should be transformed.
     * @param[in] dest destination check code.
     * @param[in] trans transformation information.
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool transform_source_space_to(MNESourceSpace& p_pMNESourceSpace,
                                                 FIFFLIB::fiff_int_t dest,
                                                 FIFFLIB::FiffCoordTrans& trans)
    {
        return p_pMNESourceSpace.transform_source_space_to(dest,
                                                           trans);
    }

    //=========================================================================================================
    /**
     * mne_transpose_named_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffNamedMatrix transpose_named_matrix member  function
     *
     * Transpose a named matrix (FiffNamedMatrix)
     *
     * @param[in, out] mat FiffNamedMatrix which shoul be transposed.
     *
     */
    static inline void transpose_named_matrix(FIFFLIB::FiffNamedMatrix& mat)
    {
        mat.transpose_named_matrix();
    }
};
} // NAMESPACE

#endif // MNE_H
