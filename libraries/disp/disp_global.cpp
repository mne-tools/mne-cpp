#include "disp_global.h"

const char* DISPLIB::BUILD_TIME(){ return BUILDINFO::time();};

const char* DISPLIB::BUILD_DATE(){ return BUILDINFO::date();};
