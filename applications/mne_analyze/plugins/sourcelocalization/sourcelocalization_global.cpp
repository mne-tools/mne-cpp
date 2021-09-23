#include "sourcelocalization_global.h"

const char* SOURCELOCALIZATIONPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* SOURCELOCALIZATIONPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* SOURCELOCALIZATIONPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
