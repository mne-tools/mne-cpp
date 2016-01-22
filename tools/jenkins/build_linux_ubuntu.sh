# startdir = mne-cpp/tools/build_agents/
# %0 Build Number
arg0=%0

echo Starting MNE-CPP Linux Ubuntu Build %arg0%



# startdir = mne-cpp/tools/build_agents/
cd ../..
qmake -r
make -j4

# user package
tar cfvz binaries_only_linux.tar.gz bin lib

# development package
tar cfvz binaries_all_linux.tar.gz *
