//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     compute_fwd_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Parameter object holding the CLI/script-level configuration consumed by ComputeFwd.
 *
 * ComputeFwdSettings is the in-memory image of the command-line
 * arguments accepted by @c mne_forward_solution — paths to the
 * source-space, BEM or sphere model, measurement FIFF, MRI/head
 * transform and coil-definition files; sensor selection (MEG-only,
 * EEG-only, both); coil-integration accuracy (point / normal /
 * accurate); whether to include the magnetic-dipole approximation; and
 * the destination file for the resulting forward solution. Centralising
 * these fields keeps ComputeFwd's constructor signature short and lets
 * GUI front-ends (@c mne_forward_gui) and batch scripts share the same
 * validation logic via @ref checkIntegrity.
 */

#ifndef COMPUTE_FWD_SETTINGS_H
#define COMPUTE_FWD_SETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fwd_global.h"
#include <fiff/fiff_constants.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_coord_trans.h>
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QSharedPointer>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FWDLIB
//=============================================================================================================

namespace FWDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Holds the full parameter set for a single forward-solution computation.
 *
 * @brief Settings struct — source-space / BEM / sphere paths, sensor selection, coordinate transforms and accuracy level consumed by ComputeFwd.
 */
class FWDSHARED_EXPORT ComputeFwdSettings
{
public:
    //=========================================================================================================
    /**
     * Default Constructor
     */
    explicit ComputeFwdSettings();

    //=========================================================================================================
    /**
     * Destructs the Compute Forward Settings
     */
    virtual ~ComputeFwdSettings();

    //=========================================================================================================
    /**
     * Check whether Compute Forward Settings are correctly set.
     */
    void checkIntegrity();

public:
    QString srcname;            /**< Source space. */
    QString measname;           /**< Measurement file. */
    QString mriname;            /**< MRI file for head <-> MRI transformation. */
    QString transname;          /**< head2mri transformation file. */
    bool mri_head_ident;        /**< Are the head and MRI coordinates the same?. */
    QString bemname;            /**< BEM model file. */
    QString solname;            /**< Solution file. */
    QString mindistoutname;     /**< Output file for omitted source space points. */
    bool filter_spaces;         /**< Filter the source space points. */
    Eigen::Vector3f r0;         /**< Sphere model origin . */
    bool accurate;              /**< Use accurate calculations. */
    bool fixed_ori;             /**< Fixed-orientation dipoles?. */
    bool include_meg;
    bool include_eeg;
    bool compute_grad;
    QString command;            /**< Saves the recognized command line for future use. */
    float mindist;              /**< Minimum allowed distance of the sources from the inner skull surface. */
    int coord_frame;            /**< Can be changed with the --mricoord option. */
    bool do_all;
    QStringList labels;         /**< Compute the solution only for these labels. */
    int nlabel;
    int ncluster;               /**< Number of sources desired in clustered solution. */

    QString eeg_model_file;     /**< File of EEG sphere model specifications. */
    QString eeg_model_name;     /**< Name of the EEG model to use. */
    float eeg_sphere_rad;   	/**< Scalp radius to use in EEG sphere model. */
    bool scale_eeg_pos;     	/**< Scale the electrode locations to scalp in the sphere model. */
    bool use_equiv_eeg;      	/**< Use the equivalent source approach for the EEG sphere model. */
    bool use_threads;        	/**< Parallelize?. */

    QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo;    /**< The FiffInfo file from the measurement.*/
    FIFFLIB::FiffCoordTrans meg_head_t;         /**< The meg <-> head transformation.*/

private:
    void initMembers();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} //NAMESPACE

#endif // COMPUTE_FWD_SETTINGS_H
