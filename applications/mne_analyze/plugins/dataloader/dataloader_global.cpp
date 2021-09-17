#include "dataloader_global.h"

const char* DATALOADERPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* DATALOADERPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* DATALOADERPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
