clear all;
close all;
clc;

addpath('../../../mne-matlab/matlab');

%% connection information
mne_rt_server_ip            =	'172.21.16.63';%'localhost';%'172.21.16.63';
mne_rt_server_commandPort	=	59827;
mne_rt_server_fiffDataPort	=	55029;

%% create command client
t_cmdClient = mne_rt_cmd_client(mne_rt_server_ip, mne_rt_server_commandPort);

%% create data client
t_dataClient = mne_rt_data_client(mne_rt_server_ip, mne_rt_server_fiffDataPort);

%% set data client alias -> for convinience (optional)
t_dataClient.setClientAlias('mne_ex_matlab'); % used in option 2 later on

%% example commands
t_clistInfo = t_cmdClient.getClientList();
fprintf('### Client List ###\n%s',t_clistInfo);

%% read meas info
% Option 1
t_aliasOrId = t_dataClient.getClientId();
% Option 2
%t_aliasOrId = 'mne_ex_matlab';
t_cmdClient.requestMeasInfo(t_aliasOrId);
t_measInfo = t_dataClient.readInfo();

%% close the sockets
t_cmdClient.close();
t_dataClient.close();