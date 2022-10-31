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
 * @brief     FwdEegSphereModelSet class declaration.
 *
 */

#ifndef FWDEEGSPHEREMODELSET_H
#define FWDEEGSPHEREMODELSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_eeg_sphere_model.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QString>

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
 * @brief Holds a set of Electric Current Dipoles.
 */

class FWDSHARED_EXPORT FwdEegSphereModelSet
{

public:
    typedef QSharedPointer<FwdEegSphereModelSet> SPtr;            /**< Shared pointer type for FwdEegSphereModelSet. */
    typedef QSharedPointer<const FwdEegSphereModelSet> ConstSPtr; /**< Const shared pointer type for FwdEegSphereModelSet. */

    //=========================================================================================================
    /**
     * Constructs a Forward EEG Sphere Model Set object.
     */
    FwdEegSphereModelSet();

//    //=========================================================================================================
//    /**
//    * Copy constructor.
//    *
//    * @param[in] p_FwdEegSphereModelSet     Forward EEG Sphere Model Set which should be copied
//    */
//    FwdEegSphereModelSet(const FwdEegSphereModelSet &p_FwdEegSphereModelSet);

    //=========================================================================================================
    /**
     * Destroys the Forward EEG Sphere Model Set description
     * Refactored: fwd_free_eeg_sphere_model_set
     */
    ~FwdEegSphereModelSet();

//    //ToDo move to destructor
//    static void fwd_free_eeg_sphere_model_set(FwdEegSphereModelSet* s);

//    static FwdEegSphereModelSet* fwd_new_eeg_sphere_model_set();

    /*
     * ToDo make non static member
     * Add a new model to a set.
     * The model should not be deallocated after this since it is attached to the set
     */

    static FwdEegSphereModelSet* fwd_add_to_eeg_sphere_model_set(FwdEegSphereModelSet* s, FwdEegSphereModel* m);

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
     * List the properties of available models
     * Refactored from: dipole_fit_setup.c
     *
     * @param[in] f      std output stream.
     */
    void fwd_list_eeg_sphere_models(FILE *f);

//    //=========================================================================================================
//    /**
//    * Appends an orward EEG Sphere Model to the set
//    */
//    void addFwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel);

//    //=========================================================================================================
//    /**
//    * Returns the number of stored FwdEegSphereModels
//    *
//    * @return number of stored FwdEegSphereModels
//    */
//    inline qint32 size() const;

//    //=========================================================================================================
//    /**
//    * Subscript operator [] to access FwdEegSphereModel by index
//    *
//    * @param[in] idx    the FwdEegSphereModel index.
//    *
//    * @return FwdEegSphereModel related to the parameter index.
//    */
//    const FwdEegSphereModel& operator[] (qint32 idx) const;

//    //=========================================================================================================
//    /**
//    * Subscript operator [] to access FwdEegSphereModel by index
//    *
//    * @param[in] idx    the FwdEegSphereModel index.
//    *
//    * @return FwdEegSphereModel related to the parameter index.
//    */
//    FwdEegSphereModel& operator[] (qint32 idx);

//    //=========================================================================================================
//    /**
//    * Subscript operator << to add a new FwdEegSphereModel
//    *
//    * @param[in] p_FwdEegSphereModel      FwdEegSphereModel to be added
//    *
//    * @return FwdEegSphereModelSet
//    */
//    FwdEegSphereModelSet& operator<< (const FwdEegSphereModel& p_FwdEegSphereModel);

    int nmodel() const
    {
        return models.size();
    }

public:
//    QList<FwdEegSphereModel> m_qListModels;    /**< Set of EEG sphere model definitions. */

    QList<FwdEegSphereModel*> models;     /**< Set of EEG sphere model definitions. */

// ### OLD STRUCT ###
//    typedef struct {
//      fwdEegSphereModel *models;  /* Set of EEG sphere model definitions */
//      int               nmodel;
//    } *fwdEegSphereModelSet,fwdEegSphereModelSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

//inline qint32 FwdEegSphereModelSet::size() const
//{
//    return m_qListModels.size();
//}
} // NAMESPACE FWDLIB

#endif // FWDEEGSPHEREMODELSET_H
