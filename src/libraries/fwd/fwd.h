//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fwd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     December 2012
 * @brief    Umbrella include header that exposes the full mne_fwd public surface in a single include.
 *
 * Pulls in forward-solution computation (@ref mne::ComputeFwd and its settings), the BEM and
 * spherical head models, the coil definitions and field mapping, together with
 * @ref mne::MNEForwardSolution, the solution they produce and read back from -fwd.fif files.
 */

#ifndef FWD_H
#define FWD_H

//=============================================================================================================
// FWD LIBRARY HEADERS
//=============================================================================================================

#include "fwd_global.h"
#include "fwd_types.h"
#include "fwd_coil.h"
#include "fwd_coil_set.h"
#include "fwd_comp_data.h"
#include "fwd_bem_model.h"
#include "fwd_bem_solution.h"
#include "fwd_eeg_sphere_layer.h"
#include "fwd_eeg_sphere_model.h"
#include "fwd_eeg_sphere_model_set.h"
#include "fwd_field_map.h"
#include "fwd_thread_arg.h"
#include "compute_fwd/compute_fwd_settings.h"
#include "compute_fwd/compute_fwd.h"

#include <mne/mne_forward_solution.h>

#endif // FWD_H
