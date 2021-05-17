#include "timefrequencyanalyzer.h"

using namespace TIMEFREQUENCYLIB;

TimeFrequencyAnalyzer::TimeFrequencyAnalyzer(AnalysisMethod& method)
 : m_method(method)
{


}

bool TimeFrequencyAnalyzer::run()
{
    bool validRun = false;
    bool validSettings = m_method.validateSettings();

    if(validSettings){
        validRun = m_method.compute();
    }

    return validRun;
}
