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

	# run the test binary from the ./bin subdir (test binary is same name as subdir)
	# after test run, have *.gcda file under test_dir AND multiple *.gcda files 
	# under every built library that the test called
	./bin/$test

	# run this script from under each ./mne-cpp/testframes(<test name>/. subdir
    cd "./testframes/$test"

	# test=$1
	test_dir=${PWD}
	test=`basename ${test_dir}`
	# parent of ./<test_name> subdir
	test_base=`dirname ${PWD}`
	sandbox_base=`dirname ${test_base}`

	echo ">> Running collect.sh for $test"	

	# do the build with gcov options under ./<test_name> from git checkout
	#(cd ${test_dir} && qmake ${test}.pro -r MNECPP_CONFIG+=withCodeCov)
	#(cd ${test_dir} && make clean)
	# maybe add these file types to the QMAKE_CLEAN macro in the .pro file
	#(cd ${test_dir} && m -f *.gcno *.gcda *.moc)
	#(cd ${test_dir} && make -j4)
	# after compilation, have *.o, *.moc, *.gcno under test_dir

	# create the subdir to hold files for gcov run of this tests unique paths thru the source code
	gcov_test_dir=gcoverage_${test}
	(cd ${sandbox_base} && rm -rf ${gcov_test_dir})
	(cd ${sandbox_base} && mkdir -p ${gcov_test_dir})

	# copy the built contents and generated files for the current test to the top of the gcoverage subdir
	(cd ${sandbox_base}/testframes/${test} && tar cpf - *) | (cd ${sandbox_base}/${gcov_test_dir} && tar xpf -)

	# copy in the library files (even though they may not all be linked against by this test)
	# to the top of the gcoverage subdir AND also preserve the directory structure beneath that
	# (with duplicate files)
	for library in communication connectivity deep disp disp3D fiff fs fwd inverse mne rtprocessing utils
	do
	   (cd ${sandbox_base}/libraries/$library && tar cpf - *) | (cd ${sandbox_base}/${gcov_test_dir} && tar xpf -)
	   (cd ${sandbox_base}/libraries && tar cpf - ${library}) | (cd ${sandbox_base}/${gcov_test_dir} && tar xpf -)
	done

	# remove any *.o files but need *.cpp, *.h, *.moc, *.gcno, *.gcda)
	(cd ${sandbox_base}/${gcov_test_dir} && find . -name "*.o" -exec rm -f {} \;)

	# set the environment for the gcov command
	GCOV_DIR=${sandbox_base}/${gcov_test_dir}
	export GCOV_PREFIX=${GCOV_DIR}
	export GCOV_PREFIX_STRIP=6

	# run the gcov command on all *.cpp files, preserve paths and give common path base and save to log
	(cd ${GCOV_DIR} && rm -f ../gcoverage.${test}.log && find . -name "*.cpp" -exec gcov -p -s ${GCOV_DIR} {} \; > /dev/null)

#	(cd ${GCOV_DIR} && rm -f ../gcoverage.${test}.log && find . -name "*.cpp" -exec gcov -p -s ${GCOV_DIR} {} \; | tee -a ../gcoverage.${test}.log)

	# grep/sed the gcov log output for quick summary of all # non header files with some code executed
	# .. cat the log file and ...
	#
	# remove lines with "Removing"
	# remove lines with "Creating"
	# join together every other line to make the single line "File ... had <some stat> executed"
	# ... now from each single line entry per file ...
	# remove file entries that had "No executable" lines of code
	# remove file entries that executed "0.00%" of their code
	# remove file entries for *.h files
	# remove "File" from the beginning of line
	# sort the output and remove any duplicate lines

	echo "**** ${test_base}/${test} ****"

	#(cd ${GCOV_DIR}/.. && cat gcoverage.${test}.log) | grep -v ".*Removing.*" | grep -v "^Creating .*" | grep -v "^$" | sed 'N;s/\n/ had /' | grep -v ".*No executable.*" | grep -v "^.*0.00\%" | grep -v "^.*\.h.*" | sed 's;^File ;;' | sort | uniq

    #./collect.sh
    cd $MNECPP_ROOT

    # Run Annotated Code
    #./bin/$test
    #cd "./testframes/$test"
    # Analyze Code Coverage with gcov
    #gcov "./$test.cpp" -r
    #cd $MNECPP_ROOT
    echo "<< Finished $test"
done

# Report code coverage; instead of "bash <(curl -s https://codecov.io/bash)" use python "codecov"
codecov
