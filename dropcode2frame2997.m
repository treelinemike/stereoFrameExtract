% convert a dropcode timecode into frame number
% assumes 29.97fps
% useful when trying to extract frame number from video file based on
% timecode seen in clip
function frameNum = dropcode2frame2997(timecode)

% calculate frames per time using dropcode
framesInOneSec  = 30;                   % 30     (for 29.97)
framesInOneMin  = framesInOneSec*60-2;  % 1798   (for 29.97)
framesInTenMin  = 10*framesInOneMin+2;  % 17982  (for 29.97)
framesInOneHour = 6*framesInTenMin;     % 107892 (for 29.97)

% split timecode into hours, minutes, seconds, and frames
tokens = strsplit(timecode,{':',';'});  % split for colon (and semicolon, which often indicates dropcode format)
if(length(tokens) ~= 4)
    error('Invalid timecode');
end
tokensNumeric = str2double(tokens);
h = uint8(tokensNumeric(1));
m = uint8(tokensNumeric(2));
s = uint8(tokensNumeric(3));
f = uint8(tokensNumeric(4));

% make sure this is a valid time
if( (f < 0) || (f > (framesInOneSec-1)))
    error('Frame number out of bounds!');
end
if( (s < 0) || (s > 59))
    error('Seconds out of bounds!');
end
if( (m < 0) || (m > 59))
    error('Minutes out of bounds!');
end
if( (h < 0) || (h > 99))
    error('Hours out of bounds!');
end
    
% make sure this is not a code that should have been dropped
if((s == 0) && (mod(m,10) ~= 0) && (f < 2))
    error('Invalid dropcode!');
end

% initialize to frame count to zero
frameNum = 0;

% add hours
frameNum = frameNum + double(h)*framesInOneHour;

% ten minute chunks
tenMinChunks = floor(double(m)/10);
frameNum = frameNum + tenMinChunks*framesInTenMin;

% add one minute chunks
oneMinChunks = (double(m) - tenMinChunks);
frameNum = frameNum + oneMinChunks*framesInOneMin;

% add seconds and frames
frameNum = frameNum + double(s)*framesInOneSec;
frameNum = frameNum + double(f);