#include "appinputparser.h"

AppInputParser::AppInputParser()
{
}

PluginParams AppInputParser::parseUserInput() {
    out << "Creating a new MNE Plugin..." << endl;
    const QString appName = getAppName();
    if (appName == "scan") {
        return MNEScanInputParser().parseUserInput();
    }

    throw std::invalid_argument("Fatal Error: Invalid MNE application type: " + appName.toStdString());
}

QString AppInputParser::getAppName() {
    QStringList valid = {"scan"};
    out << "What application would you like to create a plugin for?" << endl;
    showOptions(valid);
    return checkInput(valid);
}


