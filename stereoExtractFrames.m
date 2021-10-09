% simple script to extract stereo frame pairs from left and right video
% files
%
% vidFileL: path and filename for LEFT video channel
% vidFileR: path and filename for RIGHT video channel
% offsetR:  offset to add to left frame number to get right frame number (could be negative)
% timecodeListFile: path and filename to CSV file containing timecode for each frame to extract (in dropcode format) on separate lines
% frameRate: only valid values are 29.97 and 59.94
function stereoExtractFrames(vidFileL,vidFileR,offsetR,timecodeListFile,frameRate)

% enable imagemagick and ffmpeg on mac platform
if(ismac)
    setenv('PATH', [getenv('PATH') ':/usr/local/bin']);
end

% make sure frame rate is appropriate
if( (frameRate ~= 29.97) && (frameRate ~= 59.94))
    error('Unsupported frame rate, this only works for 29.97Hz and 59.94Hz');
end

% load timecodes for LEFT frames
fid = fopen(timecodeListFile);  
timecodeData = textscan(fid,'%s');
fclose(fid);
timecodeData = timecodeData{1};

% make L and R directories for image files
if(~isfolder('L'))
    mkdir('L');
end
if(~isfolder('R'))
    mkdir('R');
end

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
    
    disp(['Extracting frame numbers (L/R): ' num2str(frameNumL) ' @ ' flatTimecodeL ' / ' num2str(frameNumR)  ' @ ' flatTimecodeR ]);
    
    % extract frames (-y forces overwrite of output file)
    % rescale output timescale for consistent labeling, won't correspond to
    % actual clock time but we've converted our desired frame time for this
    % already...
    % -pix_fmt rgb48 => (16 bits x 3 colors) = 48 bit depth
    cmdL = ['ffmpeg -hide_banner -loglevel panic -y -r ' sprintf('%05.2f',frameRate) ' -ss ' flatTimecodeL ' -i ' vidFileL ' -vframes 1 -pix_fmt rgb48 ' sprintf('.\\L\\L%08d.tif',snapIdx)];
    cmdR = ['ffmpeg -hide_banner -loglevel panic -y -r ' sprintf('%05.2f',frameRate) ' -ss ' flatTimecodeR ' -i ' vidFileR ' -vframes 1 -pix_fmt rgb48 ' sprintf('.\\R\\R%08d.tif',snapIdx)];
    system(cmdL);
    system(cmdR);
    
end
