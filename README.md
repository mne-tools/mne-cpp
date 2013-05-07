mne-cpp (beta)
==================

MNE cross-platform standalone MEG/EEG (real-time) applications using [Qt5](http://qt-project.org/downloads)'s and [Eigen](http://eigen.tuxfamily.org)'s C++ libraries

Getting the latest code
=========================

To get the latest code using git, simply type:

    git clone git://github.com/mne-tools/mne-cpp.git

If you don't have git installed, you can download a zip or tarball
of the latest code: https://github.com/mne-tools/mne-cpp


Building
==========

To build the mne libraries [Qt5](http://qt-project.org/downloads) is required.

To build mne-cpp follow the wiki-guide:
* [1. Build Qt5](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5): This page describes how to build Qt5 for [Windows](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#windows)/[Linux](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#linux)/[Mac OS X](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#mac-os-x)

* (optional) [2. Install Qt Creator](https://github.com/mne-tools/mne-cpp/wiki/2.-Install-Qt-Creator): For convinience of the further processing it's recommended to install Qt Creator at this point.

* (optional) [3. Install Qt 3D](https://github.com/mne-tools/mne-cpp/wiki/3.-Install-Qt3D): To use the mne-cpp 3D libraries download and compile also the qt3d module. This step can be skipped when its intended to build mne-cpp for Neuromag Linux machines.

* [4. Compile mne-cpp](https://github.com/mne-tools/mne-cpp/wiki/4.-Compile-mne-cpp): Now you're ready to build the whole mne-cpp library

Dependencies
============

[Qt5](http://qt-project.org/downloads)


Mailing list
============

http://mail.nmr.mgh.harvard.edu/mailman/listinfo/mne_analysis


Workflow to contribute
=========================

To contribute to MNE-cpp, first create an account on [github](http://github.com/). Once this is done, fork the [mne-cpp repository](http://github.com/mne-tools/mne-cpp) to have you own repository,
clone it using 'git clone' on the computers where you want to work. Make
your changes in your clone, push them to your github account, test them
on several computer, and when you are happy with them, send a pull
request to the main repository.


Licensing
----------

MNE-cpp is **BSD-licenced** (3 clause):

    This software is OSI Certified Open Source Software.
    OSI Certified is a certification mark of the Open Source Initiative.

    Copyright (C) 2013, authors of MNE-cpp.
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
