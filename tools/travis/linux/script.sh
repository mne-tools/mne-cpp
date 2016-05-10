#!/bin/bash
set -ev

# Configure and build
qmake -r
make -j2