#include "communication_global.h"

const char* COMMUNICATIONLIB::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* COMMUNICATIONLIB::buildHash(){ return UTILSLIB::gitHash();};

const char* COMMUNICATIONLIB::buildHashLong(){ return UTILSLIB::gitHashLong();};
