#include "brainflowboard_global.h"

const char* BRAINFLOWBOARDPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* BRAINFLOWBOARDPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* BRAINFLOWBOARDPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};


