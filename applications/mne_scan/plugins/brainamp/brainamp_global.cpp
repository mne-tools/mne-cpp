#include "brainamp_global.h"

const char* BRAINAMPPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* BRAINAMPPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* BRAINAMPPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
