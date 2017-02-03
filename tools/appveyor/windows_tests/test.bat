cd bin

set tests=test_fiff_rwr test_dipole_fit test_fiff_mne_types_io

(for %%t in (%tests%) do ( 
   call %%t
))
