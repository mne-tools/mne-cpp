#include "communication_global.h"

const char* COMMUNICATIONLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* COMMUNICATIONLIB::BUILD_DATE(){ return BUILDINFO::date();};
