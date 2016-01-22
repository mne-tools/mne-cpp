# ###startdir### %WORKSPACE%/mne-cpp/..
# %0 Build Number
# arg0=%0

echo Starting MNE-CPP Linux Ubuntu Build

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

qmake ../mne-cpp/mne-cpp.pro -r
make clean
make -j4

# user package
# tar cfvz binaries_only_linux.tar.gz bin lib

# development package
# tar cfvz binaries_all_linux.tar.gz *
