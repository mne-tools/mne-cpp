//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file observerpattern.h
 * @since 2022
 * @date  March 2026
 * @brief Classical GoF observer pattern (@c Subject + @c IObserver) used by MNE-CPP's non-QObject model classes.
 *
 * Qt's @c QObject signal/slot machinery is the preferred change
 * notification mechanism, but a number of model classes in
 * FIFFLIB, FSLIB and INVERSELIB are deliberately kept free of
 * the moc dependency so they remain header-only or usable from
 * static-library and WebAssembly builds. Those classes derive
 * from @ref UTILSLIB::Subject to publish change notifications
 * to any number of @ref UTILSLIB::IObserver listeners without
 * dragging the @c QObject machinery in.
 *
 * The static @c notifyEnabled flag exists so callers performing
 * bulk updates (e.g. fitting a forward solution) can suspend
 * notifications until the batch completes and prevent a storm
 * of redundant repaints in connected views.
 */

#ifndef OBSERVERPATTERN_H
#define OBSERVERPATTERN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"
#include <set>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSet>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Subject;

//=============================================================================================================
/**
 * DECLARE INTERFACE OBSERVER
 *
 * @brief The IObserver interface provides the base class of every observer of the observer design pattern.
 */
class UTILSSHARED_EXPORT IObserver
{
public:
    typedef QSharedPointer<IObserver> SPtr;             /**< Shared pointer type for IObserver. */
    typedef QSharedPointer<const IObserver> ConstSPtr;  /**< Const shared pointer type for IObserver. */

    //=========================================================================================================
    /**
     * Destroys the IObserver.
     */
    virtual ~IObserver() {};

    //=========================================================================================================
    /**
     * Updates the IObserver.
     *
     * @param[in] pSubject pointer to the subject where observer is attached to.
     */
    virtual void update(Subject* pSubject) = 0;
};

//=========================================================================================================
/**
 * DECLARE BASE CLASS SUBJECT
 *
 * @brief The Subject class provides the base class of every subject of the observer design pattern.
 */
class UTILSSHARED_EXPORT Subject
{
public:
    typedef QSharedPointer<Subject> SPtr;               /**< Shared pointer type for Subject. */
    typedef QSharedPointer<const Subject> ConstSPtr;    /**< Const shared pointer type for Subject. */

    typedef QSet<IObserver*>    t_Observers;            /**< Defines a new IObserver set type. */

    //=========================================================================================================
    /**
     * Destroys the Subject.
     */
    virtual ~Subject();

    //=========================================================================================================
    /**
     * Attaches an observer to the subject.
     *
     * @param[in] pObserver pointer to the observer which should be attached to the subject.
     */
    void attach(IObserver* pObserver);

    //=========================================================================================================
    /**
     * Detaches an observer of the subject.
     *
     * @param[in] pObserver pointer to the observer which should be detached of the subject.
     */
    void detach(IObserver* pObserver);

    //=========================================================================================================
    /**
     * Notifies all attached servers by calling there update method. Is used when subject has updates to provide.
     * This method is enabled when nothifiedEnabled is true.
     */
    void notify();

    //=========================================================================================================
    /**
     * Holds the status whether notification is enabled.
     * Used to block notify() and prevent re-entrant notification loops.
     */
    static bool notifyEnabled;

    //=========================================================================================================
    /**
     * Returns attached observers.

     * @return attached observers.
     */
    inline t_Observers& observers();

//    //=========================================================================================================
//    /**
//     * Returns attached observers.

//     * @return attached observers.
//     */
//    inline std::set<IObserver*>& observers();

    //=========================================================================================================
    /**
     * Returns number of attached observers.
     *
     * @return the number of attached observers.
     */
    int observerNumDebug(){return m_Observers.size();};

protected:
    //=========================================================================================================
    /**
     * Constructs a Subject.
     */
    Subject() {};

private:
    t_Observers                 m_Observers;    /**< Holds the attached observers.*/
//    std::set<IObserver*>        m_Observers;    /**< Holds the attached observers.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Subject::t_Observers& Subject::observers()
{
    return m_Observers;
}

//inline std::set<IObserver*>& Subject::observers()
//{
//    return m_Observers;
//}

} // Namespace

#endif // OBSERVERPATTERN_H
