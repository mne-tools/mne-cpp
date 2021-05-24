#ifndef TIMEFREQUENCYANALYZER_H
#define TIMEFREQUENCYANALYZER_H

#include <memory>

#include "methods/analysismethod.h"
#include "methods/helpers/analysisIO.h"

namespace TIMEFREQUENCYLIB {

class TimeFrequencyAnalyzer
{
public:
    TimeFrequencyAnalyzer(AnalysisMethod& method);

    TimeFrequencyResult compute(TimeFrequencyInput input);

private:
    AnalysisMethod& m_method;
};
}//namespace

#endif // TIMEFREQUENCYANALYZER_H
