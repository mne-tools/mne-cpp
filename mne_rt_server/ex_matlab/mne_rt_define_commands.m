function [ MNE_RT ] = mne_rt_define_commands()
%
%    [ FIFF ] = mne_rt_define_commands()
%
%    Defines structure containing the MNE_RT constants
%

%
%   Author : Christoph Dinh, Matti Hamalainen, MGH Martinos Center
%   License : BSD 3-clause

%
% MNE RT Communication Constants
%
MNE_RT.MNE_RT_GET_CLIENT_LIST   =   1;	% Client List at mne_rt_server
MNE_RT.MNE_RT_SET_CLIENT_NAME   =   2;	% Set Client name at mne_rt_server

return

end

