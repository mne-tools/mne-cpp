clear all;
close all;
clc;

addpath('D:\Users\Christoph\Documents\GitHub\mne-matlab\matlab');


%% Connection Information
mne_rt_server_ip               =   'localhost';%'172.21.16.63';
mne_rt_server_command_port     =    22208;
mne_rt_server_fiff_stream_port =    22209;

%%
% global MNE_RT;
% if isempty(MNE_RT)
%     MNE_RT = mne_rt_define_constants();
% end


% %%
% cmd_client = mne_rt_cmd_client();
% cmd_client.init(mne_rt_server_ip, mne_rt_server_command_port);
% 
% tag = cmd_client.read_tag()

%%
data_client = mne_rt_data_client();
data_client.init(mne_rt_server_ip, mne_rt_server_fiff_stream_port);

% tag = data_client.read_tag();
% data_client.get_client_list();
data_client.set_client_name('Test');