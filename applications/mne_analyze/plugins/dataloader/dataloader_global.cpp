#include "dataloader_global.h"

const char* DATALOADERPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* DATALOADERPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* DATALOADERPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
