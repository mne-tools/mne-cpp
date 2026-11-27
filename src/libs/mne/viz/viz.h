//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viz.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Umbrella header of mne_viz: figures, views, GPU frames, forms and view models without a GUI toolkit.
 */

#ifndef VIZ_VIZ_H
#define VIZ_VIZ_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "figures/average_layout.h"
#include "figures/average_tile.h"
#include "figures/bar_chart.h"
#include "figures/butterfly_plot.h"
#include "figures/canvas.h"
#include "figures/colormap.h"
#include "figures/display_list.h"
#include "figures/filter_response_plot.h"
#include "figures/font_face.h"
#include "figures/font_library.h"
#include "figures/font_source.h"
#include "figures/font_text_metrics.h"
#include "figures/geometry.h"
#include "figures/image_plot.h"
#include "figures/line_chart.h"
#include "figures/raster_renderer.h"
#include "figures/sensor_layout.h"
#include "figures/spline_plot.h"
#include "figures/style.h"
#include "figures/svg_renderer.h"
#include "figures/system_fonts.h"
#include "figures/text_metrics.h"
#include "figures/theme.h"
#include "figures/ticks.h"
#include "figures/vector_plot.h"
#include "forms/settings_form.h"
#include "gpu/brain_frame.h"
#include "gpu/gpu_frame.h"
#include "gpu/software_renderer.h"
#include "gpu/trace_frame.h"
#include "models/bids_tree.h"
#include "models/channel_table.h"
#include "models/evoked_conditions.h"
#include "models/spectrum_table.h"
#include "views/brain_view_interaction.h"
#include "views/brain_view_overlay.h"
#include "views/camera.h"
#include "views/input_event.h"
#include "views/pane_layout.h"
#include "views/ray_picking.h"
#include "views/sweep_buffer.h"
#include "views/trace_buffer.h"
#include "views/trace_geometry.h"
#include "views/trace_markers.h"
#include "views/trace_source.h"
#include "views/trace_view.h"
#include "views/view_animator.h"

#endif // VIZ_VIZ_H
