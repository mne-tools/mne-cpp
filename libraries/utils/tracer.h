#ifndef TRACER_H
#define TRACER_H

#include "utils_global.h"

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>

#define __PRETTY_FUNCTION__ __FUNCSIG__

#ifdef TRACE
#define __TRACE_FUNC() UTILSLIB::Tracer _t__LINE__(__PRETTY_FUNCTION__, __FILE__, __LINE__);
#define __TRACER_ENABLE UTILSLIB::Tracer::enable("chrome_tracer.json");
#define __TRACER_DISABLE UTILSLIB::Tracer::disable();
#define __TRACE_VALUE(NAME, VALUE) UTILSLIB::Tracer::traceQuantity(NAME, VALUE);
#else
#define __TRACE_FUNC() PEPE
#define __TRACER_ENABLE PEPE
#define __TRACER_DISABLE PEPE
#define __TRACE_VALUE() PEPE
#endif

#ifdef TRACE_MEMORY
#define __TRACE_MEMORY_REPORT \
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
#define __TRACE_MEMORY_REPORT
#endif

namespace UTILSLIB
{

  class UTILSSHARED_EXPORT Tracer
  {
  public:
    Tracer(const std::string &function, const std::string &file, const int num);

    ~Tracer();

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
