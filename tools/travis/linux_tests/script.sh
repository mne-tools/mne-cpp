#!/bin/bash
#set -ev

# QA: static code analysis with CppCheck
cppcheck --enable=all -f -q -i./include/3rdParty/eigen3 -i/opt/qt510/include ./MNE ./applications ./testframes ./examples

# Configure with dynamic code analysis
qmake -r MNECPP_CONFIG+=withCodeCov MNECPP_CONFIG+=noExamples MNECPP_CONFIG+=noApplications

# Build
make -j2