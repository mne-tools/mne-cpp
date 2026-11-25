//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     layout_reset.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Synchronous teardown of the layouts that views rebuild on every redraw; not installed.
 */

#ifndef DISPLIB_DETAIL_LAYOUT_RESET_H
#define DISPLIB_DETAIL_LAYOUT_RESET_H

#include <QLayout>
#include <QWidget>

#include <memory>

namespace mne::disp
{
namespace detail
{

/** Removes every item from @p pLayout and deletes it together with the widget it manages. */
inline void clearLayout(QLayout* pLayout)
{
    while (std::unique_ptr<QLayoutItem> pItem{pLayout->takeAt(0)}) {
        std::unique_ptr<QWidget> pWidget{pItem->widget()};
    }
}

/** Deletes the layout of @p pWidget and the widgets it manages, so that a new layout can be installed. */
inline void resetLayout(QWidget* pWidget)
{
    // The widget accepts a new layout only once the old one is gone, and deleting a layout leaves its widgets behind
    if (std::unique_ptr<QLayout> pLayout{pWidget->layout()}) {
        clearLayout(pLayout.get());
    }
}

} // namespace detail
} // namespace mne::disp

#endif // DISPLIB_DETAIL_LAYOUT_RESET_H
