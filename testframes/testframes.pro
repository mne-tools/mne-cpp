#==============================================================================================================
#
# @file     testframes.pro
# @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
#           Felix Griesau <Felix.Griesau@tu-ilmenau.de>;
#           Gabriel B Motta <gabrielbenmotta@gmail.com>;
#           Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Ricky Tjen <ricky270@student.sgu.ac.id>;
#           Sugandha Sachdeva <sugandha.sachdeva@tu-ilmenau.de>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Lars Debor, Christoph Dinh, Ruben Doerfel, Felix Griesau, Gabriel B Motta, 
#                     Daniel Strohmeier, Lorenz Esch, Ricky Tjen, Sugandha Sachdeva. All rights reserved.
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
# @brief    This project file generates the makefile to build the unit tests.
#
#==============================================================================================================

include(../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    test_coregistration \
    test_dipole_fit \
    test_fiff_coord_trans \
    test_fiff_rwr \
    test_fiff_mne_types_io \
    test_filtering \
    test_hpiFit \
    test_hpiFit_integration \
    test_mne_forward_solution \
    test_fiff_cov \
    test_fiff_digitizer \
    test_mne_msh_display_surface_set \
    test_mne_project_to_surface \
    test_utils_circularbuffer \
    test_sensorSet

    qtHaveModule(charts) {
        SUBDIRS += \
            test_interpolation \
            test_geometryinfo \
            test_spectral_connectivity \
            test_mne_anonymize \
            test_edf2fiff_rwr
    }
