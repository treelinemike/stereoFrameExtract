
% remember to close HyperDecks
doCloseHyperDecks = 1;

% % create TCPIP objects for each Hyper Deck
% % TODO: this shouldn't be hard coded!!
% hyperDeckLeft = tcpip('192.168.10.50',9993);
% hyperDeckRight = tcpip('192.168.10.60',9993);
% isiKinstreamSocket = tcpip('192.168.10.70',9993);

% Addresses for Left and Right Hyperdecks
% TODO: this shouldn't be hard coded!!
% Set PC to 192.168.10.10 (or something similar, netmask 255.255.255.0)
hyperDeckLeftIP = '192.168.10.50';
hyperDeckRightIP = '192.168.10.60';

% Send network ping to Hyperdecks to make sure we'll be able to connect
pingError = 0;
if( isAlive(hyperDeckLeftIP,100) == 0 )
    warning('Cannot ping LEFT Hyperdeck!');
    pingError = 1;
else
    disp('Left Hyperdeck ping successful.');
end
if( isAlive(hyperDeckRightIP,100) == 0 )
    warning('Cannot ping RIGHT Hyperdeck!');
    pingError = 1;
else
    disp('Right Hyperdeck ping successful.');
end

% exit if we got a network ping error
if(pingError)
    set(handles.capturenote,'Enable','on');
    set(handles.startcap,'Enable','on');
    set(handles.startcapvid,'Enable','on');
    set(handles.singlecap,'Enable','on');
    set(handles.stopcap,'Enable','off');
    set(handles.disconnectbutton,'Enable','on');
    drawnow;
    return;
end

% create TCPIP objects for each Hyper Deck
hyperDeckLeft = tcpip(hyperDeckLeftIP,9993);
hyperDeckRight = tcpip(hyperDeckRightIP,9993);

% open both hyperdeck TCPIP channels
fopen(hyperDeckLeft);
fopen(hyperDeckRight);
if(~strcmp(hyperDeckLeft.status,'open'))
    error('Error opening LEFT HyperDeck TCP/IP channel.');
end
if(~strcmp(hyperDeckRight.status,'open'))
    error('Error opening RIGHT HyperDeck TCP/IP channel.');
end

% flush input and output buffers
flushinput(hyperDeckLeft);
flushoutput(hyperDeckLeft);
flushinput(hyperDeckRight);
flushoutput(hyperDeckRight);

% send software ping to each Hyperdeck
fprintf(hyperDeckLeft,'ping\n');
fprintf(hyperDeckRight,'ping\n');
respL = fgets(hyperDeckLeft);
respR = fgets(hyperDeckRight);
errorVec = zeros(1,2);
if(isempty(respL) || ~strcmp(respL(1:6),'200 ok'))
    errorVec(1) = 1;
end
if(isempty(respR) || ~strcmp(respR(1:6),'200 ok'))
    errorVec(2) = 1;
end
if(prod(errorVec))
    error('Software ping to Hyperdeck(s) failed!');
else
    disp('Software ping to Hyperdecks successful.');
end

% make sure REMOTE is enabled on each Hyperdeck
fprintf(hyperDeckLeft,'remote: enable: true\n');
fprintf(hyperDeckRight,'remote: enable: true\n');
respL = fgets(hyperDeckLeft);
respR = fgets(hyperDeckRight);
disp(['Remote enable LEFT: ' strtrim(char(respL))]);
disp(['Remote enable RIGHT: ' strtrim(char(respR))]);

fprintf(hyperDeckLeft,'slot select: slot id: 1\n');
fprintf(hyperDeckRight,'slot select: slot id: 1\n');
respL = fgets(hyperDeckLeft);
respR = fgets(hyperDeckRight);
disp(['Slot 1 select LEFT: ' strtrim(char(respL))]);
disp(['Slot 1 select RIGHT: ' strtrim(char(respR))]);

% start recording on both Hyper Decks
% Note: \n seems to work in MATLAB on windows, need \r\n in python
fprintf(hyperDeckLeft,'record\n');
fprintf(hyperDeckRight,'record\n');
% fprintf(hyperDeckLeft,'stop\n');
% fprintf(hyperDeckRight,'stop\n');

% get Hyperdeck response to record command
% TODO: This will add a small delay, find a workaround
respL = fgets(hyperDeckLeft);
respR = fgets(hyperDeckRight);
disp(['Record LEFT: ' strtrim(char(respL))]);
disp(['Record RIGHT: ' strtrim(char(respR))]);
% 
% % open both hyperdeck TCPIP channels
% fopen(hyperDeckLeft);
% fopen(hyperDeckRight);
% % fopen(isiKinstreamSocket);
% if(~strcmp(hyperDeckLeft.status,'open'))
%     error('Error opening LEFT HyperDeck TCP/IP channel.');
% end
% if(~strcmp(hyperDeckRight.status,'open'))
%     error('Error opening RIGHT HyperDeck TCP/IP channel.');
% end
% % if(~strcmp(isiKinstreamSocket.status,'open'))
% %     error('Error opening TCP/IP channel to ISI Kinematics Machine.');
% % end
% 
% % flush input and output buffers
% flushinput(hyperDeckLeft);
% flushoutput(hyperDeckLeft);
% flushinput(hyperDeckRight);
% flushoutput(hyperDeckRight);
% % flushinput(isiKinstreamSocket);
% % flushoutput(isiKinstreamSocket);
% 
% % start recording on both Hyper Decks
% % Note: \n seems to work in MATLAB on windows, need \r\n in python
% fprintf(hyperDeckLeft,'record\n');
% fprintf(hyperDeckRight,'record\n');
% % fprintf(isiKinstreamSocket,'record'); % NOTE: NO NEWLINE ON THIS ONE!
% % fprintf(hyperDeckLeft,'stop\n');
% % fprintf(hyperDeckRight,'stop\n');
% % fscanf(hyperDeckLeft,'%s')
% % fscanf(hyperDeckRight,'%s')
% 
% 
fclose(hyperDeckLeft);
fclose(hyperDeckRight);
% % fclose(isiKinstreamSocket);


