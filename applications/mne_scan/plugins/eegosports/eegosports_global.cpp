#include "eegosports_global.h"

const char* EEGOSPORTSPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* EEGOSPORTSPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* EEGOSPORTSPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
