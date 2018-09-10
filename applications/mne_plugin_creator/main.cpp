#include <iostream>

#include "mnescanplugincreator.h"
#include "appinputparser.h"
#include "pluginparams.h"

int main() {
  try {
    AppInputParser parser;
    PluginParams params = parser.parseUserInput();
    MNEScanPluginCreator creator;
    creator.createPlugin(params);
    return 0;
  } catch (std::exception &e) {
    std::cout << "Error while creating plugin: " << std::endl << e.what() << std::endl;
    return 1;
  }
}
