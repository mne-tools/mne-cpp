//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sphara_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpharaPanel switches SPHARA noise reduction and chooses its basis functions.
 */

#ifndef VIZ_SPHARA_PANEL_H
#define VIZ_SPHARA_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <string>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of SPHARA: whether it is active, the sensor system and the number of basis functions of its
 * one or two sensor groups.
 *
 * BabyMEG has an inner (up to 270) and an outer layer (105), VectorView magnetometers and gradiometers (102
 * each), EEG one group (256). The settings come from the plugin's defaults and are not stored.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SpharaPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(bool active)> activation_changed; /**< SPHARA was switched. */
        std::function<void(const std::string& system, int first, int second)>
            options_changed; /**< The system or a number of basis functions changed. */
    };

    //=========================================================================================================
    /**
     * Constructs the panel: inactive, BabyMEG with 270 and 105 basis functions.
     */
    SpharaPanel();

    ~SpharaPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callbacks   What to call on changes.
     */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * @return Whether SPHARA is active.
     */
    bool active() const;

    //=========================================================================================================
    /**
     * @return "BabyMEG", "VectorView" or "EEG".
     */
    std::string system() const;

    //=========================================================================================================
    /**
     * @return Basis functions of the first sensor group.
     */
    int first() const;

    //=========================================================================================================
    /**
     * @return Basis functions of the second sensor group; unused for EEG.
     */
    int second() const;

private:
    void update_system_fields();

    Callbacks m_callbacks;   /**< What to call on changes. */
    int m_listener = 0;      /**< Listener on the form. */
    bool m_updating = false; /**< Whether the panel changes fields itself. */
};

} // namespace mne::viz

#endif // VIZ_SPHARA_PANEL_H
