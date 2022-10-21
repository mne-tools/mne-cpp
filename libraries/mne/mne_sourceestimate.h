
//=============================================================================================================
/**
 * @file     mne_sourceestimate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     MNESourceEstimate class declaration.
 *
 */

#ifndef MNESOURCEESTIMATE_H
#define MNESOURCEESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fs/label.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QIODevice>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Source estimation which holds results of MNE-CPP inverse routines. (Replaces *mneStcData,mneStcDataRec struct of MNE-C mne_types.h).
 *
 * @brief Source estimation
 */
class MNESHARED_EXPORT MNESourceEstimate
{
public:
    typedef QSharedPointer<MNESourceEstimate> SPtr;             /**< Shared pointer type for MNESourceEstimate. */
    typedef QSharedPointer<const MNESourceEstimate> ConstSPtr;  /**< Const shared pointer type for MNESourceEstimate. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    MNESourceEstimate();

    //=========================================================================================================
    /**
     * Constructs a source estimation from given data
     *
     * @param[in] p_sol.
     * @param[in] p_vertices.
     * @param[in] p_tmin.
     * @param[in] p_tstep.
     */
    MNESourceEstimate(const Eigen::MatrixXd &p_sol, const Eigen::VectorXi &p_vertices, float p_tmin, float p_tstep);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_SourceEstimate    Source estimate data which should be copied.
     */
    MNESourceEstimate(const MNESourceEstimate& p_SourceEstimate);

    //=========================================================================================================
    /**
     * Constructs a source estimation, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the source estimation.
     *
     */
    MNESourceEstimate(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Initializes source estimate.
     */
    void clear();

    //=========================================================================================================
    /**
     * Reduces the source estimate to selected samples.
     *
     * @param[in] start  The start index to cut the estimate from.
     * @param[in] n      Number of samples to cut from start index.
     */
    MNESourceEstimate reduce(qint32 start, qint32 n);

    //=========================================================================================================
    /**
     * mne_read_stc_file
     *
     * Reads a source estimate from a given file
     *
     * @param[in] p_IODevice    IO device to red the stc from.
     * @param[in, out] p_stc        the read stc.
     *
     * @return true if successful, false otherwise.
     */
    static bool read(QIODevice &p_IODevice, MNESourceEstimate& p_stc);

    //=========================================================================================================
    /**
     * mne_write_stc_file
     *
     * Writes a stc file
     *
     * @param[in] p_IODevice   IO device to write the stc to.
     */
    bool write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Returns whether SourceEstimate is empty.
     *
     * @return true if is empty, false otherwise.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     SourceEstimate which should be assigned.
     *
     * @return the copied source estimate.
     */
    MNESourceEstimate& operator= (const MNESourceEstimate &rhs);

    //=========================================================================================================
    /**
     * Returns the number of samples.
     *
     * @return the number of samples.
     */
    int samples() const;

    //=========================================================================================================
    /**
     * Returns the indices of sources in the data matrix based on their beloning label.
     *
     * @param[in] lPickedLabels      The labels base the selection on.
     * @param[in] bIsClustered       Whether the source space was clustered.
     *
     * @return the indices.
     */
    Eigen::VectorXi getIndicesByLabel(const QList<FSLIB::Label> &lPickedLabels, bool bIsClustered) const;

public:
    Eigen::MatrixXd data;           /**< Matrix of shape [n_dipoles x n_times] which contains the data in source space. */
    Eigen::VectorXi vertices;       /**< The indices of the dipoles in the different source spaces. */ //ToDo define is_clustered_result; in clustered case vertices holds the ROI idcs
    Eigen::RowVectorXf times;       /**< The time vector with n_times steps. */
    float tmin;                     /**< Time starting point. */
    float tstep;                    /**< Time steps within the times vector. */

private:
    //=========================================================================================================
    /**
     * Update the times attribute after changing tmin, tmax, or tstep
     */
    void update_times();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNESourceEstimate::isEmpty() const
{
    return tstep == -1;
}
} //NAMESPACE

#ifndef metatype_mnesourceestimate
Q_DECLARE_METATYPE(MNELIB::MNESourceEstimate);/**< Provides QT META type declaration of the MNELIB::MNEForwardSolution type. For signal/slot and QVariant usage.*/
#endif

#ifndef metatype_mnesourceestimatesptr
#define metatype_mnesourceestimatesptr
Q_DECLARE_METATYPE(MNELIB::MNESourceEstimate::SPtr);/**< Provides QT META type declaration of the MNELIB::MNEForwardSolution::SPtr type. For signal/slot and QVariant usage.*/
#endif

#endif // MNESOURCEESTIMATE_H
