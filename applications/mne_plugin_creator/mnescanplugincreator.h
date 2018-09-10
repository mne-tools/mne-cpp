#ifndef MNESCANPLUGINCREATOR_H
#define MNESCANPLUGINCREATOR_H

#include "plugincreator.h"

class MNEScanPluginCreator : public PluginCreator
{
public:
  MNEScanPluginCreator();

protected:
  void copyTemplates(const PluginParams &params) override;
  void updateProjectFile(const PluginParams &params) override;
};

#endif // MNESCANPLUGINCREATOR_H
