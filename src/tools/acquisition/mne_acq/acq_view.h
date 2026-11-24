//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    mne_acq view: live traces of a source in a sweep display (ChannelDataView).
 */

#ifndef MNE_ACQ_VIEW_H
#define MNE_ACQ_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/acq/acq.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
/**
 * Shows the first stream of @p device as live traces until the window closes, the duration (0: none) has passed
 * or the source ends. Creates the QApplication itself, so the caller needs no Qt.
 *
 * @param[in] argc         Argument count of main(); QApplication keeps a reference to it.
 * @param[in] argv         Arguments of main().
 * @param[in] device       Opened source; started and closed here.
 * @param[in] window       Visible time window in seconds.
 * @param[in] duration     Seconds to run, 0 until the window is closed.
 * @param[in] screenshot   PNG written of the window when the run ends; empty for none.
 *
 * @return 0, or 2 when the source or the screenshot fails.
 */
int runAcqView(int& argc, char** argv, mne::AcqDevice& device, double window, double duration, const std::string& screenshot);

#endif // MNE_ACQ_VIEW_H
