% create a copy of this file for every extraction you run
% this just helps you set up a call to stereoExtractFrames()
% note: stereoExtractFrames.m should be versioned in GitHub
stereoExtractFramesByNumber( ... 
    '"D:\20210504_cal_01_L.mov"', ...  % left video file... double quotes inside single quotes IMPORTANT FOR MAC IF SPACES IN PATH
    '"E:\20210504_cal_01_R.mov"', ...  % right video file... double quotes inside single quotes IMPORTANT FOR MAC IF SPACES IN PATH
    3, ...  % frame offset: if positive shifts right track this many frames earlier, or left track this many frames later
    '20210504_cal_01_L.csv', ...   % CSV file containing frame numbers (relative to unshifted left channel) to extract
    29.97 ... %frame rate
    );