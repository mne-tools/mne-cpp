//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Umbrella include header that exposes the full mne_fiff public surface in a single include.
 *
 * mne_fiff reads and writes the Neuromag FIFF format: the tag stream and directory, measurement info and
 * channels, raw data, evoked responses and epochs, covariances, projections, coordinate transformations,
 * digitization, events and annotations.
 */

#ifndef FIFF_H
#define FIFF_H

//=============================================================================================================
// FIFF LIBRARY HEADERS
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_explain.h"
#include "fiff_file.h"
#include "fiff_types.h"
#include "fiff_time.h"
#include "fiff_data_ref.h"
#include "fiff_id.h"
#include "fiff_constants.h"
#include "fiff_tag.h"
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_ch_name.h"
#include "fiff_proj.h"
#include "fiff_named_matrix.h"
#include "fiff_ctf_comp.h"
#include "fiff_info.h"
#include "fiff_raw_data.h"
#include "fiff_dir_entry.h"
#include "fiff_raw_dir.h"
#include "fiff_dig_point.h"
#include "fiff_ch_pos.h"
#include "fiff_cov.h"
#include "fiff_stream.h"
#include "fiff_info_base.h"
#include "fiff_evoked.h"
#include "fiff_epochs.h"
#include "fiff_evoked_set.h"
#include "fiff_events.h"
#include "fiff_io.h"
#include "fiff_dig_montage.h"
#include "fiff_dig_point_set.h"
#include "fiff_dir_node.h"
#include "fiff_sparse_matrix.h"
#include "fiff_digitizer_data.h"
#include "fiff_coord_trans_set.h"
#include "fiff_file_sharer.h"
#include "fiff_annotations.h"
#include "fiff_annotation_event_utils.h"
#include "fiff_byte_swap.h"

#endif // FIFF_H
