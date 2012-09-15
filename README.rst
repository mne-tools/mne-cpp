mne-cpp (pre-alpha)
==================

Fast MNE Qt 5 based C++ library (pre-alpha state)


Getting the latest code
=========================

To get the latest code using git, simply type::

    git clone git://github.com/mne-tools/mne-cpp.git

If you don't have git installed, you can download a zip or tarball
of the latest code: http://github.com/mne-tools/mne-python/archives/master


Building
==========

To build the mne libraries `Qt5 beta <http://releases.qt-project.org/qt5.0/beta1/>`_ is required.

    Build and install Qt5.

To build the whole mne-cpp librarie, do the following steps:

Generate the Makefiles using qmake:

    qmake -recursive

Make the libraries and examples running:

    make

Tip: It's convinient to use the Qt Creator of the `QtSDK <http://qt.nokia.com/downloads>`_ to build the libraries.


Dependencies
============

`Qt5 beta <http://releases.qt-project.org/qt5.0/beta1/>`_


Mailing list
============

http://mail.nmr.mgh.harvard.edu/mailman/listinfo/mne_analysis


Workflow to contribute
=========================

To contribute to MNE-cpp, first create an account on `github
<http://github.com/>`_. Once this is done, fork the `mne-cpp repository
<http://github.com/mne-tools/mne-cpp>`_ to have you own repository,
clone it using 'git clone' on the computers where you want to work. Make
your changes in your clone, push them to your github account, test them
on several computer, and when you are happy with them, send a pull
request to the main repository.


Licensing
----------

MNE-cpp is **BSD-licenced** (3 clause):

    This software is OSI Certified Open Source Software.
    OSI Certified is a certification mark of the Open Source Initiative.

    Copyright (c) 2011, authors of MNE-Python.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the names of MNE-Python authors nor the names of any
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    **This software is provided by the copyright holders and contributors
    "as is" and any express or implied warranties, including, but not
    limited to, the implied warranties of merchantability and fitness for
    a particular purpose are disclaimed. In no event shall the copyright
    owner or contributors be liable for any direct, indirect, incidental,
    special, exemplary, or consequential damages (including, but not
    limited to, procurement of substitute goods or services; loss of use,
    data, or profits; or business interruption) however caused and on any
    theory of liability, whether in contract, strict liability, or tort
    (including negligence or otherwise) arising in any way out of the use
    of this software, even if advised of the possibility of such
    damage.**






This code is licensed under the BSD 3-clause. See LICENSE.txt
