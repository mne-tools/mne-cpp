//=============================================================================================================
/**
 * @file     minimumnorm.h
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

#ifndef MINIMUMNORM_H
#define MINIMUMNORM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "../IInverseAlgorithm.h"

#include <mne/mne_inverse_operator.h>
#include <fs/label.h>

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Minimum norm estimation algorithm ToDo: Paper references.
 *
 * @brief Minimum norm estimation
 */
class INVERSESHARED_EXPORT MinimumNorm : public IInverseAlgorithm
{
public:
    typedef QSharedPointer<MinimumNorm> SPtr;             /**< Shared pointer type for MinimumNorm. */
    typedef QSharedPointer<const MinimumNorm> ConstSPtr;  /**< Const shared pointer type for MinimumNorm. */

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
    explicit MinimumNorm(const MNELIB::MNEInverseOperator &p_inverseOperator, float lambda, const QString method);

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
    explicit MinimumNorm(const MNELIB::MNEInverseOperator &p_inverseOperator, float lambda, bool dSPM, bool sLORETA);

    virtual ~MinimumNorm(){}

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
    virtual MNELIB::MNESourceEstimate calculateInverse(const FIFFLIB::FiffEvoked &p_fiffEvoked, bool pick_normal = false);

    virtual MNELIB::MNESourceEstimate calculateInverse(const Eigen::MatrixXd &data, float tmin, float tstep, bool pick_normal = false) const;

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
    virtual const MNELIB::MNESourceSpace& getSourceSpace() const;

    //=========================================================================================================
    /**
     * Get the prepared inverse operator.
     *
     * @return the prepared inverse operator.
     */
    inline MNELIB::MNEInverseOperator& getPreparedInverseOperator();

    //=========================================================================================================
    /**
     * Set minimum norm algorithm method ("MNE" | "dSPM" | "sLORETA")
     *
     * @param[in] method   Use mininum norm, dSPM or sLORETA.
     */
    void setMethod(QString method);

    //=========================================================================================================
    /**
     * Set minimum norm algorithm method ("MNE" | "dSPM" | "sLORETA")
     *
     * @param[in] dSPM      Compute the noise-normalization factors for dSPM?.
     * @param[in] sLORETA   Compute the noise-normalization factors for sLORETA?.
     */
    void setMethod(bool dSPM, bool sLORETA);

    //=========================================================================================================
    /**
     * Set regularization factor
     *
     * @param[in] lambda   The regularization factor.
     */
    void setRegularization(float lambda);

    //=========================================================================================================
    /**
     * Get the assembled kernel
     *
     * @return the assembled kernel.
     */
    inline Eigen::MatrixXd& getKernel();

private:
    MNELIB::MNEInverseOperator m_inverseOperator;   /**< The inverse operator. */
    float m_fLambda;                                /**< Regularization parameter. */
    QString m_sMethod;                              /**< Selected method. */
    bool m_bsLORETA;                                /**< Do sLORETA method. */
    bool m_bdSPM;                                   /**< Do dSPM method. */

    bool inverseSetup;                              /**< Inverse Setup Calcluated. */
    MNELIB::MNEInverseOperator inv;                 /**< The setup inverse operator. */
    Eigen::SparseMatrix<double> noise_norm;         /**< The noise normalization. */
    QList<Eigen::VectorXi> vertno;                  /**< The vertices numbers. */
    FSLIB::Label label;                             /**< The corresponding labels. */
    Eigen::MatrixXd K;                              /**< Imaging kernel. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Eigen::MatrixXd& MinimumNorm::getKernel()
{
    return K;
}

//=============================================================================================================

inline MNELIB::MNEInverseOperator& MinimumNorm::getPreparedInverseOperator()
{
    return inv;
}
} //NAMESPACE

#endif // MINIMUMNORM_H
