% function to test Hyperdeck connection
% passes timout (ms) to ping

% requires GREP For windows: https://sourceforge.net/projects/gnuwin32/postdownload
% requires SED for windows: https://sourceforge.net/projects/gnuwin32/files/sed/4.2.1/sed-4.2.1-setup.exe/download?use_mirror=newcontinuum

function status = isAlive(ipAddr,timeout)
%[~,sysOut] = system(['ping -n 1 -w ' num2str(timeout) ' ' ipAddr ' | grep ''% loss'' | sed ''s/.*(\(.*\)\% loss),/\1/''']);
 [~,sysOut] = system(['ping -n 1 -w ' num2str(timeout) ' ' ipAddr ' | grep "% loss" | sed "s/.*(\(.*\)\% loss),/\1/"']);

if(str2num(sysOut) == 0)
    status = 1;
else
    status = 0;
end