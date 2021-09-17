#include "sourcelocalization_global.h"

const char* SOURCELOCALIZATIONPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* SOURCELOCALIZATIONPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* SOURCELOCALIZATIONPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
