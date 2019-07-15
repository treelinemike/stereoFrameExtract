function timecode = constRateTimecode(frameNum,frameRate)
    totalSeconds = frameNum/frameRate;
    h = floor(totalSeconds/3600);
    secondsLeft = totalSeconds - 3600*h;
    m = floor(secondsLeft/60);
    secondsLeft = secondsLeft-m*60;
    s = floor(secondsLeft);
    ms = floor((secondsLeft-s)*1000)+0;  % add a millisecond to make sure we actually get the correct frame
    
    timecode = sprintf('%02d:%02d:%02d.%03d',h,m,s,ms);
    