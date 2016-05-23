#!/bin/bash
#set -ev

# Clone MNE-CPP test data
git clone https://github.com/mne-tools/mne-cpp-test-data.git mne-cpp-test-data

# Set Environment variable
export LD_LIBRARY_PATH=$(pwd)/lib:$LD_LIBRARY_PATH 

#tbd: later on do a grep of all cpps within testframe
# Create Code Coverage
./bin/test_codecov
./bin/test_fiff_rwr

gcov ./testframes/test_codecov/test_codecov.cpp -p
gcov ./testframes/test_fiff_rwr/test_fiff_rwr.cpp -p

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
codecov
