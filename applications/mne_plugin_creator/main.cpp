#include <iostream>

#include "mnescanplugincreator.h"
#include "appinputparser.h"
#include "pluginparams.h"

int main() {
  try {
    PluginParams params = AppInputParser().parseUserInput();
    MNEScanPluginCreator creator;
    creator.createPlugin(params);
    std::cout << "Congratulations! You're new plugin is ready to go!" << std::endl;
    return 0;
  } catch (std::exception &e) {
    std::cout << "Error while creating plugin: " << std::endl << e.what() << std::endl;
    return 1;
  }
}
