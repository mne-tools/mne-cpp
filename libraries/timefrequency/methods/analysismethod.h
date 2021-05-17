#ifndef ANALYSISMETHOD_H
#define ANALYSISMETHOD_H


class AnalysisMethod
{
public:
    AnalysisMethod() = default;

    virtual bool compute() = 0;
    virtual bool validateSettings() = 0;

};

#endif // ANALYSISMETHOD_H
