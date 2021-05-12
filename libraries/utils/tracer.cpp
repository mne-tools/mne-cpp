#include "tracer.h"

using namespace UTILSLIB;

int Tracer::numTracers = 0;
bool Tracer::isEnabled = false;
std::ofstream Tracer::outputFileStream;
bool Tracer::isFirstEvent = true;
bool Tracer::outFileMutex = false;
long long Tracer::zeroTime = 0;

Tracer::Tracer(const std::string &function, const std::string &file, const int num)
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

Tracer::~Tracer()
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

void Tracer::enable(const std::string &jsonFileName)
{
  outputFileStream.open(jsonFileName);
  outFileMutex = false;
  writeHeader();
  setZeroTime();
  isEnabled = true;
}

void Tracer::enable()
{
  enable("default_tracer_file.json");
}

void Tracer::disable()
{
  if (Tracer::isEnabled)
  {
    writeFooter();
    outputFileStream.close();
    outFileMutex = false;
    isEnabled = false;
  }
}

void Tracer::start(const std::string &jsonFileName)
{
  enable(jsonFileName);
}

void Tracer::start()
{
  enable();
}

void Tracer::stop()
{
  disable();
}

void Tracer::traceQuantity(std::string &name, long val)
{
  long long timeNow = getTimeNow() - zeroTime;
  std::string s;
  s.append("{\"name\":\"").append(name).append("\",\"ph\":\"C\",\"ts\":");
  s.append(std::to_string(timeNow)).append(",\"pid\":1,\"tid\":1");
  s.append(",\"args\":{\"").append(name).append("\":").append(std::to_string(val)).append("}}\n");
  writeToFile(s);
}

void Tracer::initialize(const std::string &function, const std::string &file, const int num)
{
  numTracers++;

  initializeFunctionName(function);
  initializeFile(file);
  lineNumber = num;

  registerThreadId();
  registerInitialTime();
}

void Tracer::setZeroTime()
{
  zeroTime = getTimeNow();
}

void Tracer::registerInitialTime()
{
  beginTime = getTimeNow() - zeroTime;
}

void Tracer::registerFinalTime()
{
  endTime = getTimeNow() - zeroTime;
}

long long Tracer::getTimeNow()
{
  auto timeNow = std::chrono::high_resolution_clock::now();
  return std::chrono::time_point_cast<std::chrono::microseconds>(timeNow).time_since_epoch().count();
}

void Tracer::registerThreadId()
{
  auto longId = std::hash<std::thread::id>{}(std::this_thread::get_id());
  threadId = std::to_string(longId).substr(0, 5);
}

void Tracer::initializeFunctionName(const std::string &function)
{
  functionName = function;
  std::string pattern(" __cdecl");
  size_t pos = functionName.find("__cdecl");
  if (pos != std::string::npos)
    functionName.replace(pos, pattern.length(), "");
}

void Tracer::initializeFile(std::string file)
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

void Tracer::calculateDuration()
{
  durationMicros = endTime - beginTime;
  durationMilis = durationMicros * 0.001;
}

void Tracer::printDurationMiliSec()
{
  std::cout << "Scope: " << fileName << " - " << functionName << " DurationMs: " << durationMilis << "ms.\n";
}

void Tracer::writeHeader()
{
  writeToFile("{\"displayTimeUnit\": \"ms\",\"traceEvents\":[\n");
}

void Tracer::writeFooter()
{
  writeToFile("]}");
}

void Tracer::writeToFile(const std::string &str)
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

void Tracer::writeBeginEvent()
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

void Tracer::writeEndEvent()
{
  std::string s;
  s.append(",{\"name\":\"").append(functionName).append("\",\"cat\":\"bst\",");
  s.append("\"ph\":\"E\",\"ts\":").append(std::to_string(endTime)).append(",\"pid\":1,\"tid\":");
  s.append(threadId).append(",\"args\":{\"file path\":\"").append(fileName).append("\",\"line number\":");
  s.append(std::to_string(lineNumber)).append("}}\n");
  writeToFile(s);
}

bool Tracer::printToTerminalIsSet()
{
  return printToTerminal;
}

void Tracer::setPrintToTerminal(bool s)
{
  printToTerminal = s;
}
