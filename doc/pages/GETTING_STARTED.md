Getting started {#page_getting_started}
=======================================

We recommend using Qt Creator for development.

Getting Qt
----------

* Download the current Qt version:
    Go to the Qt download section and download the [Qt installer](https://www.qt.io/download-qt-installer?hsCtaTracking=9f6a2170-a938-42df-a8e2-a9f0b1d6cdce%7C6cb0de4f-9bb5-4778-ab02-bfb62735f3e5). Qt 5.10.0 or higher is needed in order to have full Qt3D support. 

    Windows users: Make sure NOT to download the WinRt or UWP version.

    Select the appropriate version based on your compiler and development platform.

    For example if you are using the Microsoft Visual Studio 2015 Compiler on a Windows 32bit system, select the "MSVC 2015 32-bit" version.

* Install the Qt version with the minimum of the following features (uncheck all other boxes):

    - Qt/5.10.0/Pre-compiled Qt (i.e. Qt 5.10.0/MSVC 2015 32-bit)
    - Qt/5.10.0/QtCharts
    - Qt/Tools/QtCreator

* You now have the latest Qt version installed on your local machine. Add the Qt bin folder to the environment variable:

    - Windows:

        - Right-click Computer and click Properties.
        - Click Advanced System Settings link in the left column.
        - In the Advanced window and click the Environment Variables button.
        - Add to the `PATH` variable `C:\Qt\5.10.0\msvc2015\bin` (the qt bin path may differ depending on your Qt installation).

    - Linux:

        - Add Qt bin and lib directory to the environment variables (the qt paths may differ depending on your Qt installation). 
          by adding it to your `.bashrc` file under `/home/<username>/`:
 
              $ export PATH=$PATH:~/Qt/Qt5.10.0/bin
              $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/Qt/Qt5.10.0/lib

         Note this command may differ depending on your shell (for example `$ export env` in `.tchrc`)

* In case you work on a Windows machine and do not have a compiler set up:

    - Go to the [Windows SDK download website](https://dev.windows.com/de-de/downloads/windows-8-1-sdk)
    - Download and install the Windows SDK (during the installation, make sure to also select the Debugging Tools for Windows box).
    - Open the QtCreator and select the debugger under `Tools/Options/Build & Run/Kits/[your Kit name goes here]/Debugger`.

Get the MNE-CPP source code
---------------------------

Fork MNE-CPP's main repository (mne-tools/mne-cpp) to your own GitHub account:

* Go to the GitHub website and sign into your GitHub account.
* Go to the MNE-CPP's GitHub website.
* Click on the upper right Fork button. This will copy the main repository to your own GitHub account. This is the code you can now work with, without breaking the project's main code.

Clone the forked MNE-CPP repository to your local machine:

* Open the Git command line tool (Git CMD).
* Navigate to your folder where you want to clone the repository to (Use cd to navigate to a specific path on your hard drive). Please note that the folder path should not contain any whitespaces. For example, `C:/Git/` would be a good candidate to clone the repository to.
* Type in the following command to clone the repository:

      $ git clone https://github.com/<YourGitUserName>/mne-cpp.git

* Every time you want to pull the newest changes made in the main MNE-CPP repository use the code below. Please make sure that you are on the local `master` branch when you do this:

      $ git pull https://github.com/mne-tools/mne-cpp.git master

Congrats, you now have the latest MNE-CPP source code on your remote and local machine. For a more detailed overview of how to make a pull request, we recommend checking out the [guide on the official Git website](https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project).

Compile the MNE-CPP source code
-------------------------------

* If you are working on an operating system on a "non-western" system, i.e. Japan, you might encounter problems with unicode interpretation. Please do the  following: Go to Control Panel -> Language and Region -> Management tab -> Language Settings for non-Unicode Programs -> Set to English (U.S.) -> Reboot your system.
* Go to your cloned repository folder and run the `mne-cpp/mne-cpp.pro` file with the QtCreator.
* The first time you open the mne-cpp.pro file with QtCreator you will be prompted to configure the project with an installed Qt version. Select teh appropriate Qt version, e.g. Qt MSVC 2015 32-bit and configure the project.
* In QtCreator select the Release mode in the lower left corner.
* In the Qt Creator's Projects window, right mouse click on the top level mne-cpp tree item and select Run qmake. Wait until progress bar in lower right corner turns green (this step may take some time).
* Right mouse click again and then hit Build (this step may take some time). Wait until progress bar in lower right corner turns green.
* After the build process is finished, go to the `mne-cpp/bin` folder. All applications and libraries should have been created throughout the build process.
