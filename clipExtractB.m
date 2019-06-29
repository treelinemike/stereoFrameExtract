% simple script to extract a series of
% video clips from a longer video file
% at regularly-spaced intervals
%
% uses system calls to ffmpeg

% restart
close all; clear all; clc;

% options / settings
readFile = 'shoulder.mov';
clipLength = 10.0;   % [sec] length of clips to extract
clipSpacing = 30.0;  % [sec] spacing between clips (start to start)
%sysPrefix = '/usr/local/bin/';  % needed for Mac, for some reason?
sysPrefix = ''; % use this on MAK PC

% get number of frames in file
cmd = [sysPrefix 'ffprobe -v error -select_streams v:0 -show_entries stream=nb_frames -of default=nokey=1:noprint_wrappers=1 ' readFile];
[~,numFramesStr] = system(cmd);
numFrames = str2num(numFramesStr);

% get framerate
cmd = [sysPrefix 'ffprobe -v 0 -of csv=p=0 -select_streams v:0 -show_entries stream=r_frame_rate ' readFile];
[~,frameRateStr] = system(cmd);
frameRate = eval(frameRateStr);

% generate list of frame numbers on which to start extraction
clipLengthNum = floor(frameRate*clipLength);
clipLengthStr = constRateTimecode(clipLengthNum,frameRate);
clipSpacingNum = floor(frameRate*clipSpacing);
clipStartIdxList = 1:clipSpacingNum:(numFrames-clipLengthNum);  % could make this something other than linear, and adjust starting point

% extract clips
for clipIdx = 1:length(clipStartIdxList)
    startTimecode = constRateTimecode(clipStartIdxList(clipIdx),frameRate);
    cmd = [sysPrefix 'ffmpeg -y -r ' sprintf('%05.2f',frameRate) ' -ss ' startTimecode ' -i ' readFile ' -t ' clipLengthStr ' ' sprintf('clip_%08d.mov',clipIdx)];
    system(cmd);
end
