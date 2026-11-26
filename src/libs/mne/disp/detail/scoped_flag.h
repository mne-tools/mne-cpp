//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scoped_flag.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A flag that is set for a scope, shared by the mne_disp sources; not installed.
 */

#ifndef DISPLIB_DETAIL_SCOPED_FLAG_H
#define DISPLIB_DETAIL_SCOPED_FLAG_H

namespace mne::disp
{
namespace detail
{

/**
 * Sets a flag while it lives and restores the previous value afterwards, e.g. to apply settings without signals.
 *
 * @since 3.0.0
 */
class ScopedFlag
{
public:
    /**
     * Sets the flag.
     *
     * @param[in, out] flag   Flag, restored when the object goes out of scope.
     */
    explicit ScopedFlag(bool& flag)
    : m_flag(flag)
    , m_previous(flag)
    {
        m_flag = true;
    }

    ~ScopedFlag()
    {
        m_flag = m_previous;
    }

    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;

private:
    bool& m_flag;
    bool m_previous;
};

} // namespace detail
} // namespace mne::disp

#endif // DISPLIB_DETAIL_SCOPED_FLAG_H
