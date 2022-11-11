% restart
close all; clear; clc;

kinematicsOnly = 0;

% Addresses for Left and Right Hyperdecks
% TODO: this shouldn't be hard coded!!
% Set PC to 192.168.10.10 (or something similar, netmask 255.255.255.0)
hyperDeckLeftIP = '192.168.10.50';
hyperDeckRightIP = '192.168.10.60';
kinematicsPCIP = '192.168.10.70';
% kinematicsPCIP = '127.0.0.1';

% Send network ping to Hyperdecks to make sure we'll be able to connect
use.hyperDeckLeft = 1;
use.hyperDeckRight = 1;
use.kinematicsPC = 0;
pingError = 0;

if(~kinematicsOnly)
    if( isAlive(hyperDeckLeftIP,100) == 0 )
        warning('Cannot directly ping LEFT HyperDeck!');
        pingError = 1;
    else
        disp('Left HyperDeck direct ping successful.');
        use.hyperDeckLeft = 1;
    end

    if( isAlive(hyperDeckRightIP,100) == 0 )
        warning('Cannot directly ping RIGHT HyperDeck!');
        pingError = 1;
    else
        disp('Right HyperDeck direct ping successful.');
        use.hyperDeckRight = 1;
    end
end

if( isAlive(kinematicsPCIP,100) == 0 )
    warning('Cannot directly ping kinematics PC!');
    pingError = 1;
else
    disp('Kinematics PC direct ping successful.');
    use.kinematicsPC = 1;
end
    
% we could do something here if we got a network ping error
% if(pingError)
%     disp('Ping error!');
% end

% create TCPIP objects for each connection
% and open TCPIP channels
if(~kinematicsOnly)
if(use.hyperDeckLeft)
    hyperDeckLeftSocket = tcpip(hyperDeckLeftIP,9993);
    fopen(hyperDeckLeftSocket);
    if(~strcmp(hyperDeckLeftSocket.status,'open'))
        error('Error opening LEFT HyperDeck TCP/IP socket.');
    end
    flushinput(hyperDeckLeftSocket);
    flushoutput(hyperDeckLeftSocket);
end

if(use.hyperDeckRight)
    hyperDeckRightSocket = tcpip(hyperDeckRightIP,9993);
    fopen(hyperDeckRightSocket);
    if(~strcmp(hyperDeckRightSocket.status,'open'))
        error('Error opening RIGHT HyperDeck TCP/IP socket.');
    end
    flushinput(hyperDeckRightSocket);
    flushoutput(hyperDeckRightSocket);
end
end

if(use.kinematicsPC)
    kinematicsPCSocket = tcpip(kinematicsPCIP,9993);
    fopen(kinematicsPCSocket);
    if(~strcmp(kinematicsPCSocket.status,'open'))
        error('Error opening kinematics PC TCP/IP socket.');
    end
    flushinput(kinematicsPCSocket);
    flushoutput(kinematicsPCSocket);
end

% send software ping to each Hyperdeck
% and configure settings
if(~kinematicsOnly)
if(use.hyperDeckLeft)
    fprintf(hyperDeckLeftSocket,'ping\n');
    respL = fgets(hyperDeckLeftSocket);
    if(isempty(respL) || ~strcmp(respL(1:6),'200 ok'))
        error('Software ping to LEFT HyperDeck failed!');
    else
        disp('Software ping to LEFT HyperDeck successful.');
    end
    
    % enable REMOTE
    fprintf(hyperDeckLeftSocket,'remote: enable: true\n');
    respL = fgets(hyperDeckLeftSocket);
    disp(['Remote enable LEFT: ' strtrim(char(respL))]);
    
    % select slot 1
    fprintf(hyperDeckLeftSocket,'slot select: slot id: 1\n');
    respL = fgets(hyperDeckLeftSocket);
    disp(['Slot 1 select LEFT: ' strtrim(char(respL))]);
end

if(use.hyperDeckRight)
    fprintf(hyperDeckRightSocket,'ping\n');
    respR = fgets(hyperDeckRightSocket);
    if(isempty(respL) || ~strcmp(respL(1:6),'200 ok'))
        error('Software ping to RIGHT HyperDeck failed!');
    else
        disp('Software ping to RIGHT HyperDeck successful.');
    end
    % enable REMOTE
    fprintf(hyperDeckRightSocket,'remote: enable: true\n');
    respR = fgets(hyperDeckRightSocket);
    disp(['Remote enable RIGHT: ' strtrim(char(respR))]);
    
    % select slot 1
    fprintf(hyperDeckRightSocket,'slot select: slot id: 1\n');
    respR = fgets(hyperDeckRightSocket);
    disp(['Slot 1 select RIGHT: ' strtrim(char(respR))]);
end
end

% start recording on both Hyper Decks
% Note: \n seems to work in MATLAB on windows, need \r\n in python
if(~kinematicsOnly)
if(use.hyperDeckLeft)
    fprintf(hyperDeckRightSocket,'record\n');    
end
if(use.hyperDeckRight)
    fprintf(hyperDeckLeftSocket,'record\n');
end
end
if(use.kinematicsPC)
    fprintf(kinematicsPCSocket,'record');
end
% fprintf(hyperDeckLeft,'stop\n');
% fprintf(hyperDeckRight,'stop\n');

% get response to record command (if applicable) and close socket
% TODO: This will add a small delay, find a workaround
if(~kinematicsOnly)
if(use.hyperDeckLeft)
    respL = fgets(hyperDeckLeftSocket);
    disp(['Record LEFT: ' strtrim(char(respL))]);
    fclose(hyperDeckLeftSocket);
end

if(use.hyperDeckRight)
    respR = fgets(hyperDeckRightSocket);
    disp(['Record RIGHT: ' strtrim(char(respR))]);
    fclose(hyperDeckRightSocket);
end
end

if(use.kinematicsPC)
    fclose(kinematicsPCSocket);
end