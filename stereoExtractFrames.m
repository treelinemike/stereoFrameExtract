% simple script to extract stereo frame pairs from left and right video
% files
%
% vidFileL: path and filename for LEFT video channel
% vidFileR: path and filename for RIGHT video channel
% offsetR:  offset to add to left frame number to get right frame number (could be negative)
% timecodeListFile: path and filename to CSV file containing timecode for each frame to extract (in dropcode format) on separate lines
% frameRate: only valid values are 29.97 and 59.94
function stereoExtractFrames(vidFileL,vidFileR,offsetR,timecodeListFile,frameRate)

% make sure frame rate is appropriate
if( (frameRate ~= 29.97) && (frameRate ~= 59.97))
    error('Unsupported frame rate, this only works for 29.97Hz and 59.94Hz');
end

% load timecodes for LEFT frames
fid = fopen(timecodeListFile);  
timecodeData = textscan(fid,'%s');
fclose(fid);
timecodeData = timecodeData{1};


% extract each stereo pair
for snapIdx = 1:length(timecodeData)
    
    % get left and right frame NUMBERS from dropcode and offset
    timecodeL = timecodeData{snapIdx};
    switch(frameRate)
        case 29.97
            frameNumL = dropcode2frame2997(timecodeL);
        case 59.94
            frameNumL = dropcode2frame5994(timecodeL);
    end
    frameNumR = frameNumL + offsetR;

    % compute non-dropcode time (we'll have ffmpeg rescale things not dropping codes because it is faster)
    flatTimecodeL = constRateTimecode(frameNumL,frameRate);
    flatTimecodeR = constRateTimecode(frameNumR,frameRate);
    
    % extract frames
    cmdL = ['ffmpeg -r ' sprintf('%05.2f',frameRate) ' -ss ' flatTimecodeL ' -i ' vidFileL ' -vframes 1 ' sprintf('.\\L\\L%08d.png',snapIdx)];
    cmdR = ['ffmpeg -r ' sprintf('%05.2f',frameRate) ' -ss ' flatTimecodeR ' -i ' vidFileR ' -vframes 1 ' sprintf('.\\R\\R%08d.png',snapIdx)];
    system(cmdL);
    system(cmdR);
    
end
