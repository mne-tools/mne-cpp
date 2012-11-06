clear all;
close all;
clc;

addpath('../../../mne-matlab/matlab');

%% Connection Information
mne_rt_server_ip            =	'localhost';%'172.21.16.63';
mne_rt_server_commandPort	=	49534;
mne_rt_server_fiffDataPort	=	49535;

%% create command client
t_cmdClient = mne_rt_cmd_client(mne_rt_server_ip, mne_rt_server_commandPort);

%% create data client
t_dataClient = mne_rt_data_client(mne_rt_server_ip, mne_rt_server_fiffDataPort);

%% set data client alias
t_dataClient.setClientAlias('mne_ex_matlab');

%% example commands
clistInfo = t_cmdClient.getClientList();
fprintf('### Client List ###\n%s',clistInfo);



