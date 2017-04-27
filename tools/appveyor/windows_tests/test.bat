:: Go to bin directory
cd bin

:: Array of tests to run
set tests=test_fiff_rwr test_dipole_fit test_fiff_mne_types_io test_fiff_cov test_fiff_digitizer

:: Run tests
(for %%t in (%tests%) do ( 
   call %%t
))
