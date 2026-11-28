//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     minimum_norm_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    MinimumNormPanel chooses the inverse method, the trigger type and time point and the CMNE model.
 */

#ifndef VIZ_MINIMUM_NORM_PANEL_H
#define VIZ_MINIMUM_NORM_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of the minimum-norm source estimation: the method (MNE, dSPM, sLORETA or CMNE), the trigger
 * type and time point of the averages, and for CMNE the ONNX model checkpoint.
 *
 * A method given to the constructor is fixed. Only the checkpoint is stored, under "<path>/MinimumNormSettingsView/
 * modelCheckpoint".
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT MinimumNormPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(const std::string& method)> method_changed;                   /**< The method changed. */
        std::function<void(const std::string& type)> trigger_type_changed;               /**< The trigger type changed. */
        std::function<void(int milliseconds)> time_point_changed;                        /**< The time point changed. */
        std::function<void(const std::filesystem::path& path)> model_checkpoint_changed; /**< The CMNE checkpoint changed. */
    };

    //=========================================================================================================
    /**
     * @param[in] method   A fixed method, or empty to let the user choose.
     */
    explicit MinimumNormPanel(const std::string& method = {});

    ~MinimumNormPanel() override;

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
     * Adds trigger types the panel does not offer yet.
     *
     * @param[in] types   Trigger types.
     */
    void set_trigger_types(const std::vector<std::string>& types);

    //=========================================================================================================
    /**
     * @param[in] path   The CMNE checkpoint; reported if it changed.
     */
    void set_model_checkpoint(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * @return The CMNE checkpoint.
     */
    std::filesystem::path model_checkpoint() const;

    //=========================================================================================================
    /**
     * @return The method.
     */
    std::string method() const;

private:
    Callbacks m_callbacks;              /**< What to call on changes. */
    std::filesystem::path m_checkpoint; /**< The reported checkpoint. */
    int m_listener = 0;                 /**< Listener on the form. */
};

} // namespace mne::viz

#endif // VIZ_MINIMUM_NORM_PANEL_H
