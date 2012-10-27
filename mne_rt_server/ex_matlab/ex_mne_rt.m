clear all;
close all;
clc;

%%
% data = client('localhost', 3000)

client = mne_matlab_client();
client.init_connection('localhost', 48662);%3000);

%%
[blocksize, msg] = client.read();

msg