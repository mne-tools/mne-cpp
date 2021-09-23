#include "covariance_global.h"

const char* COVARIANCEPLUGIN::buildDateTime(){ return UTILSLIB::dateTimeNow();};

const char* COVARIANCEPLUGIN::buildHash(){ return UTILSLIB::gitHash();};

const char* COVARIANCEPLUGIN::buildHashLong(){ return UTILSLIB::gitHashLong();};
