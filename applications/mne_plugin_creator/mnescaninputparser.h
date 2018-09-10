#ifndef MNESCANINPUTPARSER_H
#define MNESCANINPUTPARSER_H

#include "iinputparser.h"
#include "pluginparams.h"

class MNEScanInputParser : public IInputParser {

public:
    MNEScanInputParser();
    PluginParams parseUserInput() override;

private:
    QString getSuperClassName();
};

#endif // MNESCANINPUTPARSER_H
