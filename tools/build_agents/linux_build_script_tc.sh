# teamcity build agent script
# commandline with parameters
# 

qmake -r
make -j4

# user package
tar cfvz binaries_only_linux.tar.gz bin lib

# development package
tar cfvz binaries_all_linux.tar.gz *
