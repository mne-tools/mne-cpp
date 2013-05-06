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

    Build and install Qt5.

It’s really recommended to always download the source code of Qt5 and compile it from scratch, to make sure everything is compatible.

    [Download](http://releases.qt-project.org/qt5/5.0.1/single/qt-everywhere-opensource-src-5.0.1.tar.gz) and extract Qt5 

To compile use the following steps:

    ./configure -debug-and-release -opensource -opengl desktop -prefix /path/to/your/Qt/Qt5.0.1
    make
    make install

For convinience download and extract [Qt Creator](http://download.qt-project.org/official_releases/qtcreator/2.7/2.7.0/qt-creator-2.7.0-src.tar.gz)
To compile the qt creator do the following

    qmake –recursive
    make
    make install INSTALL_ROOT=/path/to/your/Qt/qt-creator-2.6.2

To use the mne-cpp 3D libraries download and compile also the qt3d module.

    Clone the [qt3d git repo](http://qt.gitorious.org/qt/qt3d)

To compile qt3d it’s highly recommended to make use of qt creator, since it supports shadow builds. This prevents the source folder from being messed up.

    First open qt creator make sure Tools->Options->Qt Versions/Kits are setup correctly (Qt 5.0.1)
    Then open qt3d in qt creator. After configuring the shadow build, make sure release mode is selected. Then compile it.
    After compilation, go to the shadow build directory open a terminal and type make install. Qt3d now installs itself to the Qt5.0.1 folder

Tip: Windows users should just use nmake of Visual Studio instead of make.

Now you're ready to build the whole mne-cpp librarie by doing the following steps:

Generate the Makefiles using qmake:

    qmake -recursive

Make the libraries and examples by running:

    make

Tip: It's convinient to use the Qt Creator of the [QtSDK](http://qt-project.org/downloads) to build the libraries.
Don't forget to add mne-cpp documentation to Qt Creator (Tools->Options->Help->Documentation->Add...). You'll find the qt creator documentation under: /doc/qt-creator_doc/mne-cpp.qch


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
