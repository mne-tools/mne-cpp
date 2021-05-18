#ifndef TRACER_H
#define TRACER_H

#include "utils_global.h"

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>

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

namespace UTILSLIB
{

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

    static int numTracers;
    static bool isEnabled;
    static std::ofstream outputFileStream;
    static bool isFirstEvent;
    static bool outFileMutex;
    static long long zeroTime;

    bool printToTerminal;
    std::string functionName;
    std::string fileName;
    int lineNumber;
    std::string threadId;
    long long beginTime;
    long long endTime;
    long long durationMicros;
    double durationMilis;
};

} // namespace ChromeTracer
#endif // TRACER_H
