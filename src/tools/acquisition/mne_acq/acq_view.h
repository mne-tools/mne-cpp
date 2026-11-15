//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    mne_acq view: live traces of a source in a sweep display (ChannelDataView).
 */

#ifndef MNE_ACQ_VIEW_H
#define MNE_ACQ_VIEW_H

#include <acq/acq.h>

#include <QString>

//=============================================================================================================
/**
 * Shows the first stream of @p device as live traces until the window closes, the duration (0: none) has passed
 * or the source ends. Needs a running QApplication.
 *
 * @param[in] device       Opened source; started and closed here.
 * @param[in] window       Visible time window in seconds.
 * @param[in] duration     Seconds to run, 0 until the window is closed.
 * @param[in] screenshot   PNG written of the window when the run ends; empty for none.
 *
 * @return 0, or 2 when the source or the screenshot fails.
 */
int runAcqView(mne::AcqDevice& device, double window, double duration, const QString& screenshot);

#endif // MNE_ACQ_VIEW_H
