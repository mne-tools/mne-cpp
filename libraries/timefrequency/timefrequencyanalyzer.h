#ifndef TIMEFREQUENCYANALYZER_H
#define TIMEFREQUENCYANALYZER_H

#include <memory>

namespace TIMEFREQUENCYLIB {

enum Method{
    Superlets
};

struct TimeFrequencySettings {
    Method m_method;
    TimeFrequencySettings() = default;
};

class TimeFrequencyAnalyzer
{
public:
    TimeFrequencyAnalyzer(const TimeFrequencySettings& settings);

    void run();

private:
    bool vaildateSettings();

    TimeFrequencySettings m_pSettings;
};
}//namespace

#endif // TIMEFREQUENCYANALYZER_H
