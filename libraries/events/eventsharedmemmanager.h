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

namespace EVENTSINTERNAL {

//=========================================================================================================
/**
 * The type enum specifies what kind of event happened.
 */
enum EventUpdateType{ NULL_EVENT, NEW_EVENT, DELETE_EVENT};

//=========================================================================================================
/**
 * Allows for a text description of each option in enum EventUpdateType.
 */
const std::string EventUpdateTypeString[3] = {"Null Event", "New Event", "Delete Event"};

/**
 * The EventUpdate class serves as a minimal class definition to store updates in the shared memory buffer.
 * The updates are events that happened and need to be processed by a shared memory reader object.
 */
struct EventUpdate
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     */
    EventUpdate();

    //=========================================================================================================
    /**
     * Constructor with members defined.
     * @param[in] sample
     * @param[in] creator
     * @param[in] t
     */
    EventUpdate(int sample, int creator, enum EventUpdateType t);

    //=========================================================================================================
    /**
     * Retrieve the creation time.
     * @return A long integer with the creation time.
     */
    long long getCreationTime() const;

    //=========================================================================================================
    /**
     * Retrieve the sample number.
     * @return Sample.
     */
    int getSample() const;

    //=========================================================================================================
    /**
     * retrieve the creator's id.
     * @return Creator's id.
     */
    int getCreatorId() const;

    //=========================================================================================================
    /**
     * Retrieve the type.
     * @return Type.
     */
    enum EventUpdateType getType() const;

    //=========================================================================================================
    /**
     * set the type of the event update.
     * @param[in] t
     */
    void setType(enum EventUpdateType t);

    //=========================================================================================================
    /**
     * Retrieve a text descriptive tag for each option in the type of update variable.
     * @return
     */
    std::string eventTypeToText();

protected:
    int                     m_EventSample;  /**< Sample for the event. */
    int                     m_CreatorId;    /**< Id of the creator. */
    long long               m_CreationTime; /**< Creation time point. */
    enum EventUpdateType    m_TypeOfUpdate; /**< Type of update. */
};

/**
 * The EventSharedMemManager class
 */
class EventSharedMemManager
{
public:
    //=========================================================================================================
    /**
     * EventSharedMemManager
     * @param[in] Pointer to Parent
     */
    explicit EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);

    //=========================================================================================================
    /**
     *
     */
    ~EventSharedMemManager();

    //=========================================================================================================
    /**
     * init
     * @param[in] mode
     */
    void init(EVENTSLIB::SharedMemoryMode mode);

    //=========================================================================================================
    /**
     * isInit
     * @return
     */
    bool isInit() const;

    //=========================================================================================================
    /**
     * stop
     */
    void stop();

    //=========================================================================================================
    /**
     * Add an event.
     * @param[in] sample
     */
    void addEvent(int sample);

    //=========================================================================================================
    /**
     * deleteEvent
     * @param[in] sample
     */
    void deleteEvent(int sample);

    //=========================================================================================================
    /**
     * getTimeNow
     * @return
     */
    static long long getTimeNow();

    //=========================================================================================================
    /**
     * processEvent
     * @param[in] ne
     */
    void processEvent(const EventUpdate& ne);

private:
    //=========================================================================================================
    /**
     * attachToSharedSegment
     * @param[in] mode
     */
    void attachToSharedSegment(QSharedMemory::AccessMode mode);

    //=========================================================================================================
    /**
     * createSharedSegment
     * @param[in] bufferSize
     * @param[in] mode
     * @return
     */
    bool createSharedSegment(int bufferSize, QSharedMemory::AccessMode mode);

    //=========================================================================================================
    /**
     * launchSharedMemoryWatcherThread
     */
    void launchSharedMemoryWatcherThread();

    //=========================================================================================================
    /**
     * attachToOrCreateSharedSegment
     * @param[in] mode
     */
    void attachToOrCreateSharedSegment(QSharedMemory::AccessMode mode);

    //=========================================================================================================
    /**
     * stopSharedMemoryWatcherThread
     */
    void stopSharedMemoryWatcherThread();

    //=========================================================================================================
    /**
     * detachFromSharedMemory
     */
    void detachFromSharedMemory();

    //=========================================================================================================
    /**
     * generateId
     * @return
     */
    inline static int generateId();

    //=========================================================================================================
    /**
     * processNewEvent
     * @param[in] n
     */
    void processNewEvent(const EventUpdate& n);

    //=========================================================================================================
    /**
     * processDeleteEvent
     * @param[in] EventUpdate.
     */
    void processDeleteEvent(const EventUpdate& n);

    //=========================================================================================================
    /**
     * printLocalBuffer
     */
    void printLocalBuffer();

    //=========================================================================================================
    /**
     * copyNewUpdateToSharedMemory
     * @param[in] newUpdate
     */
    void copyNewUpdateToSharedMemory(EventUpdate& newUpdate);

    //=========================================================================================================
    /**
     * initializeSharedMemory
     */
    void initializeSharedMemory();

    //=========================================================================================================
    /**
     * copySharedMemoryToLocalBuffer
     */
    void copySharedMemoryToLocalBuffer();

    //=========================================================================================================
    /**
     * processLocalBuffer
     */
    void processLocalBuffer();

    //=========================================================================================================
    /**
     * bufferWatcher
     */
    void bufferWatcher();

    //=========================================================================================================
    /**
     * createGroupIfNeeded
     */
    void createGroupIfNeeded();

    static int                          m_iLastUpdateIndex;             /**<  The last position in the buffer to be updated.*/
    EVENTSLIB::EventManager*            m_pEventManager;                /**<  Pointer to the parent EventManager object.*/
    QSharedMemory                       m_SharedMemory;                 /**<  Multiplatform Qt shared memory object.*/
    std::atomic_bool                    m_IsInit;                       /**<  Flag if the shared memory has not been initialized.*/
    std::string                         m_sGroupName;                   /**<  Group name to use when creating events in the shared memory segment.*/
    bool                                m_bGroupCreated;                /**<  Check if the group has already been created.*/
    idNum                               m_GroupId;                      /**<  Store the group ID of the event group to which events will be assigned.*/
    int                                 m_SharedMemorySize;             /**<  Size of the shared memory segment.*/
    int                                 m_fTimerCheckBuffer;            /**<  Time period between checks of the shared buffer.*/
    std::thread                         m_BufferWatcherThread;          /**<  Offloaded thread to check for new events.*/
    std::atomic_bool                    m_BufferWatcherThreadRunning;   /**<  Flag if the BufferWatcher thread has been created.*/
    std::atomic_bool                    m_WritingToSharedMemory;        /**<  Mutex to control writing new events to the shared memory buffer.*/
    long long                           m_lastCheckTime;                /**<  Place holder for the time when the buffer was last checked.*/
    EventUpdate*                        m_LocalBuffer;                  /**<  Buffer in the local memory of this application, mirroring the shared memory buffer.*/
    EventUpdate*                        m_SharedBuffer;                 /**<  Buffer in shared memory segment.*/
    int                                 m_Id;                           /**<  Stores the creator Id.*/
    enum EVENTSLIB::SharedMemoryMode    m_Mode;                         /**<  Shared memory working mode.*/
};

//=========================================================================================================
/**
 * EventSharedMemManager::generateId
 * @return
 */
inline int EventSharedMemManager::generateId()
{
    auto t = static_cast<int>(EventSharedMemManager::getTimeNow());
    return abs(t);
}

} //namespace EVENTSINTERNAL
}//namespace EVENTSLIB
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H

