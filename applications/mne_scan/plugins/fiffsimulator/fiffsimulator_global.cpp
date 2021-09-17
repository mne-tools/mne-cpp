#include "fiffsimulator_global.h"

const char* FIFFSIMULATORPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* FIFFSIMULATORPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* FIFFSIMULATORPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
