//=============================================================================================================
/**
 * @file     threads.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     January, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel Motta. All rights reserved.
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
 * @brief    Contains threading classes.
 *
 */

#ifndef MNESCAN_THREADS_H
#define MNESCAN_THREADS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <thread>
#include <atomic>
#include <chrono>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

class Thread
{
public:
    //=========================================================================================================
    /**
     * Constructs an "empty" object.
     */
    Thread(): m_bIsRunning(false), m_bTriggerStop(false) {};


    template<class Function, class... Args >
    //=========================================================================================================
    /**
     * Constructs a thread object to run a function in a separate thread.
     *
     * @param[in] func  function to be run in separate thread
     * @param[in] args  function arguments.
     *                  (Note - member functions need to be passed usually-implicit 'this' pointer)
     */
    Thread(Function&& func, Args&&... args): m_thread(func, args...), m_bIsRunning(true), m_bTriggerStop(false){}

    //=========================================================================================================
    /**
     * Returns whether the thread is running.
     */
    bool isRunning() const {return m_bIsRunning;}

    //=========================================================================================================
    /**
     * Returns whether a stop has been triggered.
     */
    bool stopTriggered() const {return m_bTriggerStop;}

    //=========================================================================================================
    /**
     * Attempts to join thread. Underlying function needs to finish before this happens.
     * Recommend using the 'stopTriggered' method to check, similarly to 'interruptionRequested'
     */
    void stop() {m_bTriggerStop = true; m_thread.join(); m_bIsRunning = false;}

private:
    std::thread m_thread;                       /**< The actual thread. */
    std::atomic_bool m_bIsRunning;              /**< Whether the thred is runing. */
    std::atomic_bool m_bTriggerStop;            /**< Whether a thread interruption has been requested. */
};


}//namespace

#endif // MNESCAN_THREADS_H
