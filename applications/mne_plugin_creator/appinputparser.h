#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include <QTextStream>

#include <stdexcept>

#include "pluginparams.h"
#include "iinputparser.h"
#include "mnescaninputparser.h"

class AppInputParser : public IInputParser
{
public:
    AppInputParser();
    virtual PluginParams parseUserInput();

private:
    QString getAppName();
};

#endif // INPUTPARSER_H
