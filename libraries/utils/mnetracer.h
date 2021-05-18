//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Juan GPC <jgarciaprietoh@mgh.harvard.edu>;
 * @since    0.1.9
 * @date     May 18, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of a MNETracer object. This class allows a user to easily measure executions times
 * of a function or a block of code, and formats the output so that it is compatible with Chrome Tracer application.
 *
 */

#ifndef TRACER_H
#define TRACER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#ifdef TRACE
#define MNE_TRACE() UTILSLIB::MNETracer _mneTracer__LINE__(__func__, __FILE__, __LINE__);
#define MNE_TRACER_ENABLE(FILENAME) UTILSLIB::MNETracer::enable(#FILENAME);
#define MNE_TRACER_DISABLE UTILSLIB::MNETracer::disable();
#define MNE_TRACE_VALUE(NAME, VALUE) UTILSLIB::MNETracer::traceQuantity(NAME, VALUE);
#else
#define MNE_TRACE() PEPE
#define MNE_TRACER_ENABLE
#define MNE_TRACER_DISABLE
#define MNE_TRACE_VALUE()
#endif

#ifdef MNE_TRACE_MEMORY
#define MNE_TRACE_MEMORY_REPORT \
std::cout << "Total Memory Allocated : " << 0.000001 * __totalMemAllocated << " MB\n";\
std::cout << "Total Memory Deleted : " << 0.000001 * __totalMemDeleted << " MB\n";\
std::cout << "Total Memory Diff : " << 0.000001 * (__totalMemAllocated - __totalMemDeleted) << " MB\n";\

size_t __totalMemAllocated(0);
size_t __totalMemDeleted(0);
void *operator new(size_t size)
{
    // std::cout << "Allocating " << size << " Bytes.\n";
    __totalMemAllocated += size;
    return malloc(size);
}

void operator delete(void *memory, size_t size)
{
    // std::cout << "Deleting " << size << " Bytes.\n";
    __totalMemDeleted += size;
    free(memory);
}
#else
#define MNE_TRACE_MEMORY_REPORT
#endif

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace UTILSLIB
{

/**
 * The MNETracer is defined as a single class helper tool to measure execution times of blocks of code
 * (includeing scope blocks of code between brackets) that formats the output so that it is compatible with
 * Chrome browser's Tracing application (json format).
 *
 * The class is defined so that it has a some settings (ie. output file, zero time, etc...) which correspond to static
 * variables shared and accessible from all the instances of this class MNETracer. Each instance, in its contructor will
 * record the creation time and write to file that begin-measurement event.
 * Whenever the MNETracer object is destructued (normally by falling out of scope), the destructor of this class MNETracer will
 * be called and it is in the desctructor where the end-measurement event is recorded and written to file.
 *
 * There are some additional macros defined to make is handy for the user to use this class.
 * MNE_TRACER_ENABLE(filename) and MNE_TRACER_DISABLE macros will set the static variables like the output file initialization and
 * a few other needed variables. This should be called before any MNETracer is created, and after the last MNETracer object is destructed.
 * For instance, in the main.cpp file.
 * The MNE_TRACE() macro is to be used for marking which method, function or block of code is to be measured and traced.
 */
class UTILSSHARED_EXPORT MNETracer
{
public:
    MNETracer(const std::string &function, const std::string &file, const int num);
    ~MNETracer();
    static void enable(const std::string &jsonFileName);
    static void enable();
    static void disable();
    static void start(const std::string &jsonFileName);
    static void start();
    static void stop();
    static void traceQuantity(std::string &name, long val);
    bool printToTerminalIsSet();
    void setPrintToTerminal(bool s);

private:
    static void writeHeader();
    static void writeFooter();
    static void writeToFile(const std::string &str);
    static void setZeroTime();
    static long long getTimeNow();
    void initialize(const std::string &function, const std::string &file, const int num);
    void initializeFunctionName(const std::string &function);
    void initializeFile(std::string file);
    void registerInitialTime();
    void registerFinalTime();
    void registerThreadId();
    void calculateDuration();
    void printDurationMiliSec();
    void writeBeginEvent();
    void writeEndEvent();

    static int numTracers;                  /**< Number of MNETracer objs instantiated. */
    static bool isEnabled;                  /**< Bool variable to store if the "class" (ie. the MNETracer) has been enabled. */
    static std::ofstream outputFileStream;  /**< Output file stream to write results. */
    static bool isFirstEvent;               /**< Bool variable to check if this is the first event to be written to the file. */
    static bool outFileMutex;               /**< Mutex to guard the writing to file between threads. */
    static long long zeroTime;              /**< Integer value to store the origin-time (ie the Zero time) from which all other time measurements will depend. */

    bool printToTerminal;       /**< Store if it is needed from this MNETracer object. to print to terminal too. */
    std::string functionName;   /**< String to store the function name. */
    std::string fileName;       /**< String to store the code file name where the MNETracer obj is instantiated. */
    int lineNumber;             /**< The line number within the code file where the MNETracer obj is instantiated. */
    std::string threadId;       /**< A string identifier for the thread id in which the MNETracer obj is instantiated. */
    long long beginTime;        /**< The time when the tracer MNETracer obj is created. */
    long long endTime;          /**< The time when the tracer MNETracer obj is destructed. */
    long long durationMicros;   /**< The time difference between MNETracer object creation and destruction in micro seconds. */
    double durationMilis;       /**< The time difference between MNETracer object creation and destruction in in milli seconds. */
}; // MNETracer

} // namespace UTILSLIB

#endif // TRACER_H
