#==============================================================================================================
#
# @file     mne-cpp.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh, Lorenz Esch. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds all libraries and examples of the mne-cpp project.
#
#==============================================================================================================

include(mne-cpp.pri)

# Check versions
!minQtVersion(5, 10, 0) {
    error("You are trying to build with Qt version $${QT_VERSION}. However, the minimal Qt version to build MNE-CPP is 5.10.0.")
}

# Build static version if wasm flag was defined
contains(MNECPP_CONFIG, wasm) {
    message("The wasm flag was detected. Building static version of MNE-CPP. Disable QOpenGLWidget support.")
    MNECPP_CONFIG += static noQOpenGLWidget
}

contains(MNECPP_CONFIG, static) {
    message("The static flag was detected. Building static version of MNE-CPP.")
}

# Do not support QOpenGLWidget support on macx because signal backgrounds are not plotted correctly (tested on Qt 5.15.0 and Qt 5.15.1)
macx:minQtVersion(5, 15, 0) {
    message("Excluding QOpenGLWidget on MacOS for Qt version greater than 5.15.0")
    MNECPP_CONFIG += noQOpenGLWidget
}

TEMPLATE = subdirs

SUBDIRS += \
    libraries \

!contains(MNECPP_CONFIG, noApplications) {
    SUBDIRS += applications
}

!contains(MNECPP_CONFIG, noExamples) {
    SUBDIRS += examples
}

!contains(MNECPP_CONFIG, noTests) {
    SUBDIRS += testframes
}

# Overwrite SUBDIRS if wasm flag was defined
contains(MNECPP_CONFIG, wasm) {
    SUBDIRS = \
        libraries \
        applications
}

# Specify library dependencies
libraries.depends =
applications.depends = libraries
examples.depends = libraries
testframes.depends = libraries

