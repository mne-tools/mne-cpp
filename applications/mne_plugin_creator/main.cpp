#include <iostream>

#include "plugincreator.h"
#include "mnescanplugincreator.h"
#include "pluginparams.h"

int main() {

  PluginParams params("MoppyPlugin", "SHI", "Erik Hornberger", "erik.hornberger@shi-g.com");
  MNEScanPluginCreator creator;

  try {
    creator.createPlugin(params);
    return 0;
  } catch (std::exception &e) {
    std::cout << "Error while creating plugin: " << std::endl << e.what() << std::endl;
    return 1;
  }
}
