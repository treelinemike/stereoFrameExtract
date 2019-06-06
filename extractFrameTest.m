% simple script to extract stereo frame pairs from left and right video
% files

% source video files
vidFileL = 'D:\mak\stereo\20190605-or-testing\L\CaptureL0002.mov';
vidFileR = 'D:\mak\stereo\20190605-or-testing\R\CaptureR0002.mov';

offsetR = 0;   % add this value to left frame number to get corresponding right frame number

timecodeListFile = '20190605-capture0002-timecodes.csv';

% load timecodes for LEFT frames
fid = fopen(timecodeListFile);  
timecodeData = textscan(fid,'%s');
fclose(fid);
timecodeData = timecodeData{1};

% extract each stereo pair
for snapIdx = 1:length(timecodeData)
    
    % get left and right frame NUMBERS from dropcode and offset
    timecodeL = timecodeData{snapIdx};
    frameNumL = dropcode2frame5994(timecodeL);
    frameNumR = frameNumL + offsetR;
    
    flatTimecodeL = constRateTimecode(frameNumL,59.94);
    flatTimecodeR = constRateTimecode(frameNumR,59.94);
    
    % non-dropcode time (we'll have ffmpeg rescale things not dropping codes because it is faster)

    % extract frames
    cmdL = ['ffmpeg -r 59.94 -ss ' flatTimecodeL ' -i ' vidFileL ' -vframes 1 ' sprintf('L%08d.png',snapIdx)];
    cmdR = ['ffmpeg -r 59.94 -ss ' flatTimecodeR ' -i ' vidFileR ' -vframes 1 ' sprintf('R%08d.png',snapIdx)];
    system(cmdL);
    system(cmdR);
    
end
