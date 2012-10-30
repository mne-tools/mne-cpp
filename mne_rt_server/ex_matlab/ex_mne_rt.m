clear all;
close all;
clc;

%% Connection Information
mne_rt_server_ip = 'localhost';
mne_rt_server_command_port = 22686;
mne_rt_server_fiff_stream_port = 22687;

%%
command_client = mne_rt_client();
command_client.init_connection(mne_rt_server_ip, mne_rt_server_command_port);

[blocksize, msg] = command_client.read()

%%
fiff_stream_client = mne_rt_client();
fiff_stream_client.init_connection(mne_rt_server_ip, mne_rt_server_fiff_stream_port);

[blocksize, msg] = fiff_stream_client.read()