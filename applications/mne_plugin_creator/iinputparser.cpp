#include "iinputparser.h"

IInputParser::IInputParser()
{

}
QString IInputParser::checkInput(const QStringList &validInputs) {
    QString value = in.readLine();
    if (validInputs.contains(value)) {
        return value;
    }
    out << "Your input, " << value << ", is invalid!" << endl;;
    showOptions(validInputs);
    return checkInput(validInputs);
}

void IInputParser::showOptions(const QStringList &validInputs) {
    out << "Valid options:" <<  endl;
    for (QString val : validInputs) {
        out << '\t' << val << endl;
    }
}
