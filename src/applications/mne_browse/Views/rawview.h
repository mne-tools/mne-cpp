//=============================================================================================================
/**
 * @file     rawview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the RawView class.
 *
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
