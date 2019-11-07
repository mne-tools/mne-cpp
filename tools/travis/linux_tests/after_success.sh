#!/bin/bash
#set -ev
set -e

# Clone MNE-CPP test data
git clone https://github.com/mne-tools/mne-cpp-test-data.git mne-cpp-test-data

# Set Environment variable
MNECPP_ROOT=$(pwd)

# Tests to run - TODO: find required tests automatically with grep
tests=( test_codecov test_fiff_rwr test_dipole_fit test_fiff_mne_types_io test_fiff_cov test_fiff_digitizer test_mne_msh_display_surface_set test_geometryinfo test_interpolation test_spectral_connectivity test_mne_forward_solution)

for test in ${tests[*]};
do
    echo ">> Starting $test"	
	./bin/$test
	find . -name "*.cpp" -exec gcov {} \; > /dev/null
	#find . -name "*.cpp" -exec gcov -p -s ${PWD} {} \; > /dev/null
	codecov
	#codecov > /dev/null
    echo "<< Finished $test"
done

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
#codecov
