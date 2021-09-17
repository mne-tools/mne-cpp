#include "fiffsimulator_global.h"

const char* FIFFSIMULATORPLUGIN::BUILD_DATETIME(){ return BUILDINFO::dateTime();};

const char* FIFFSIMULATORPLUGIN::BUILD_HASH(){ return BUILDINFO::gitHash();};

const char* FIFFSIMULATORPLUGIN::BUILD_HASH_LONG(){ return BUILDINFO::gitHashLong();};
