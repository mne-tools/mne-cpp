#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include <QTextStream>

#include <stdexcept>

#include "pluginparams.h"
#include "mnescaninputparser.h"

class AppInputParser
{
public:
    AppInputParser();
    virtual PluginParams parseUserInput();

protected:
    QString checkInput(const QStringList &validInputs);
    void showOptions(const QStringList &validInputs);

    QTextStream in;
    QTextStream out;

private:
    QString getAppName();
};

#endif // INPUTPARSER_H
