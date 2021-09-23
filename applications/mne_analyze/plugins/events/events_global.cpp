#include "events_global.h"

const char* EVENTSPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* EVENTSPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* EVENTSPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};

