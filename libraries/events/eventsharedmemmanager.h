//=============================================================================================================
/**
 * @file     eventsharedmemmanager.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.8
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief     EventSharedMemManager declaration.
 *
 */

#ifndef EVENTSHAREDMEMMANAGER_EVENTS_H
#define EVENTSHAREDMEMMANAGER_EVENTS_H


//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedMemory>

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "event.h"
#include "eventgroup.h"

namespace EVENTSLIB {

//=============================================================================================================
// EVENTSLIB FORWARD DECLARATIONS
//=============================================================================================================

enum SharedMemoryMode { READ, WRITE, READWRITE };
class EventManager;
}


namespace EVENTSINTERNAL {

/**
 * @brief The EventUpdate class
 */
class EventUpdate
{
public:
    /**
     * @brief The type enum
     */
    enum type{ NULL_EVENT, NEW_EVENT, DELETE_EVENT};


//=============================================================================================================
// INCLUDES
//=============================================================================================================
    /**
     * typeString
     */
    const char* typeString[3] = {"Null Event", "New Event", "Delete Event"};

    /**
     * @brief EventUpdate
     */
    EventUpdate();

    /**
     * @brief EventUpdate
     * @param sample
     * @param creator
     * @param t
     */
    EventUpdate(int sample, int creator,type t);

    /**
     * @brief getCreationTime
     * @return
     */
    long long getCreationTime() const;

    /**
     * @brief getSample
     * @return
     */
    int getSample() const;

    /**
     * @brief getCreatorId
     * @return
     */
    int getCreatorId() const;

    /**
     * @brief getType
     * @return
     */
    EventUpdate::type getType() const;

    /**
     * @brief setType
     * @param t
     */
    void setType(type t);

    /**
     * @brief eventTypeToText
     * @return
     */
    std::string eventTypeToText();

protected:
    int                 m_EventSample;  /**< */
    int                 m_CreatorId;    /**< */
    long long           m_CreationTime; /**< */
    EventUpdate::type   m_TypeOfUpdate; /**< */
};

/**
 * @brief The EventSharedMemManager class
 */
class EventSharedMemManager
{
public:
    /**
     * @brief EventSharedMemManager
     * @param parent
     */
    explicit EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);

    /**
     *
     */
    ~EventSharedMemManager();
    /**
     * @brief init
     * @param mode
     */
    void init(EVENTSLIB::SharedMemoryMode mode);

    /**
     * @brief isInit
     * @return
     */
    bool isInit() const;

    /**
     * @brief stop
     */
    void stop();

    /**
     * @brief addEvent
     * @param sample
     */
    void addEvent(int sample);
    /**
     * @brief deleteEvent
     * @param sample
     */
    void deleteEvent(int sample);

    /**
     * @brief getTimeNow
     * @return
     */
    static long long getTimeNow();

    /**
     * @brief processEvent
     * @param ne
     */
    void processEvent(const EventUpdate& ne);

private:
    /**
     * @brief attachToSharedSegment
     * @param mode
     */
    void attachToSharedSegment(QSharedMemory::AccessMode mode);

    /**
     * @brief createSharedSegment
     * @param bufferSize
     * @param mode
     * @return
     */
    bool createSharedSegment(int bufferSize, QSharedMemory::AccessMode mode);

    /**
     * @brief launchSharedMemoryWatcherThread
     */
    void launchSharedMemoryWatcherThread();

    /**
     * @brief attachToOrCreateSharedSegment
     * @param mode
     */
    void attachToOrCreateSharedSegment(QSharedMemory::AccessMode mode);

    /**
     * @brief stopSharedMemoryWatcherThread
     */
    void stopSharedMemoryWatcherThread();

    /**
     * @brief detachFromSharedMemory
     */
    void detachFromSharedMemory();

    /**
     * @brief generateId
     * @return
     */
    inline static int generateId();

    /**
     * @brief processNewEvent
     * @param n
     */
    void processNewEvent(const EventUpdate& n);

    /**
     * @brief processDeleteEvent
     * @param n
     */
    void processDeleteEvent(const EventUpdate& n);

    /**
     * @brief printLocalBuffer
     */
    void printLocalBuffer();

    /**
     * @brief copyNewUpdateToSharedMemory
     * @param newUpdate
     */
    void copyNewUpdateToSharedMemory(EventUpdate& newUpdate);

    /**
     * @brief initializeSharedMemory
     */
    void initializeSharedMemory();

    /**
     * @brief copySharedMemoryToLocalBuffer
     */
    void copySharedMemoryToLocalBuffer();

    /**
     * @brief processLocalBuffer
     */
    void processLocalBuffer();

    /**
     * @brief bufferWatcher
     */
    void bufferWatcher();

    /**
     * @brief createGroupIfNeeded
     */
    void createGroupIfNeeded();

    static int                      m_iLastUpdateIndex;             /**<  */
    EVENTSLIB::EventManager*        m_pEventManager;                /**<  */
    QSharedMemory                   m_SharedMemory;                 /**<  */
    std::atomic_bool                m_IsInit;                       /**<  */
    std::string                     m_sGroupName;                   /**<  */
    bool                            m_bGroupCreated;                /**<  */
    idNum                           m_GroupId;                      /**<  */
    int                             m_SharedMemorySize;             /**<  */
    int                             m_fTimerCheckBuffer;            /**<  */
    std::thread                     m_BufferWatcherThread;          /**<  */
    std::atomic_bool                m_BufferWatcherThreadRunning;   /**<  */
    std::atomic_bool                m_WritingToSharedMemory;        /**<  */
    long long                       m_lastCheckTime;                /**<  */
    EventUpdate*                    m_LocalBuffer;                  /**<  */
    EventUpdate*                    m_SharedBuffer;                 /**<  */
    int                             m_Id;                           /**<  */
    enum EVENTSLIB::SharedMemoryMode m_Mode;                        /**<  */
};

/**
 * @brief EventSharedMemManager::generateId
 * @return
 */
inline int EventSharedMemManager::generateId()
{
    auto t = static_cast<int>(EventSharedMemManager::getTimeNow());
    return abs(t);
}

} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H

