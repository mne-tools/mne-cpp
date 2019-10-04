:: Go to bin directory
cd bin

:: Clone MNE-CPP test data
call git clone https://github.com/mne-tools/mne-cpp-test-data.git mne-cpp-test-data

curl https://www.dropbox.com/s/464j97jbaef7q3n/sample-5120-5120-5120-bem-sol.fif?dl=1 -L -o sample-5120-5120-5120-bem-sol.fif

curl https://www.dropbox.com/s/tkrl3p1kifbzjo1/sample-5120-5120-5120-bem.fif?dl=1 -L -o sample-5120-5120-5120-bem.fif

:: Go back to mne-cpp root
cd ..
