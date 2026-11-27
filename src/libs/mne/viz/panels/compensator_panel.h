//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     compensator_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CompensatorPanel: choose the CTF software gradient compensation.
 */

#ifndef VIZ_COMPENSATOR_PANEL_H
#define VIZ_COMPENSATOR_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"
#include <mne/fiff/fiff_ctf_comp.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
 * @brief One check box per compensation grade of the data; at most one is on.
 *
 * The state of each grade is remembered across sessions under "<path>/CompensatorView/active".
 *
 * @snippet ex_scene/main.cpp panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT CompensatorPanel : public Panel
{
public:
    /** Called with the chosen grade (the FIFF kind of the compensator), 0 for none. */
    using SelectionCallback = std::function<void(int to)>;

    CompensatorPanel();
    ~CompensatorPanel() override;

    //=========================================================================================================
    /**
     * Shows the compensators of the data; a remembered grade is chosen again and reported.
     *
     * @param[in] comps   Compensators of the measurement info.
     */
    void set_compensators(const std::vector<mne::FiffCtfComp>& comps);

    /** @return The compensators shown. */
    const std::vector<mne::FiffCtfComp>& compensators() const
    {
        return m_comps;
    }

    /** @return The chosen grade, 0 for none. */
    int selected() const
    {
        return m_selected;
    }

    //=========================================================================================================
    /**
     * Chooses a grade, as a click on its check box does.
     *
     * @param[in] to   FIFF kind of a shown compensator, 0 for none.
     */
    void select(int to);

    /** @param[in] callback   Called whenever the chosen grade changes. */
    void set_selection_callback(SelectionCallback callback)
    {
        m_callback = std::move(callback);
    }

    void load(const SettingsStore& store, const std::string& path) override;
    void save(SettingsStore& store, const std::string& path) const override;

    /**
     * @param[in] kind   FIFF kind of a compensator.
     *
     * @return The form key of the check box of the grade.
     */
    static std::string field_key(int kind);

private:
    void toggled(int kind, bool on);

    std::vector<mne::FiffCtfComp> m_comps;
    std::map<int, bool> m_active; /**< Remembered state per grade. */
    int m_selected = 0;
    bool m_updating = false;
    int m_listener = 0;
    SelectionCallback m_callback;
};

} // namespace mne::viz

#endif // VIZ_COMPENSATOR_PANEL_H
