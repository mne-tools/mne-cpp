//=============================================================================================================
/**
 * @file     SignalModel.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief     SignalModel class declaration.
 *
 */

#ifndef SignalModel_H
#define SignalModel_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

/**
 * The strucut specifing important frequencies used for hpi fitting.
 */
struct Frequencies {
    int iLineFreq;
    int iSampleFreq;
    QVector<int> vecHpiFreqs;
};

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT SignalModel
{

public:
    typedef QSharedPointer<SignalModel> SPtr;            /**< Shared pointer type for SignalModel. */
    typedef QSharedPointer<const SignalModel> ConstSPtr; /**< Const shared pointer type for SignalModel. */

    //=========================================================================================================
    /**
     * Constructs a SignalModel object.
     *
     * @param[in] frequencies     The frequencies.
     * @param[in] bBasicModel     Compute the basic model yes/no.
     */
    SignalModel(const Frequencies frequencies, bool bBasicModel);

    //=========================================================================================================
    /**
     * Fit the data to the model.
     *
     * @param[in] matData     The data matrix.
     *
     */
    Eigen::MatrixXd fitData(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Set the model to use. The basic model only contains sines and cosines of the hpi frequencies, the advanced yields the linefrequency and its harmonics as well.
     *
     * @param[in] bBasicModel     Compute the basic model yes/no.
     *
     */
    void setModelType(const bool bBasic);

    //=========================================================================================================
    /**
     * Update the frequencies used for the signal model
     *
     * @param[in] frequencies     The frequencies.
     *
     */
    void updateFrequencies(const Frequencies frequencies);

    inline Eigen::MatrixXd getModel() const;

protected:

private:
    //=========================================================================================================
    /**
     * Computes the model.
     *
     * @param[in] bBasicModel  weather to compute the basic model or the advanced.
     *
     */
    void selectModelAndCompute(const bool bBasicModel);

    //=========================================================================================================
    /**
     * Computes the model.
     *
     */
    void computeInverseBasicModel();
    void computeInverseAdvancedModel();

    //=========================================================================================================
    /**
     * Check if dimensions of input data match the model.
     *
     * @param[in] matData     The data matrix.
     *
     * @return true if changed
     *
     */
    bool checkDataDimensions(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Check if the frequencies changed.
     *
     * @param[in] iSamplingFreq     The sampling frequency.
     * @param[in] iLineFreq     The line frequency.
     * @param[in] vecHpiFreqs     The hpi frequencies.
     *
     * @return true if changed
     */
    bool checkFrequencies(const Frequencies frequencies);

    Eigen::MatrixXd m_matInverseSignalModel;
    int m_iCurrentModelCols;
    bool m_bBasicModel;
    Frequencies m_frequencies;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
inline Eigen::MatrixXd SignalModel::getModel() const
{
    return m_matInverseSignalModel;
}
} // namespace INVERSELIB

#endif // SignalModel_H

