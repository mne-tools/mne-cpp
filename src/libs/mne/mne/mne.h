//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Umbrella include header that exposes the full mne_mne public surface in a single include.
 *
 * mne_mne holds the MNE data model shared by forward and inverse modelling: source spaces and hemispheres,
 * BEM surfaces, forward solutions, inverse operators, measurement and covariance data, epochs, events,
 * projections, morph maps and the viewer surface types.
 */

#ifndef MNE_H
#define MNE_H

//=============================================================================================================
// MNE LIBRARY HEADERS
//=============================================================================================================

#include "mne_global.h"
#include "mne_source_spaces.h"
#include "mne_hemisphere.h"
#include "mne_epoch_data.h"
#include "mne_epoch_data_list.h"
#include "mne_cluster_info.h"
#include "mne_bem.h"
#include "mne_bem_surface.h"
#include "mne_project_to_surface.h"
#include "mne_icp.h"
#include "mne_cov_matrix.h"
#include "mne_ctf_comp_data.h"
#include "mne_ctf_comp_data_set.h"
#include "mne_deriv.h"
#include "mne_deriv_set.h"
#include "mne_mne_data.h"
#include "mne_named_matrix.h"
#include "mne_named_vector.h"
#include "mne_sparse_named_matrix.h"
#include "mne_layout_port.h"
#include "mne_layout.h"
#include "mne_ch_selection.h"
#include "mne_event.h"
#include "mne_event_list.h"
#include "mne_filter_def.h"
#include "mne_proj_item.h"
#include "mne_proj_op.h"
#include "mne_raw_buf_def.h"
#include "mne_raw_data.h"
#include "mne_raw_info.h"
#include "mne_sss_data.h"
#include "mne_triangle.h"
#include "mne_types.h"
#include "mne_vol_geom.h"
#include "mne_nearest.h"
#include "mne_patch_info.h"
#include "mne_source_space.h"
#include "mne_surface.h"
#include "mne_surface_or_volume.h"
#include "filter_thread_arg.h"
#include "mne_msh_display_surface.h"
#include "mne_msh_display_surface_set.h"
#include "mne_msh_picked.h"
#include "mne_morph_map.h"
#include "mne_msh_color_scale_def.h"
#include "mne_proj_data.h"
#include "mne_msh_light.h"
#include "mne_msh_light_set.h"
#include "mne_surface_patch.h"
#include "mne_msh_eyes.h"
#include "mne_mgh_tag.h"
#include "mne_mgh_tag_group.h"
#include "mne_process_description.h"
#include "mne_description_parser.h"
#include "mne_forward_solution.h"
#include "mne_inverse_operator.h"
#include "mne_cortical_map.h"
#include "mne_meas_data.h"
#include "mne_meas_data_set.h"

#endif // MNE_H
