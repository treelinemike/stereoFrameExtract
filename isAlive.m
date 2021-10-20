% function to test Hyperdeck connection
% passes timout (ms) to ping
function status = isAlive(ipAddr,timeout)
[~,sysOut] = system(['ping -n 1 -w ' num2str(timeout) ' ' ipAddr ' | grep ''% loss'' | sed ''s/.*(\(.*\)\% loss),/\1/''']);
if(str2num(sysOut) == 0)
    status = 1;
else
    status = 0;
end