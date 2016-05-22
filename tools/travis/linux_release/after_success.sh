#!/bin/bash
set -ev

# Create Code Coverage
./bin/test_codecov
#tbd: later on do a grep of all cpps within testframe
gcov ./testframes/test_codecov/test_codecov.cpp

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
codecov
