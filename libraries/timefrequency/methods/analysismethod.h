#ifndef ANALYSISMETHOD_H
#define ANALYSISMETHOD_H

#include "helpers/analysisIO.h"

namespace TIMEFREQUENCYLIB {

class AnalysisMethod
{
public:
    AnalysisMethod() = default;

    virtual bool compute() = 0;
    virtual bool validateSettings() = 0;
    virtual TimeFrequencyResult getResult() = 0;

};
}//namespace
#endif // ANALYSISMETHOD_H
