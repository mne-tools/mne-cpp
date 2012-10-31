clear all;
close all;
clc;

addpath('D:\Users\Christoph\Documents\GitHub\mne-matlab\matlab');


%% Connection Information
mne_rt_server_ip               =   'localhost';%'172.21.16.63';
mne_rt_server_command_port     =    27794;
mne_rt_server_fiff_stream_port =    27795;

%%
global MNE_RT;
if isempty(MNE_RT)
    MNE_RT = mne_rt_define_constants();
end


% %%
% command_client = mne_rt_client();
% command_client.init_connection(mne_rt_server_ip, mne_rt_server_command_port);
% 
% [blocksize, msg] = command_client.read()

%%
fiff_stream_client = mne_rt_client();
fiff_stream_client.init_connection(mne_rt_server_ip, mne_rt_server_fiff_stream_port);

tag = fiff_stream_client.read_tag_stream();