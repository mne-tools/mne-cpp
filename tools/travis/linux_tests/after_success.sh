#!/bin/bash
#set -ev
set -e

# Clone MNE-CPP test data
git clone https://github.com/mne-tools/mne-cpp-test-data.git mne-cpp-test-data

# Temporary fix to run test_forward_solution on the large sample-5120-5120-5120-bem-sol.fif file.
wget https://www.dropbox.com/s/464j97jbaef7q3n/sample-5120-5120-5120-bem-sol.fif?dl=1 -O sample-5120-5120-5120-bem-sol.fif 

wget https://www.dropbox.com/s/tkrl3p1kifbzjo1/sample-5120-5120-5120-bem.fif?dl=1 -O sample-5120-5120-5120-bem.fif 

# Set Environment variable
MNECPP_ROOT=$(pwd)

# Tests to run - TODO: find required tests automatically with grep
tests=( test_codecov test_fiff_rwr test_dipole_fit test_fiff_mne_types_io test_fiff_cov test_fiff_digitizer test_mne_msh_display_surface_set test_geometryinfo test_interpolation test_spectral_connectivity test_forward_solution)

for test in ${tests[*]};
do
    echo ">> Starting $test"
    # Run Annotated Code
    ./bin/$test
    cd "./testframes/$test"
    # Analyze Code Coverage with gcov
    gcov "./$test.cpp" -r
    cd $MNECPP_ROOT
    echo "<< Finished $test"
done

#./bin/test_codecov
#cd ./testframes/test_codecov
#gcov ./test_codecov.cpp -r
#cd $MNECPP_ROOT

#./bin/test_fiff_rwr
#cd ./testframes/test_fiff_rwr
#gcov ./test_fiff_rwr.cpp -r
#cd $MNECPP_ROOT

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
codecov
