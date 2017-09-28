#!/bin/bash
#set -ev

# QA: static code analysis with CppCheck
cppcheck --enable=all -f -q -i./include/3rdParty/eigen3 -i/opt/qt59/include ./MNE ./applications ./testframes ./examples

# Configure with dynamic code analysis
qmake -r MNECPP_CONFIG+=withCodeCov

# Build
make -j2