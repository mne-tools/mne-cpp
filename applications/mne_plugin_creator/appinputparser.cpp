#include "appinputparser.h"

AppInputParser::AppInputParser(): in(stdin), out(stdout)
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


QString AppInputParser::checkInput(const QStringList &validInputs) {
    QString value = in.readLine();
    if (validInputs.contains(value)) {
        return value;
    }
    out << "Your input, " << value << ", is invalid!" << endl;;
    showOptions(validInputs);
    return checkInput(validInputs);
}

void AppInputParser::showOptions(const QStringList &validInputs) {
    out << "Valid options:" <<  endl;
    for (QString val : validInputs) {
        out << '\t' << val << endl;
    }
}
