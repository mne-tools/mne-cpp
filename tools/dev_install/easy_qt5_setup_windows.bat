cd C:\
mkdir Qt\depot
cd Qt\depot
git clone https://git.gitorious.org/qt/qt5.git qt5
cd qt5
git checkout stable
git clone https://git.gitorious.org/qt/qt3d.git qt3d
perl init-repository --no-webkit
configure -platform win32-msvc2013 -nomake examples -opensource -opengl desktop -mp -prefix "C:\Qt\Qt5.3.1-opengl" -confirm-license
nmake
nmake install
nmake docs
nmake install_docs
cd C:\
mkdir Git
cd Git
git clone https://github.com/mne-tools/mne-cpp.git mne-cpp