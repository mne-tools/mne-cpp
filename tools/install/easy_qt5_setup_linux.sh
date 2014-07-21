mkdir Qt
cd Qt
mkdir depot
cd depot
git clone https://git.gitorious.org/qt/qt5.git qt5
cd qt5
git checkout stable
git clone https://git.gitorious.org/qt/qt3d.git
perl init-repository --no-webkit
./configure -nomake examples -opensource -opengl desktop -prefix ~/Qt/Qt5.3.1-opengl -confirm-license
make -j8
make install
make docs -j8
make install_docs
cd ~
mkdir Git
cd Git
git clone https://github.com/mne-tools/mne-cpp.git mne-cpp
