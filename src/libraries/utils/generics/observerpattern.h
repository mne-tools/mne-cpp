//=============================================================================================================
/**
 * @file     observerpattern.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains declarations of the observer design pattern: Subject class and IObserver interface.
 *
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
     * This is used to block notify() to make the observer pattern thread safe. It's working like a mutex. The different is that data aren't stored. -> it's okay when values are queued in their own buffer.
     */
    static bool notifyEnabled; //ToDo move this to obeservers + make it thread safe

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
     * ToDo only for debug purpose -> could be removed
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
