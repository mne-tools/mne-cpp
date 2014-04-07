MNE-CPP
=========

MNE cross-platform standalone MEG/EEG (real-time) applications using [Qt5](http://qt-project.org/downloads)'s and [Eigen](http://eigen.tuxfamily.org)'s C++ libraries

Getting the latest code
=========================

To get the latest code using git, simply type:

    git clone git://github.com/mne-tools/mne-cpp.git

If you don't have git installed, you can download a zip or tarball
of the latest code: https://github.com/mne-tools/mne-cpp

Requirements
==========

**Windows**

\>= MSVC2010

**Linux**

\>= gcc 4.5

**MacOSX**

\>= Clang 3.1

Dependencies
============

[Qt5](http://qt-project.org/downloads)


Building
========

To build the MNE-CPP libraries [Qt5](http://qt-project.org/downloads) and a platform specific C++ compiler are required.

To build MNE-CPP follow the wiki-guide:
* [1. Build Qt5](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5): This page describes how to build Qt5 for desktop platforms [Windows](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#windows)/[Linux](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#linux)/[Mac OS X](https://github.com/mne-tools/mne-cpp/wiki/1.-Build-Qt5#mac-os-x). How to build Qt5 on embedded plaforms (DirectFB, EGLFS, KMS, Wayland, Windows embedded, QNX, VxWorks, INTEGRITY) or mobile platforms (Android, iOS, Windows 8 (WinRT), BlackBerry 10) you'll find [here](http://qt-project.org/doc/qt-5.0/qtdoc/platform-details.html).

* (optional) [2. Install Qt Creator](https://github.com/mne-tools/mne-cpp/wiki/2.-Install-Qt-Creator): For convinience of the further processing it's recommended to install Qt Creator at this point.

* (optional) [3. Install Qt 3D](https://github.com/mne-tools/mne-cpp/wiki/3.-Install-Qt3D): To use the MNE-CPP 3D libraries download and compile also the qt3d module. This step can be skipped when its intended to build MNE-CPP for Neuromag Linux machines or no 3D support is requested.

* [4. Compile MNE-CPP](https://github.com/mne-tools/mne-cpp/wiki/4.-Compile-mne-cpp): Now you're ready to build the whole MNE-CPP library.

Mailing list
============

http://mail.nmr.mgh.harvard.edu/mailman/listinfo/mne_analysis


Workflow to contribute
=========================

To contribute to MNE-CPP, first create an account on [github](http://github.com/). Once this is done, fork the [MNE-CPP repository](http://github.com/mne-tools/mne-cpp) to have you own repository,
clone it using 'git clone' on the computers where you want to work. Make
your changes in your clone, push them to your github account, test them
on several computer, and when you are happy with them, send a pull
request to the main repository.


Release
==========

[MNE1.0 release](https://github.com/mne-tools/mne-cpp/wiki/MNE-1.0-release)



Licensing
----------

MNE-CPP is **BSD-licenced** (3 clause):

    This software is OSI Certified Open Source Software.
    OSI Certified is a certification mark of the Open Source Initiative.

    Copyright (C) 2012-2014, authors of MNE-CPP.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the names of MNE-CPP authors nor the names of any
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
