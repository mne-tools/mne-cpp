#include "events_global.h"

const char* EVENTSLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* EVENTSLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* EVENTSLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
