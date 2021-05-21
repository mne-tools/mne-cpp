//=============================================================================================================
/**
 * @file     mnetracer.cpp
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     May, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021 Juan GPC. All rights reserved.
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
 * @brief    Implements the mnetracer class.
 *
 */

#include "mnetracer.h"


using namespace UTILSLIB;

//=============================================================================================================
// DEFINE STATIC MEMBER VARIABLES
//=============================================================================================================

static const char* defaultTracerFileName("default_MNETracer_file.json");
bool MNETracer::ms_bIsEnabled(false);
std::ofstream MNETracer::ms_OutputFileStream;
bool MNETracer::ms_bIsFirstEvent(true);
std::mutex MNETracer::ms_outFileMutex;
long long MNETracer::ms_iZeroTime(0);

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNETracer::MNETracer(const std::string &file, const std::string &function, int lineNumber)
: m_bIsInitialized(false)
, m_bPrintToTerminal(false)
, m_sFileName(file)
, m_sFunctionName(function)
, m_iLineNumber(lineNumber)
, m_iThreadId("0")
, m_iBeginTime(0)
, m_iEndTime(0)
, m_dDurationMilis(0.)
{
    if (ms_bIsEnabled)
    {
        initialize();
        writeBeginEvent();
    }
}

//=============================================================================================================

MNETracer::~MNETracer()
{
    if (ms_bIsEnabled && m_bIsInitialized)
    {
        registerFinalTime();
        writeEndEvent();
        if (m_bPrintToTerminal)
        {
            calculateDuration();
            printDurationMiliSec();
        }
    }
}

//=============================================================================================================

void MNETracer::enable(const std::string &jsonFileName)
{
    ms_OutputFileStream.open(jsonFileName);
    writeHeader();
    setZeroTime();
    if (ms_OutputFileStream.is_open())
    {
        ms_bIsEnabled = true;
    }
}

//=============================================================================================================

void MNETracer::enable()
{
    enable(defaultTracerFileName);
}

//=============================================================================================================

void MNETracer::disable()
{
    if (ms_bIsEnabled)
    {
        writeFooter();
        ms_OutputFileStream.flush();
        ms_OutputFileStream.close();
        ms_bIsEnabled = false;
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
    long long timeNow = getTimeNow() - ms_iZeroTime;
    std::string s;
    s.append("{\"name\":\"").append(name).append("\",\"ph\":\"C\",\"ts\":");
    s.append(std::to_string(timeNow)).append(",\"pid\":1,\"tid\":1");
    s.append(",\"args\":{\"").append(name).append("\":").append(std::to_string(val)).append("}}\n");
    writeToFile(s);
}

//=============================================================================================================

void MNETracer::initialize()
{
    registerConstructionTime();
    registerThreadId();
    formatFileName();
    m_bIsInitialized = true;
}

//=============================================================================================================

void MNETracer::setZeroTime()
{
    ms_iZeroTime = getTimeNow();
}

//=============================================================================================================

void MNETracer::registerConstructionTime()
{
    m_iBeginTime = getTimeNow() - ms_iZeroTime;
}

//=============================================================================================================

void MNETracer::registerFinalTime()
{
    m_iEndTime = getTimeNow() - ms_iZeroTime;
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
    m_iThreadId = std::to_string(longId).substr(0, 5);
}

//=============================================================================================================

void MNETracer::formatFunctionName()
{
    const char* pattern(" __cdecl");
    constexpr int patternLenght(8);
    size_t pos = m_sFunctionName.find(pattern);
    if (pos != std::string::npos) {
        m_sFunctionName.replace(pos, patternLenght, "");
    }
}

//=============================================================================================================

void MNETracer::formatFileName()
{
    const char* patternIn("\\");
    const char* patternOut("\\\\");
    constexpr int patternOutLength(4);
    size_t start_pos = 0;
    while ((start_pos = m_sFileName.find(patternIn, start_pos)) != std::string::npos)
    {
        m_sFileName.replace(start_pos, 1, patternOut);
        start_pos += patternOutLength;
    }
}

//=============================================================================================================

void MNETracer::calculateDuration()
{
    m_dDurationMilis = (m_iEndTime - m_iBeginTime) * 0.001;
}

//=============================================================================================================

void MNETracer::printDurationMiliSec()
{
    std::cout << "Scope: " << m_sFileName << " - " << m_sFunctionName << " DurationMs: " << m_dDurationMilis << "ms.\n";
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

void MNETracer::writeToFile(const std::string& str)
{
    ms_outFileMutex.lock();
    if(ms_OutputFileStream.is_open()) {
        ms_OutputFileStream << str;
    }
    ms_outFileMutex.unlock();
}

//=============================================================================================================

void MNETracer::writeBeginEvent()
{
    std::string s;
    if (!ms_bIsFirstEvent)
        s.append(",");

    s.append("{\"name\":\"").append(m_sFunctionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"B\",\"ts\":").append(std::to_string(m_iBeginTime)).append(",\"pid\":1,\"tid\":");
    s.append(m_iThreadId).append(",\"args\":{\"file path\":\"").append(m_sFileName).append("\",\"line number\":");
    s.append(std::to_string(m_iLineNumber)).append("}}\n");
    writeToFile(s);
    ms_bIsFirstEvent = false;
}

//=============================================================================================================

void MNETracer::writeEndEvent()
{
    std::string s;
    s.append(",{\"name\":\"").append(m_sFunctionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"E\",\"ts\":").append(std::to_string(m_iEndTime)).append(",\"pid\":1,\"tid\":");
    s.append(m_iThreadId).append(",\"args\":{\"file path\":\"").append(m_sFileName).append("\",\"line number\":");
    s.append(std::to_string(m_iLineNumber)).append("}}\n");
    writeToFile(s);
}

//=============================================================================================================

bool MNETracer::printToTerminalIsSet()
{
    return m_bPrintToTerminal;
}

//=============================================================================================================

void MNETracer::setPrintToTerminal(bool s)
{
    m_bPrintToTerminal = s;
}

