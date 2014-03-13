# startdir = mne-cpp/tools/build_agents/
cd ../..
qmake -r
make -j4

# dir = ../ mne-cpp
cd ..

# user package
tar cfvz binaries_only_unix.tar.gz mne-cpp/bin mne-cpp/lib/

# development package
tar cfvz binaries_all_unix.tar.gz mne-cpp/
