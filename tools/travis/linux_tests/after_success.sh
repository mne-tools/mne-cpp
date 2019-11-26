#!/bin/bash
#set -ev
set -e

# Clone MNE-CPP test data
git clone https://github.com/mne-tools/mne-cpp-test-data.git ./bin/mne-cpp-test-data

# Set Environment variable
MNECPP_ROOT=$(pwd)

# Tests to run - TODO: find required tests automatically with grep
tests=( test_fiff_rwr test_dipole_fit test_fiff_mne_types_io test_fiff_cov test_fiff_digitizer test_mne_msh_display_surface_set test_geometryinfo test_interpolation test_spectral_connectivity test_mne_forward_solution)

for test in ${tests[*]};
do
    echo ">> Starting $test"
	./bin/$test

	# Find all .cpp files, cd to their folder and run gcov
	find ./libraries -type f -name "*.cpp" -execdir gcov {} \; > /dev/null

	# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash) use python codecov
	# Do this for every test run since codecov is able to process different uploads and will merge them as soon as the Travis job is done
	codecov
	#codecov > /dev/null

    echo "<< Finished $test"
done
