% restart
close all; clear; clc;

% options
swping_max_attempts = 3;   % number of HyperDeck ping retries (to avoid lockups)

% Addresses for Left and Right Hyperdecks
% TODO: this shouldn't be hard coded!!
% Set PC to 192.168.10.10 (or something similar, netmask 255.255.255.0)
hyperDeckLeftIP = '192.168.10.50';
hyperDeckRightIP = '192.168.10.60';
kinematicsPCIP = '192.168.10.70';
% kinematicsPCIP = '127.0.0.1';

%% Send standard network ping to Hyperdecks to check physical connection
use.hyperDeckLeft = true;
use.hyperDeckRight = true;
use.kinematicsPC = true;
pingError = false;

if(use.hyperDeckLeft)
    if( isAlive(hyperDeckLeftIP,100) == 0 )
        warning('Cannot directly ping LEFT HyperDeck!');
        use.hyperDeckLeft = false;
        pingError = true;
    else
        disp('LEFT HyperDeck direct ping successful.');
        use.hyperDeckLeft = true;
    end
end

if(use.hyperDeckRight)
    if( isAlive(hyperDeckRightIP,100) == 0 )
        warning('Cannot directly ping RIGHT HyperDeck!');
        use.hyperDeckRight = false;
        pingError = true;
    else
        disp('RIGHT HyperDeck direct ping successful.');
        use.hyperDeckRight = true;
    end
end

if(use.kinematicsPC)
    if( isAlive(kinematicsPCIP,100) == 0 )
        warning('Cannot directly ping kinematics PC!');
        use.kinematicsPC = false;
        pingError = true;
    else
        disp('KINPC direct ping successful.');
        use.kinematicsPC = true;
    end
end

% do not attempt to open any TCP connections if requested pings were
% unsuccessful
if(pingError)
    error('Ping error! Not proceeding');
end
fprintf("\n");

%% open sockets to each device and send initialization commands

if(use.hyperDeckLeft)
    
    % open socket and flush input and output buffers
    hyperDeckLeftSocket = tcpclient(hyperDeckLeftIP,9993,'ConnectTimeout',5,'Timeout',5);
    hyperDeckLeftSocket.flush();
    
    % attempt software ping
    swping_success = false;
    swping_attempts = 0;
    while(~swping_success && swping_attempts < swping_max_attempts)
        hyperDeckLeftSocket.writeline('ping');
        resp = hyperDeckLeftSocket.readline();
        if(strcmp(resp.extractBetween(1,6),"200 ok"))
            swping_success = true;
        else
            hyperDeckLeftSocket.flush();
        end
        swping_attempts = swping_attempts + 1;
    end
    if(~swping_success)
        clear hyperDeckLeftSocket;
        error('LEFT HyperDeck software ping FAILED.');
    else
        fprintf('LEFT HyperDesk software ping successful (attempts: %d)\n',swping_attempts);
    end

    % enable REMOTE
    hyperDeckLeftSocket.writeline('remote: enable: true');
    resp = hyperDeckLeftSocket.readline();
    disp(['LEFT HyperDeck remote enable: ' strtrim(char(resp))]);
    
    % select slot 1
    hyperDeckLeftSocket.writeline('slot select: slot id: 1');
    resp = hyperDeckLeftSocket.readline();
    disp(['LEFT HyperDeck slot 1 select: ' strtrim(char(resp))]);

    fprintf('\n');
end

if(use.hyperDeckRight)
    
    % open socket and flush input and output buffers
    hyperDeckRightSocket = tcpclient(hyperDeckRightIP,9993,'ConnectTimeout',5,'Timeout',5);
    hyperDeckRightSocket.flush();
    
    % attempt software ping
    swping_success = false;
    swping_attempts = 0;
    while(~swping_success && swping_attempts < swping_max_attempts)
        hyperDeckRightSocket.writeline('ping');
        resp = hyperDeckRightSocket.readline();
        if(strcmp(resp.extractBetween(1,6),"200 ok"))
            swping_success = true;
        else
            hyperDeckRightSocket.flush();
        end
        swping_attempts = swping_attempts + 1;
    end
    if(~swping_success)
        clear hyperDeckRightSocket;
        error('RIGHT HyperDeck software ping FAILED.');
    else
        fprintf('RIGHT HyperDesk software ping successful (attempts: %d)\n',swping_attempts);
    end

    % enable REMOTE
    hyperDeckRightSocket.writeline('remote: enable: true');
    resp = hyperDeckRightSocket.readline();
    disp(['RIGHT HyperDeck remote enable: ' strtrim(char(resp))]);
    
    % select slot 1
    hyperDeckRightSocket.writeline('slot select: slot id: 1');
    resp = hyperDeckRightSocket.readline();
    disp(['RIGHT HyperDeck slot 1 select: ' strtrim(char(resp))]);

    fprintf('\n');
end


if(use.kinematicsPC)
    kinematicsPCSocket = tcpclient(kinematicsPCIP,9993,'ConnectTimeout',5,'Timeout',5);
    kinematicsPCSocket.flush();
    fprintf('KINPC socket creation successful.\n');
end


% start recording on both Hyper Decks
% Note: \n seems to work in MATLAB on windows, need \r\n in python
if(use.hyperDeckLeft)
    hyperDeckLeftSocket.writeline('record');    
end
if(use.hyperDeckRight)
    hyperDeckRightSocket.writeline('record');
end
if(use.kinematicsPC)
    fprintf(kinematicsPCSocket,'record');
end
% fprintf(hyperDeckLeft,'stop\n');
% fprintf(hyperDeckRight,'stop\n');

% get response to record command (if applicable) and close socket
% TODO: This will add a small delay, find a workaround
if(use.hyperDeckLeft)
    resp = hyperDeckLeftSocket.readline();
    disp(['LEFT record: ' strtrim(char(resp))]);
    clear hyperDeckLeftSocket;
end
if(use.hyperDeckRight)
    resp = hyperDeckRightSocket.readline();
    disp(['RIGHT record: ' strtrim(char(resp))]);
    clear hyperDeckRightSocket;
end

if(use.kinematicsPC)
    fprintf('KINPC record command sent.\n');
    clear kinematicsPCSocket;
end