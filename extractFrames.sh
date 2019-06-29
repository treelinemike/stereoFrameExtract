# Script to call ffmpeg to extract a specified number of frames from each of two video sequences
# corresponding to left and right stereo cameras
#
# Note: this is a pretty slow way to extract frames; it essentially requires reprocessing the entire input video file. For a better approach see the MATLAB script stereoExtractFrames.m
#
# Written by Mike Kokko
# Updated 2019-03-22

# input video files
rightFile="CamRight6mm001.avi"
leftFile="CamLeft6mm001.avi"

# number of frames to extract from each (approximate if not an integer factor of the total number of frames)
framesToExtract=20

# get number of frames in each video sequence
rightFrames=`ffprobe -select_streams v -show_streams $rightFile 2>/dev/null | grep nb_frames | sed -e 's/nb_frames=//'`
leftFrames=`ffprobe -select_streams v -show_streams $leftFile 2>/dev/null | grep nb_frames | sed -e 's/nb_frames=//'`

# make sure both video sequences have the same number of frames
if [ "$rightFrames" -ne "$leftFrames" ]
then
    echo "Error: files contain different numbers of frames!"
    exit -1
fi

# determine integer skip factor
# luckily BASH only does integer math...
increment=$(($rightFrames/$framesToExtract))

# create R and L directories if necessary
if [ ! -d R ]
then
    mkdir R
fi
if [ ! -d L ]
then
    mkdir L
fi

# helpful ref: https://ffmpeg.org/ffmpeg-filters.html
RCMD="ffmpeg -i $rightFile -vf select='not(mod(n\,$increment))' -vsync 0 -compression_algo raw -pix_fmt rgb24 ./R/R%03d.tiff"
LCMD="ffmpeg -i $leftFile -vf select='not(mod(n\,$increment))' -vsync 0 -compression_algo raw -pix_fmt rgb24 ./L/L%03d.tiff"
$RCMD
$LCMD
