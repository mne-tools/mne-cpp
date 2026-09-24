//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rawview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  2.1.0
 * @brief    Declaration of the RawView class.
 */

#ifndef RAWVIEW_H
#define RAWVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/viewers/channeldataview.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * @brief Thin application-level wrapper around DISPLIB::ChannelDataView.
 *
 * RawView intentionally adds no browser-specific behavior yet. It exists as the stable seam where
 * mne_browse-specific adaptations can live later without pushing those changes into the generic DISPLIB
 * viewer stack too early.
 */
class RawView : public DISPLIB::ChannelDataView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs the raw browser view wrapper.
     *
     * @param[in] sSettingsPath  QSettings prefix for persistent GUI settings.
     * @param[in] parent         Parent widget.
     * @param[in] f              Window flags.
     */
    explicit RawView(const QString &sSettingsPath = QString(),
                     QWidget *parent = nullptr,
                     Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the raw browser view wrapper.
     */
    ~RawView() override;
};

} // namespace MNEBROWSE

#endif // RAWVIEW_H
