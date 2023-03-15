#==============================================================================================================
#
# @file     windows.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
# @since    0.1.0
# @date     January, 2016
#
# @section  LICENSE
#
# Copyright (C) 2016, Christoph Dinh. All rights reserved.
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
# @brief    This project file builds the windows installer
#
#==============================================================================================================

include (../../../mne-cpp.pri)

TEMPLATE = aux

INSTALLER_TARGET = mne-cpp-windows-x86_64

OTHER_FILES = \
    readme.txt \
    config/config.xml \
    packages/org.mne_cpp.suite/meta/package.xml \
    packages/org.mne_cpp.suite/meta/license_mne.txt \
    packages/org.mne_cpp.suite/meta/installscript.js \
    packages/org.mne_cpp.suite.mne_scan/meta/package.xml \
    packages/org.mne_cpp.suite.mne_scan/meta/license_mne-x.txt \
    packages/org.mne_cpp.suite.mne_scan/meta/installscript.qs \
    packages/org.mne_cpp.suite.mne_browse/meta/package.xml \
    packages/org.mne_cpp.suite.mne_browse/meta/license_mne_browse.txt \
    packages/org.mne_cpp.suite.mne_browse/meta/installscript.qs \
    packages/org.mne_cpp.suite.mne_analyze/meta/package.xml \
    packages/org.mne_cpp.suite.mne_analyze/meta/license_mne_analyze.txt \
    packages/org.mne_cpp.suite.mne_analyze/meta/installscript.qs

FORMS += \
    packages/org.mne_cpp.suite/meta/vcredistcheckboxform.ui

# Offline installer configuration
INPUT = $$PWD/config/config.xml $$PWD/packages
mne-cpp_installer.input = INPUT
mne-cpp_installer.output = $$INSTALLER_TARGET
mne-cpp_installer.commands = binarycreator --offline-only -c $$PWD/config/config.xml -p $$PWD/packages -r $$PWD/resources/additional.qrc ${QMAKE_FILE_OUT}
mne-cpp_installer.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += mne-cpp_installer

# Resource
RESOURCES += \
    resources/additional.qrc
