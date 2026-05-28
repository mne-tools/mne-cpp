//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwd_eeg_sphere_model_set.h
 * @since March 2017
 * @brief Named container of FwdEegSphereModel objects loaded from an @c mne_setup_eeg_sphere_model parameter file.
 *
 * MNE distributes a small text file (one model per line: @c name, radii,
 * conductivities) so users can pick a standard 4-shell head model —
 * @c "Default", Stok/Cuffin/Cohen variants, etc. — by name from the
 * command line. FwdEegSphereModelSet parses that file, builds the
 * corresponding FwdEegSphereModel objects (including the Berg-Scherg
 * fit) and exposes name-based lookup so the rest of the pipeline can
 * resolve a string like @c "Default" into a fully usable analytic head
 * model.
 *
 * Refactored from @c fwdEegSphereModelSetRec in MNE-C @c fwd_types.h.
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
 * Definitions for the EEG Sphere Model Set (replaces @c fwdEegSphereModelSet / @c fwdEegSphereModelSetRec from MNE-C @c fwd_types.h).
 *
 * @brief Name-indexed collection of FwdEegSphereModel objects parsed from an @c mne_setup_eeg_sphere_model parameter file so callers can resolve textual model names ("Default", "Stok", …) into ready-to-use analytic head models.
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

    FwdEegSphereModelSet(const FwdEegSphereModelSet&) = delete;
    FwdEegSphereModelSet& operator=(const FwdEegSphereModelSet&) = delete;
    FwdEegSphereModelSet(FwdEegSphereModelSet&&) = default;
    FwdEegSphereModelSet& operator=(FwdEegSphereModelSet&&) = default;

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
