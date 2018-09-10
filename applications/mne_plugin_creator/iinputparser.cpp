#include "iinputparser.h"

IInputParser::IInputParser()
{

}

QString IInputParser::getPluginName() {
    out << "Enter a class name to be used for your new plugin (Use CamelCase):" << endl;
    return validateArbitraryInput();
}

QString IInputParser::getNamespace() {
    out << "Enter a namespace for your new plugin to reside in:" << endl;
    return validateArbitraryInput();
}

QString IInputParser::getAuthorName() {
    out << "Enter your (the author's) name. It will be inserted into the license." << endl;
    return in.readLine();
}

QString IInputParser::getAuthorEmail() {
    out << "Enter your (the author's) email address. It will be inserted into the license" << endl;
    return validateArbitraryInput();
}

QString IInputParser::validateFiniteOptionsInput(const QStringList &validInputs) {
    QString value = in.readLine();
    if (!validInputs.contains(value)) {
      out << "Your input, " << value << ", is invalid!" << endl;;
      showOptions(validInputs);
      return validateFiniteOptionsInput(validInputs);
    }
    return value;
}

QString IInputParser::validateArbitraryInput() {
    QString value = in.readLine();
    if (value.contains(" ")) {
        out << "Value may not contain any spaces!";
        return validateArbitraryInput();
    }
    return value;
}

void IInputParser::showOptions(const QStringList &validInputs) {
    out << "Valid options:" <<  endl;
    for (QString val : validInputs) {
        out << '\t' << val << endl;
    }
}
