#ifndef IINPUTPARSER_H
#define IINPUTPARSER_H

#include <QTextStream>

#include <stdexcept>

#include "pluginparams.h"

class IInputParser
{
public:
    IInputParser();

protected:
    virtual PluginParams parseUserInput() = 0;

    QString checkInput(const QStringList &validInputs);
    void showOptions(const QStringList &validInputs);

    QTextStream in;
    QTextStream out;
};

#endif // IINPUTPARSER_H
