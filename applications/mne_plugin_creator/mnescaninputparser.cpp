#include "mnescaninputparser.h"

MNEScanInputParser::MNEScanInputParser(): IInputParser()
{

}

PluginParams MNEScanInputParser::parseUserInput() {
    const QString superclass = getSuperClassName();
    const QString pluginName = getPluginName();
    const QString nameSpace = getNamespace();
    const QString author = getAuthorName();
    const QString email = getAuthorEmail();
    return PluginParams(pluginName, superclass, nameSpace, author, email);
}

QString MNEScanInputParser::getSuperClassName() {
    QStringList valid = {"algorithm", "sensor"};
    out << "Which type of plugin would you like to create?" << endl;
    showOptions(valid);
    return validateFiniteOptionsInput(valid);
}




