clear all;
close all;
clc;

addpath('../../../mne-matlab/matlab');

%% connection information
mne_rt_server_ip            =	'localhost';%'172.21.16.63';
mne_rt_server_commandPort	=	48332;
mne_rt_server_fiffDataPort	=	48333;

%% create command client
t_cmdClient = mne_rt_cmd_client(mne_rt_server_ip, mne_rt_server_commandPort);

%% create data client
t_dataClient = mne_rt_data_client(mne_rt_server_ip, mne_rt_server_fiffDataPort);

%% set data client alias -> for convinience (optional)
t_dataClient.setClientAlias('mne_ex_matlab');

%% example commands
clistInfo = t_cmdClient.getClientList();
fprintf('### Client List ###\n%s',clistInfo);

%% read meas info
%AliasOrId = t_dataClient.getClientId();
AliasOrId = 'mne_ex_matlab';
t_cmdClient.requestMeasInfo(AliasOrId);
pause(0.5); % replace that by end block meas
info = t_dataClient.readInfo();

%% close the sockets
t_cmdClient.close();
t_dataClient.close();