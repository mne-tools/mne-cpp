#include <QCoreApplication>
#include <QDir>
#include <iostream>

#include "plugincreator.h"
#include "pluginparams.h"

using std::cout;
using std::endl;

int main() {
  cout << "Enter a name for your new plugin:" << endl;

  PluginParams params("MoppyPlugin", "SHI", "Erik Hornberger", "erik.hornberger@shi-g.com");
  PluginCreator creator(nullptr);

  try {
    creator.createPlugin(params);
    return 0;
  } catch (std::exception &e) {
    cout << "Error while creating plugin: " << endl << e.what() << endl;
    return 1;
  }
}
