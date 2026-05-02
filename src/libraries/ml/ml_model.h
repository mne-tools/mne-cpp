//=============================================================================================================
/**
 * @file     ml_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MlModel pure-abstract base class declaration.
 *
 */

#ifndef ML_MODEL_H
#define ML_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_types.h"
#include "ml_tensor.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB{

//=============================================================================================================
/**
 * @brief Abstract interface for all ML models.
 */
class MLSHARED_EXPORT MlModel
{
public:
    typedef QSharedPointer<MlModel> SPtr;   /**< Shared pointer type for MlModel. */

    //=========================================================================================================
    /**
     * Virtual destructor.
     */
    virtual ~MlModel() = default;

    //=========================================================================================================
    /**
     * Run inference on the given input tensor.
     *
     * @param[in] input   The input data.
     * @return The prediction result.
     */
    virtual MlTensor predict(const MlTensor& input) const = 0;

    //=========================================================================================================
    /**
     * Serialise the model to disk.
     *
     * @param[in] path   File path.
     * @return True if successful.
     */
    virtual bool save(const QString& path) const = 0;

    //=========================================================================================================
    /**
     * Load a model from disk.
     *
     * @param[in] path   File path.
     * @return True if successful.
     */
    virtual bool load(const QString& path) = 0;

    //=========================================================================================================
    /**
     * @return Human-readable model type name.
     */
    virtual QString modelType() const = 0;

    //=========================================================================================================
    /**
     * @return The task type this model is configured for.
     */
    virtual MlTaskType taskType() const = 0;
};

} // namespace MLLIB

#endif // ML_MODEL_H
