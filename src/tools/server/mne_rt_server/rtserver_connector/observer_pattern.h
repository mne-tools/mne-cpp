//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     observer_pattern.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2017
 * @brief    Classical observer pattern (@c Subject and @c IObserver) behind the real-time command dispatch.
 *
 * @ref RTSERVER::CommandParser is the @ref RTSERVER::Subject that announces every parsed command, and each
 * @ref RTSERVER::CommandManager is an @ref RTSERVER::IObserver that executes the commands it knows. The pattern
 * needs no QObject, so the subject and observers can live on any thread; notifications are synchronous.
 */

#ifndef RTSERVER_OBSERVER_PATTERN_H
#define RTSERVER_OBSERVER_PATTERN_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstddef>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

class Subject;

//=============================================================================================================
/**
 * @brief Receives the notifications of the subjects it is attached to.
 *
 * @snippet ex_com/main.cpp observer_pattern_usage
 */
class IObserver
{
public:
    virtual ~IObserver() = default;

    //=========================================================================================================
    /**
     * Called by @ref Subject::notify.
     *
     * @param[in] subject    The notifying subject; observers read the state they need from it.
     */
    virtual void update(Subject* subject) = 0;
};

//=============================================================================================================
/**
 * @brief Notifies its attached observers, in the order they were attached.
 *
 * The subject does not own its observers; detach an observer before it is destroyed.
 *
 * @snippet ex_com/main.cpp observer_pattern_usage
 */
class Subject
{
public:
    virtual ~Subject() = default;

    //=========================================================================================================
    /**
     * Attaches an observer. Attaching an observer twice or attaching a null pointer has no effect.
     *
     * @param[in] observer   Observer to notify from now on.
     */
    void attach(IObserver* observer)
    {
        if (observer != nullptr && std::ranges::find(m_observers, observer) == m_observers.end()) {
            m_observers.push_back(observer);
        }
    }

    //=========================================================================================================
    /**
     * Detaches an observer. Detaching an observer that is not attached has no effect.
     *
     * @param[in] observer   Observer to stop notifying.
     */
    void detach(IObserver* observer)
    {
        std::erase(m_observers, observer);
    }

    //=========================================================================================================
    /**
     * Calls @ref IObserver::update of every attached observer, unless notifications are disabled. Observers may
     * attach or detach observers while they are notified; the change applies from the next notification on.
     */
    void notify()
    {
        if (!m_notifyEnabled) {
            return;
        }
        std::vector<IObserver*> observers; // a copy: assign() rather than the copy constructor, which trips GCC 14's -Warray-bounds at -O3
        observers.assign(m_observers.begin(), m_observers.end());
        for (IObserver* observer : observers) {
            observer->update(this);
        }
    }

    //=========================================================================================================
    /**
     * Suspends or resumes notifications, e.g. while a batch of changes is applied.
     *
     * @param[in] enabled    False makes @ref notify a no-op until notifications are enabled again.
     */
    void set_notify_enabled(bool enabled)
    {
        m_notifyEnabled = enabled;
    }

    //=========================================================================================================
    /**
     * @return True if @ref notify reaches the observers.
     */
    bool notify_enabled() const
    {
        return m_notifyEnabled;
    }

    //=========================================================================================================
    /**
     * @return The attached observers, in the order they were attached.
     */
    const std::vector<IObserver*>& observers() const
    {
        return m_observers;
    }

protected:
    Subject() = default;

private:
    std::vector<IObserver*> m_observers; /**< Attached observers, not owned. */
    bool m_notifyEnabled = true;         /**< False while notifications are suspended. */
};

} // namespace RTSERVER

#endif // RTSERVER_OBSERVER_PATTERN_H
