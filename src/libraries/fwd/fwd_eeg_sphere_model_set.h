//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_model_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FwdEegSphereModelSet class declaration.
 *
 */

#ifndef FWD_EEG_SPHERE_MODEL_SET_H
#define FWD_EEG_SPHERE_MODEL_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_eeg_sphere_model.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

#include <memory>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB {

//=============================================================================================================
// FIFFLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Definitions for the EEG Sphere Model Set (Replaces *fwdEegSphereModelSet,fwdEegSphereModelSetRec struct of MNE-C fwd_types.h).
 *
 * @brief Collection of FwdEegSphereModel objects for multi-model EEG forward solutions.
 */

class FWDSHARED_EXPORT FwdEegSphereModelSet
{

public:
    typedef std::unique_ptr<FwdEegSphereModelSet> UPtr;   /**< Unique pointer type for FwdEegSphereModelSet. */

    //=========================================================================================================
    /**
     * Constructs a Forward EEG Sphere Model Set object.
     */
    FwdEegSphereModelSet();

    //=========================================================================================================
    /**
     * Destroys the Forward EEG Sphere Model Set description
     */
    ~FwdEegSphereModelSet();

    //=========================================================================================================
    /**
     * Add a model to a set. The model will be owned by the set.
     *
     * @param[in] s   The model set (created if nullptr).
     * @param[in] m   The model to add (ownership transferred).
     *
     * @return The model set.
     */
    static FwdEegSphereModelSet* fwd_add_to_eeg_sphere_model_set(FwdEegSphereModelSet* s, FwdEegSphereModel::UPtr m);

    //=========================================================================================================
    /**
     * Choose and setup the default EEG sphere model
     * Refactored from: fwd_eeg_sphere_models.c
     *
     * @param[in] s            The model set to which loaded models are added.
     *
     * @return The model set with the default model (s + default models).
     */
    static FwdEegSphereModelSet* fwd_add_default_eeg_sphere_model(FwdEegSphereModelSet* s);

    //=========================================================================================================
    /**
     * Load all models available in the specified file
     * Refactored from: fwd_eeg_sphere_models.c
     *
     * @param[in] p_sFileName    file name to load models from.
     * @param[in] now            The model set to which loaded models are added.
     *
     * @return The loaded model set (now + loaded models).
     */
    static FwdEegSphereModelSet* fwd_load_eeg_sphere_models(const QString& p_sFileName, FwdEegSphereModelSet* now);

    //=========================================================================================================
    /**
     * Find a model with a given name and return a duplicate
     * Refactored from: fwd_eeg_sphere_models.c
     *
     * @param[in] p_sName    Name of the model to find.
     *
     * @return A duplicat of the found model.
     */
    FwdEegSphereModel* fwd_select_eeg_sphere_model(const QString& p_sName);

    //=========================================================================================================
    /**
     * List the properties of available models via qInfo.
     */
    void fwd_list_eeg_sphere_models();

    /** Number of models in this set. */
    int nmodel() const
    {
        return models.size();
    }

public:
    std::vector<FwdEegSphereModel::UPtr> models;     /**< Set of EEG sphere model definitions. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE FWDLIB

#endif // FWD_EEG_SPHERE_MODEL_SET_H
