//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     projectors_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ProjectorsPanel: switch the SSP projectors of the data on and off.
 */

#ifndef VIZ_PROJECTORS_PANEL_H
#define VIZ_PROJECTORS_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"
#include <mne/fiff/fiff_proj.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief One check box per projector and one that switches all of them.
 *
 * The state of each projector is remembered by its description across sessions under
 * "<path>/ProjectorsView/active".
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ProjectorsPanel : public Panel
{
public:
    /** Called with the projectors and their new active flags. */
    using SelectionCallback = std::function<void(const std::vector<mne::FiffProj>& projs)>;

    /** Form key of the check box that switches all projectors. */
    static constexpr const char* kAllKey = "proj_all";

    ProjectorsPanel();
    ~ProjectorsPanel() override;

    //=========================================================================================================
    /**
     * Shows the projectors of the data with their remembered states, and reports them.
     *
     * @param[in] projs   Projectors of the measurement info.
     */
    void set_projectors(const std::vector<mne::FiffProj>& projs);

    /** @return The projectors with their current active flags. */
    const std::vector<mne::FiffProj>& projectors() const
    {
        return m_projs;
    }

    //=========================================================================================================
    /**
     * Switches a projector, as a click on its check box does.
     *
     * @param[in] index    Index into projectors().
     * @param[in] active   New state.
     */
    void set_active(std::size_t index, bool active);

    //=========================================================================================================
    /**
     * Switches all projectors.
     *
     * @param[in] active   New state.
     */
    void set_all_active(bool active);

    /** @param[in] callback   Called whenever a projector is switched. */
    void set_selection_callback(SelectionCallback callback)
    {
        m_callback = std::move(callback);
    }

    void load(const SettingsStore& store, const std::string& path) override;
    void save(SettingsStore& store, const std::string& path) const override;

    /**
     * @param[in] index   Index into projectors().
     *
     * @return The form key of the check box of the projector.
     */
    static std::string field_key(std::size_t index);

private:
    void apply(std::size_t index, bool active);
    void changed();

    std::vector<mne::FiffProj> m_projs;
    std::map<std::string, bool> m_active; /**< Remembered state per description. */
    bool m_updating = false;
    int m_listener = 0;
    SelectionCallback m_callback;
};

} // namespace mne::viz

#endif // VIZ_PROJECTORS_PANEL_H
