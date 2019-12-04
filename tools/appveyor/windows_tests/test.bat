:: Go to bin directory
cd bin

:: Array of tests to run
set tests=test_fiff_rwr test_dipole_fit test_fiff_mne_types_io test_fiff_cov test_fiff_digitizer test_mne_msh_display_surface_set test_geometryinfo test_interpolation test_spectral_connectivity test_mne_forward_solution test_rtfilter_filterdata

:: Run tests
(for %%t in (%tests%) do ( 
   call %%t
))
