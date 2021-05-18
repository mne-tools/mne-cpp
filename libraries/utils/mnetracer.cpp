#include "mnetracer.h"

using namespace UTILSLIB;

static const char* defaultTracerFileName = "default_MNETracer_file.json";
int MNETracer::numTracers = 0;
bool MNETracer::isEnabled = false;
std::ofstream MNETracer::outputFileStream;
bool MNETracer::isFirstEvent = true;
bool MNETracer::outFileMutex = false;
long long MNETracer::zeroTime = 0;

//=============================================================================================================

MNETracer::MNETracer(const std::string &function, const std::string &file, const int num)
: printToTerminal(false)
, fileName(file)
, threadId("0")
, beginTime(0)
, endTime(0)
, durationMicros(0)
, durationMilis(0.0)
{
    if (isEnabled)
    {
        initialize(function, file, num);
        writeBeginEvent();
    }
}

//=============================================================================================================

MNETracer::~MNETracer()
{
    if (isEnabled)
    {
        registerFinalTime();
        writeEndEvent();
        if (printToTerminal)
        {
            calculateDuration();
            printDurationMiliSec();
        }
    }
}

//=============================================================================================================

void MNETracer::enable(const std::string &jsonFileName)
{
    outputFileStream.open(jsonFileName);
    outFileMutex = false;
    writeHeader();
    setZeroTime();
    isEnabled = true;
}

//=============================================================================================================

void MNETracer::enable()
{
    enable(defaultTracerFileName);
}

//=============================================================================================================

void MNETracer::disable()
{
    if (MNETracer::isEnabled)
    {
        writeFooter();
        outputFileStream.close();
        outFileMutex = false;
        isEnabled = false;
    }
}

//=============================================================================================================

void MNETracer::start(const std::string &jsonFileName)
{
    enable(jsonFileName);
}

//=============================================================================================================

void MNETracer::start()
{
    enable();
}

//=============================================================================================================

void MNETracer::stop()
{
    disable();
}

//=============================================================================================================

void MNETracer::traceQuantity(const std::string &name, long val)
{
    long long timeNow = getTimeNow() - zeroTime;
    std::string s;
    s.append("{\"name\":\"").append(name).append("\",\"ph\":\"C\",\"ts\":");
    s.append(std::to_string(timeNow)).append(",\"pid\":1,\"tid\":1");
    s.append(",\"args\":{\"").append(name).append("\":").append(std::to_string(val)).append("}}\n");
    writeToFile(s);
}

//=============================================================================================================

void MNETracer::initialize(const std::string &function, const std::string &file, const int num)
{
    numTracers++;

    initializeFunctionName(function);
    initializeFile(file);
    lineNumber = num;

    registerThreadId();
    registerInitialTime();
}

//=============================================================================================================

void MNETracer::setZeroTime()
{
    zeroTime = getTimeNow();
}

//=============================================================================================================

void MNETracer::registerInitialTime()
{
    beginTime = getTimeNow() - zeroTime;
}

//=============================================================================================================

void MNETracer::registerFinalTime()
{
    endTime = getTimeNow() - zeroTime;
}

//=============================================================================================================

long long MNETracer::getTimeNow()
{
    auto timeNow = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::microseconds>(timeNow).time_since_epoch().count();
}

//=============================================================================================================

void MNETracer::registerThreadId()
{
    auto longId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    threadId = std::to_string(longId).substr(0, 5);
}

//=============================================================================================================

void MNETracer::initializeFunctionName(const std::string &function)
{
    functionName = function;
    std::string pattern(" __cdecl");
    size_t pos = functionName.find("__cdecl");
    if (pos != std::string::npos)
        functionName.replace(pos, pattern.length(), "");
}

//=============================================================================================================

void MNETracer::initializeFile(std::string file)
{
    std::string patternIn("\\");
    std::string patternOut("\\\\");
    size_t start_pos = 0;
    while ((start_pos = file.find(patternIn, start_pos)) != std::string::npos)
    {
        file.replace(start_pos, 1, patternOut);
        start_pos += patternOut.length();
    }

    fileName = file;
}

//=============================================================================================================

void MNETracer::calculateDuration()
{
    durationMicros = endTime - beginTime;
    durationMilis = durationMicros * 0.001;
}

//=============================================================================================================

void MNETracer::printDurationMiliSec()
{
    std::cout << "Scope: " << fileName << " - " << functionName << " DurationMs: " << durationMilis << "ms.\n";
}

//=============================================================================================================

void MNETracer::writeHeader()
{
    writeToFile("{\"displayTimeUnit\": \"ms\",\"traceEvents\":[\n");
}

//=============================================================================================================

void MNETracer::writeFooter()
{
    writeToFile("]}");
}

//=============================================================================================================

void MNETracer::writeToFile(const std::string &str)
{
    if (outFileMutex)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        writeToFile(str);
    }
    else
    {
        outFileMutex = true;
        outputFileStream << str;
        outputFileStream.flush();
        outFileMutex = false;
    }
}

//=============================================================================================================

void MNETracer::writeBeginEvent()
{
    std::string s;
    if (!isFirstEvent)
        s.append(",");

    s.append("{\"name\":\"").append(functionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"B\",\"ts\":").append(std::to_string(beginTime)).append(",\"pid\":1,\"tid\":");
    s.append(threadId).append(",\"args\":{\"file path\":\"").append(fileName).append("\",\"line number\":");
    s.append(std::to_string(lineNumber)).append("}}\n");
    writeToFile(s);
    isFirstEvent = false;
}

//=============================================================================================================

void MNETracer::writeEndEvent()
{
    std::string s;
    s.append(",{\"name\":\"").append(functionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"E\",\"ts\":").append(std::to_string(endTime)).append(",\"pid\":1,\"tid\":");
    s.append(threadId).append(",\"args\":{\"file path\":\"").append(fileName).append("\",\"line number\":");
    s.append(std::to_string(lineNumber)).append("}}\n");
    writeToFile(s);
}

//=============================================================================================================

bool MNETracer::printToTerminalIsSet()
{
    return printToTerminal;
}

//=============================================================================================================

void MNETracer::setPrintToTerminal(bool s)
{
    printToTerminal = s;
}
