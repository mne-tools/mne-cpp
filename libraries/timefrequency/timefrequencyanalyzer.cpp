#include "timefrequencyanalyzer.h"

using namespace TIMEFREQUENCYLIB;

TimeFrequencyAnalyzer::TimeFrequencyAnalyzer(AnalysisMethod& method)
 : m_method(method)
{

}

TimeFrequencyResult TimeFrequencyAnalyzer::compute(TimeFrequencyInput input)
{
    bool validSettings = m_method.validateSettings();

    TimeFrequencyResult output;

    if(validSettings){
        m_method.compute(input, output);
    }
}
