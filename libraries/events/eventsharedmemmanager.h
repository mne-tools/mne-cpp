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
 *     EventSharedMemManager declaration.
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
// MNECPP INCLUDES
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
 * The type enum specifies what kind of event happened.
 */
enum EventUpdateType{ NULL_EVENT, NEW_EVENT, DELETE_EVENT};

/**
 * typeString Allows for a coherent
 */
const std::string EventUpdateTypeString[3] = {"Null Event", "New Event", "Delete Event"};

/**
 * The EventUpdate class serves as a minimal class definition to store updates in the shared memory buffer.
 * The updates are events that happened and need to be followed by a shared memory reader object to process them
 * further.
 */
struct EventUpdate
{
public:

    /**
     * EventUpdate
     */
    EventUpdate();

    /**
     * EventUpdate
     * @param[in] sample
     * @param[in] creator
     * @param[in] t
     */
    EventUpdate(int sample, int creator, enum EventUpdateType t);

    /**
     * getCreationTime
     * @return
     */
    long long getCreationTime() const;

    /**
     * getSample
     * @return
     */
    int getSample() const;

    /**
     * getCreatorId
     * @return
     */
    int getCreatorId() const;

    /**
     * getType
     * @return
     */
    enum EventUpdateType getType() const;

    /**
     * setType
     * @param[in] t
     */
    void setType(enum EventUpdateType t);

    /**
     * eventTypeToText
     * @return
     */
    std::string eventTypeToText();

protected:
    int                     m_EventSample;  /**< */
    int                     m_CreatorId;    /**< */
    long long               m_CreationTime; /**< */
    enum EventUpdateType    m_TypeOfUpdate; /**< */
};

/**
 * The EventSharedMemManager class
 */
class EventSharedMemManager
{
public:
    /**
     * EventSharedMemManager
     * @param[in] parent
     */
    explicit EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);

    /**
     *
     */
    ~EventSharedMemManager();
    /**
     * init
     * @param[in] mode
     */
    void init(EVENTSLIB::SharedMemoryMode mode);

    /**
     * isInit
     * @return
     */
    bool isInit() const;

    /**
     * stop
     */
    void stop();

    /**
     * addEvent
     * @param[in] sample
     */
    void addEvent(int sample);
    /**
     * deleteEvent
     * @param[in] sample
     */
    void deleteEvent(int sample);

    /**
     * getTimeNow
     * @return
     */
    static long long getTimeNow();

    /**
     * processEvent
     * @param[in] ne
     */
    void processEvent(const EventUpdate& ne);

private:
    /**
     * attachToSharedSegment
     * @param[in] mode
     */
    void attachToSharedSegment(QSharedMemory::AccessMode mode);

    /**
     * createSharedSegment
     * @param[in] bufferSize
     * @param[in] mode
     * @return
     */
    bool createSharedSegment(int bufferSize, QSharedMemory::AccessMode mode);

    /**
     * launchSharedMemoryWatcherThread
     */
    void launchSharedMemoryWatcherThread();

    /**
     * attachToOrCreateSharedSegment
     * @param[in] mode
     */
    void attachToOrCreateSharedSegment(QSharedMemory::AccessMode mode);

    /**
     * stopSharedMemoryWatcherThread
     */
    void stopSharedMemoryWatcherThread();

    /**
     * detachFromSharedMemory
     */
    void detachFromSharedMemory();

    /**
     * generateId
     * @return
     */
    inline static int generateId();

    /**
     * processNewEvent
     * @param[in] n
     */
    void processNewEvent(const EventUpdate& n);

    /**
     * processDeleteEvent
     * @param[in] n
     */
    void processDeleteEvent(const EventUpdate& n);

    /**
     * printLocalBuffer
     */
    void printLocalBuffer();

    /**
     * copyNewUpdateToSharedMemory
     * @param[in] newUpdate
     */
    void copyNewUpdateToSharedMemory(EventUpdate& newUpdate);

    /**
     * initializeSharedMemory
     */
    void initializeSharedMemory();

    /**
     * copySharedMemoryToLocalBuffer
     */
    void copySharedMemoryToLocalBuffer();

    /**
     * processLocalBuffer
     */
    void processLocalBuffer();

    /**
     * bufferWatcher
     */
    void bufferWatcher();

    /**
     * createGroupIfNeeded
     */
    void createGroupIfNeeded();

    static int                          m_iLastUpdateIndex;             /**<  */
    EVENTSLIB::EventManager*            m_pEventManager;                /**<  */
    QSharedMemory                       m_SharedMemory;                 /**<  */
    std::atomic_bool                    m_IsInit;                       /**<  */
    std::string                         m_sGroupName;                   /**<  */
    bool                                m_bGroupCreated;                /**<  */
    idNum                               m_GroupId;                      /**<  */
    int                                 m_SharedMemorySize;             /**<  */
    int                                 m_fTimerCheckBuffer;            /**<  */
    std::thread                         m_BufferWatcherThread;          /**<  */
    std::atomic_bool                    m_BufferWatcherThreadRunning;   /**<  */
    std::atomic_bool                    m_WritingToSharedMemory;        /**<  */
    long long                           m_lastCheckTime;                /**<  */
    EventUpdate*                        m_LocalBuffer;                  /**<  */
    EventUpdate*                        m_SharedBuffer;                 /**<  */
    int                                 m_Id;                           /**<  */
    enum EVENTSLIB::SharedMemoryMode    m_Mode;                         /**<  */
};

/**
 * EventSharedMemManager::generateId
 * @return
 */
inline int EventSharedMemManager::generateId()
{
    auto t = static_cast<int>(EventSharedMemManager::getTimeNow());
    return abs(t);
}

} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H

