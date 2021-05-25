//=============================================================================================================
/**
 * @file     mnetracer.h
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
// MACRO DEFINITIONS
//=============================================================================================================

#ifdef TRACE
#define MNE_TRACE() UTILSLIB::MNETracer _mneTracer__LINE__(__FILE__,__func__,__LINE__);
#define MNE_TRACER_ENABLE(FILENAME) UTILSLIB::MNETracer::enable(#FILENAME);
#define MNE_TRACER_DISABLE UTILSLIB::MNETracer::disable();
#define MNE_TRACE_VALUE(NAME, VALUE) UTILSLIB::MNETracer::traceQuantity(NAME, VALUE);
#else
#define MNE_TRACE()
#define MNE_TRACER_ENABLE(FILENAME)
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
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>

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
 * be called and it is in the desctructor where the end-measurement event is recorded and written to file. So the time-alive (time
 * between the constructor and the desctructor calls, for objects of this class, will be linked to a specific scope (i.e. function).
 *
 * Since this class is oriented as a development tool, all the events are written to the output file stream directly, so there
 * should be not much difficulty recovering the results even if the application crashed.
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
    /**
     * @brief MNETracer constructor will check if the class "is enabled". If it is, it will record the creation time with respect to the
     * ZeroTime set during the last enable function call. It will also write that time to the output file and if needed, it will also print
     * it to terminal.
     * @param function Function name where the MNETracer object is created.
     * @param file File name where the MNETracer object is created.
     * @param num Line number where the MNETracer object is created.
     */
    MNETracer(const std::string& file, const std::string& function, int lineNumber);

    /**
     * MNETracer destructor will check if the class "is enabled". If it is, it will record the destruction time with respect to the
     * ZeroTime set during the last enable function call. It will also write that time to the output file and, if needed, it will also print
     * it to terminal.
     */
    ~MNETracer();

    /**
     * The enable function initializes an output file (output file stream ie std::ofstream) to write the events.
     * @param jsonFileName is the name of the output file to configure as the outuput file (it is in json format).
     */
    static void enable(const std::string& jsonFileName);

    /**
     * Overriden function for enable, but using the default filename.
     */
    static void enable();

    /**
     * @brief disable If the class "is enabled" (it's static variabable ms_bIsEnabled is true), the output file has a Footer written to it and the
     * output file stream is closed. Finally, the static member variable ms_bIsEnabled is set to false.
     */
    static void disable();

    /**
     * @brief Convenience overload of the method enable.
     * @param jsonFileName
     */
    static void start(const std::string& jsonFileName);

    /**
     * @brief Convenience overload of the method enable.
     */
    static void start();

    /**
     * @brief Convenience overload of the method disable.
     */
    static void stop();

    /**
     * @brief traceQuantity Allows to keep track of a specific variable in the output tracing file.
     * @param name Name of the variable to keep track of.
     * @param val Value of the variable to keep track of.
     */
    static void traceQuantity(const std::string& name, long val);

    /**
     * Getter function for the member variable that defines whether the output should be printed to terminal, or only to a file.
     * @return bool value.
     */
    bool printToTerminalIsSet();

    /**
     * Setter function for the member variable that defines whether the output should be printed to terminal, or only to a file.
     * @param s bool value to set the output to terminal control member variable.
     */
    void setPrintToTerminal(bool s);

private:
    /**
     * @brief writeHeader The outputfile needs a specific header to be compatible with Chrome Tracer app. This ads this header to
     * the output file.
     */
    static void writeHeader();

    /**
     * @brief writeFooter The outputfile needs a specific footer to be compatible with Chrome Tracer app. and close the arrays defined in
     * the json file.  This ads this footer (closing brackets) to the output file.
     */
    static void writeFooter();

    /**
     * @brief writeToFile This function writes a string to the output file.
     * @param str String to write.
     */
    static void writeToFile(const std::string& str);

    /**
     * @brief setZeroTime Sets the zero time, which is the time that will be considered zero in the tracer result. Typically, this is called
     * by the enable function.
     */
    static void setZeroTime();

    /**
     * @brief getTimeNow Wrapper function over chronos std library functionality to get the tick of this instant (in microseconds).
     * @return The actual time now in microseconds.
     */
    static long long getTimeNow();

    /**
     * @brief initialize Formats the fileName, FunctionName and line of code text shown in each event saved to the output file.
     * It then registers the threadId and the construction time for each instance of the class.
     */
    void initialize();

    /**
     * Removes extra back-slashes from the filename string.
     */
    void formatFileName();

    /**
     * removes  "__cdecl" from the function name.
     */
    void formatFunctionName();

    /**
     * Saves the construction time of this object. To later compute the time between construction and destruction of each object.
     */
    void registerConstructionTime();

    /**
     * Saves the destruction time of this object. See registerConstructionTime.
     */
    void registerFinalTime();

    /**
     * Stores a string, definiing a specific thread based on a hashing function. The same thread will always have the same stringId.
     */
    void registerThreadId();

    /**
     * Calculate duration between construction and destruction of an object.
     */
    void calculateDuration();

    /**
     * Print duration in miliseconds.
     */
    void printDurationMiliSec();

    /**
     * Write an event of type B (begin) in the output file.
     */
    void writeBeginEvent();

    /**
     * Write an event of type E (end) in the ouptut file.
     */
    void writeEndEvent();

    static bool ms_bIsEnabled;                  /**< Bool variable to store if the "class" (ie. the MNETracer) has been enabled. */
    static std::ofstream ms_OutputFileStream;   /**< Output file stream to write results. */
    static bool ms_bIsFirstEvent;               /**< Bool variable to check if this is the first event to be written to the file. */
    static std::mutex ms_outFileMutex;          /**< Mutex to guard the writing to file between threads. */
    static long long ms_iZeroTime;              /**< Integer value to store the origin-time (ie the Zero time) from which all other time measurements will depend. */

    bool m_bIsInitialized;          /**< Store if this object has been initialized properly. */
    bool m_bPrintToTerminal;        /**< Store if it is needed from this MNETracer object. to print to terminal too. */
    std::string m_sFileName;        /**< String to store the code file name where the MNETracer obj is instantiated. */
    std::string m_sFunctionName;    /**< String to store the function name. */
    int m_iLineNumber;              /**< The line number within the code file where the MNETracer obj is instantiated. */
    std::string m_iThreadId;        /**< A string identifier for the thread id in which the MNETracer obj is instantiated. */
    long long m_iBeginTime;         /**< The time when the tracer MNETracer obj is created. */
    long long m_iEndTime;           /**< The time when the tracer MNETracer obj is destructed. */
    double m_dDurationMilis;        /**< The time difference between MNETracer object creation and destruction in in milli seconds. */
}; // MNETracer

} // namespace UTILSLIB

#endif //if TRACE defined

