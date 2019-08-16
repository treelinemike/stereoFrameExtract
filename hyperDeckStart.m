
% remember to close HyperDecks
doCloseHyperDecks = 1;

% create TCPIP objects for each Hyper Deck
% TODO: this shouldn't be hard coded!!
hyperDeckLeft = tcpip('192.168.10.50',9993);
hyperDeckRight = tcpip('192.168.10.60',9993);

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

% start recording on both Hyper Decks
% Note: \n seems to work in MATLAB on windows, need \r\n in python
fprintf(hyperDeckLeft,'record\n');
fprintf(hyperDeckRight,'record\n');
% fprintf(hyperDeckLeft,'stop\n');
% fprintf(hyperDeckRight,'stop\n');
% fscanf(hyperDeckLeft,'%s')
% fscanf(hyperDeckRight,'%s')


fclose(hyperDeckLeft);
fclose(hyperDeckRight);
