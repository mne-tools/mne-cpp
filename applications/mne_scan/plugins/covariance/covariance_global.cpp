#include "covariance_global.h"

const char* COVARIANCEPLUGIN::buildDateTime(){ return BUILDINFO::dateTime();};

const char* COVARIANCEPLUGIN::buildHash(){ return BUILDINFO::gitHash();};

const char* COVARIANCEPLUGIN::buildHashLong(){ return BUILDINFO::gitHashLong();};
