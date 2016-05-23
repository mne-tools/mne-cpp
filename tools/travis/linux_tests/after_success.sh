#!/bin/bash
#set -ev
set -ev

# Clone MNE-CPP test data
git clone https://github.com/mne-tools/mne-cpp-test-data.git mne-cpp-test-data

# Set Environment variable
export LD_LIBRARY_PATH=$(pwd)/lib:$LD_LIBRARY_PATH
MNECPP_ROOT=$(pwd)

#tbd: later on do a grep of all cpps within testframe
# Create Code Coverage
./bin/test_codecov
./bin/test_fiff_rwr

ls
cd ./testframes/test_codecov
gcov ./testframes/test_codecov/test_codecov.cpp -r
ls

cd $MNECPP_ROOT
ls

cd ./testframes/test_fiff_rwr
gcov ./test_fiff_rwr.cpp -r
ls

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
codecov
