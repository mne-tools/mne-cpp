#include "fiffsimulator_global.h"

const char* FIFFSIMULATORPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* FIFFSIMULATORPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* FIFFSIMULATORPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
