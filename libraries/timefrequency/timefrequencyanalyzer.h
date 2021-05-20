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

    bool run();

    TimeFrequencyResult compute(TimeFrequencyInput input);

    bool setInput(const TimeFrequencyInput& data);

    TimeFrequencyResult getResult() const;

private:
    AnalysisMethod& m_method;
};
}//namespace

#endif // TIMEFREQUENCYANALYZER_H
