//=============================================================================================================
/**
 * @file     inv_minimum_norm.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Minimum norm class declaration.
 *
 */

#ifndef INV_MINIMUM_NORM_H
#define INV_MINIMUM_NORM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

#include <mne/mne_inverse_operator.h>
#include "../inv_source_estimate.h"
#include <fs/fs_label.h>

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Minimum norm estimation algorithm.
 *
 * Computes L2 minimum-norm, dSPM, or sLORETA source estimates from MEG/EEG data
 * using a pre-computed inverse operator.
 *
 * References:
 * - Hamalainen & Ilmoniemi, Med. & Biol. Eng. & Comput. 32, 35-42, 1994.
 * - Dale et al., Neuron 26, 55-67, 2000 (dSPM).
 * - Pascual-Marqui, Methods Find. Exp. Clin. Pharmacol. 24D, 5-12, 2002 (sLORETA).
 *
 * @brief Minimum norm estimation
 */
class INVSHARED_EXPORT InvMinimumNorm
{
public:
    typedef QSharedPointer<InvMinimumNorm> SPtr;             /**< Shared pointer type for InvMinimumNorm. */
    typedef QSharedPointer<const InvMinimumNorm> ConstSPtr;  /**< Const shared pointer type for InvMinimumNorm. */

    //=========================================================================================================
    /**
     * Constructs minimum norm inverse algorithm
     *
     * @param[in] p_inverseOperator  The inverse operator.
     * @param[in] lambda             The regularization factor.
     * @param[in] method             Use mininum norm, dSPM or sLORETA. ("MNE" | "dSPM" | "sLORETA").
     *
     * @return the prepared inverse operator.
     */
    explicit InvMinimumNorm(const MNELIB::MNEInverseOperator &p_inverseOperator, float lambda, const QString method);

    //=========================================================================================================
    /**
     * Constructs minimum norm inverse algorithm
     *
     * @param[in] p_inverseOperator  The inverse operator.
     * @param[in] lambda             The regularization factor.
     * @param[in] dSPM               Compute the noise-normalization factors for dSPM?.
     * @param[in] sLORETA            Compute the noise-normalization factors for sLORETA?.
     *
     * @return the prepared inverse operator.
     */
    explicit InvMinimumNorm(const MNELIB::MNEInverseOperator &p_inverseOperator, float lambda, bool dSPM, bool sLORETA);

    virtual ~InvMinimumNorm(){}

    //=========================================================================================================
    /**
     * Computes a L2-norm inverse solution Actual code using these principles might be different because the
     * inverse operator is often reused across data sets.
     *
     * @param[in] p_fiffEvoked   Evoked data.
     * @param[in] pick_normal    If True, rather than pooling the orientations by taking the norm, only the.
     *                           radial component is kept. This is only applied when working with loose orientations.
     *
     * @return the calculated source estimation.
     */
    virtual InvSourceEstimate calculateInverse(const FIFFLIB::FiffEvoked &p_fiffEvoked, bool pick_normal = false);

    virtual InvSourceEstimate calculateInverse(const Eigen::MatrixXd &data, float tmin, float tstep, bool pick_normal = false) const;

    //=========================================================================================================
    /**
     * Perform the inverse setup: Prepares this inverse operator and assembles the kernel.
     *
     * @param[in] nave           Number of averages to use.
     * @param[in] pick_normal    If True, rather than pooling the orientations by taking the norm, only the.
     *                           radial component is kept. This is only applied when working with loose orientations.
     */
    virtual void doInverseSetup(qint32 nave, bool pick_normal = false);

    //=========================================================================================================
    /**
     * Get the name of the inverse operator.
     *
     * @return the name of the inverse operator.
     */
    virtual const char* getName() const;

    //=========================================================================================================
    /**
     * Get the source space corresponding to this inverse operator.
     *
     * @return the source space corresponding to this inverse operator.
     */
    virtual const MNELIB::MNESourceSpaces& getSourceSpace() const;

    //=========================================================================================================
    /**
     * Get the prepared inverse operator.
     *
     * @return the prepared inverse operator.
     */
    inline MNELIB::MNEInverseOperator& getPreparedInverseOperator();

    //=========================================================================================================
    /**
     * Set minimum norm algorithm method ("MNE" | "dSPM" | "sLORETA" | "eLORETA")
     *
     * @param[in] method   Use minimum norm, dSPM, sLORETA, or eLORETA.
     */
    void setMethod(QString method);

    //=========================================================================================================
    /**
     * Set minimum norm algorithm method ("MNE" | "dSPM" | "sLORETA" | "eLORETA")
     *
     * @param[in] dSPM      Compute the noise-normalization factors for dSPM?.
     * @param[in] sLORETA   Compute the noise-normalization factors for sLORETA?.
     * @param[in] eLoreta   Compute the eLORETA source weights?.
     */
    void setMethod(bool dSPM, bool sLORETA, bool eLoreta = false);

    //=========================================================================================================
    /**
     * Set regularization factor
     *
     * @param[in] lambda   The regularization factor.
     */
    void setRegularization(float lambda);

    //=========================================================================================================
    /**
     * Set eLORETA options.
     *
     * @param[in] maxIter   Maximum number of eLORETA weight iterations (default: 20).
     * @param[in] eps       Convergence threshold for weight fitting (default: 1e-6).
     * @param[in] forceEqual  If true, use uniform orientation weights (default: false).
     */
    void setELoretaOptions(int maxIter = 20, double eps = 1e-6, bool forceEqual = false);

    //=========================================================================================================
    /**
     * Get the assembled kernel
     *
     * @return the assembled kernel.
     */
    inline Eigen::MatrixXd& getKernel();

private:
    //=========================================================================================================
    /**
     * Compute the eLORETA optimized source covariance and update the inverse operator.
     *
     * Iteratively adjusts source weights R so that the spatial resolution is uniform
     * across depth, providing exact localization for single dipoles.
     *
     * Reference: Pascual-Marqui, 2007. Discrete, 3D distributed, linear imaging
     * methods of electric neuronal activity.
     */
    void computeELoreta();

    MNELIB::MNEInverseOperator m_inverseOperator;   /**< The inverse operator. */
    float m_fLambda;                                /**< Regularization parameter. */
    QString m_sMethod;                              /**< Selected method. */
    bool m_bsLORETA;                                /**< Do sLORETA method. */
    bool m_bdSPM;                                   /**< Do dSPM method. */
    bool m_beLoreta;                                /**< Do eLORETA method. */

    // eLORETA parameters
    int m_iELoretaMaxIter;                          /**< eLORETA max iterations. */
    double m_dELoretaEps;                           /**< eLORETA convergence threshold. */
    bool m_bELoretaForceEqual;                      /**< eLORETA uniform orientation weights. */

    bool inverseSetup;                              /**< Inverse Setup Calculated. */
    MNELIB::MNEInverseOperator inv;                 /**< The setup inverse operator. */
    Eigen::SparseMatrix<double> noise_norm;         /**< The noise normalization. */
    QList<Eigen::VectorXi> vertno;                  /**< The vertices numbers. */
    FSLIB::FsLabel label;                             /**< The corresponding labels. */
    Eigen::MatrixXd K;                              /**< Imaging kernel. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Eigen::MatrixXd& InvMinimumNorm::getKernel()
{
    return K;
}

//=============================================================================================================

inline MNELIB::MNEInverseOperator& InvMinimumNorm::getPreparedInverseOperator()
{
    return inv;
}
} //NAMESPACE

#endif // INV_MINIMUM_NORM_H
