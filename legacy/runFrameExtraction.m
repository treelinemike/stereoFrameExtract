% create a copy of this file for every extraction you run
% this just helps you set up a call to stereoExtractFramesByNumber()
% note: stereoExtractFramesByNumber.m should be versioned in GitHub
stereoExtractFramesByNumber( ... 
    '"D:\Capture0006.mov"', ...  % left video file... double quotes inside single quotes IMPORTANT FOR MAC IF SPACES IN PATH
    '"E:\Capture0006.mov"', ...  % right video file... double quotes inside single quotes IMPORTANT FOR MAC IF SPACES IN PATH
    0, ...  % frame offset: if positive shifts right track this many frames earlier, or left track this many frames later
    'phantom_frames.csv', ...   % CSV file containing frame numbers (relative to unshifted left channel) to extract
    29.970030 ... % frame rate (29.970030 for da Vinci S)
    );