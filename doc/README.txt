Processing for stereo
Transcribed by Shannon Kossmann
Small revisions by Mike Kokko
04-May-2021

Dependencies:
•	ffmpeg and ffprobe: https://ffmpeg.org/download.html 
	o	Put ffmpeg and ffprobe somewhere convenient (programs folder?)
	o	Make sure path to binaries is in your $PATH$ environmental variable
•	Shotcut: https://shotcut.org/download/ 
•	MATLAB (recommend 2020a or newer)
	o	Make sure you have the Computer Vision toolbox
•	GitHub Desktop (or just command line git if you prefer): https://desktop.github.com/
•	Then use git to clone these repos and add them to your MATLAB search path:
	o	https://github.com/treelinemike/stereoFrameExtract.git	
	o	https://github.com/treelinemike/surgnav-tools.git 

Connecting drives to computer
•	Cards only go in 1 way
•	Find drives
•	Make sure everything labeled appropriately on hard drives
	o	Label as something intuitive
	o	Use L and R in names

Synchronizing L/R videos in time, and identifying frame pairs for processing (either calibration or reconstruction)
•	Start Shotcut (will use to visually align L & R videos in time, and to select pairs of frames to extract for further processing)
•	In playlist tab, drag in L&R calibration (checkerboard) videos
•	Drag to timeline L first
•	R click below and click add video track
•	Add R channel
•	Drag both tracks to start at time zero
•	Click on Right video in timeline. Then click filters tab, +, video filter, size position and rotate 
	o	Use 900 for first dimension of size
	o	Make position 1000
•	Click on Left channel
	o	Apply size and position filter again, this time with size 900 but position 0
•	Add another filter TO LEFT CHANNEL
	o	Go to Video, Text simple
	o	Delete time code and hit frame number button
	o	Change size/font as necessary so you see frame number below the video channels
•	Go back to the beginning and step through with arrows to find lights
•	Find a place where it transitions, you’ll probably find the two channels aren’t synchronized
•	Find one frame where clear what’s happening (one frame forward vs back big transition)
•	If light transition event happens sooner in the Left channel: 
	o	Figure out how many frames (this case 3)
	o	Move playhead to frame 3
	o	Zoom in on timeline
	o	MAKE SURE RIGHT (not left) CHANNEL IS SELECTED
	o	Click split at playhead
	o	Delete portion of R video before the playhead (3 frames long)
	o	Click and drag remaining R channel video back to beginning
	o	Check frames to confirm that light transition is now synchronized
	o	YOU WILL ENTER 3 (positive) as the offset in the runFrameExtraction.m file
•	If light transition event happens sooner in the Right channel:
	o	Figure out how many frames (this case 3)
	o	Move the playhead to frame 3
	o	Drag the right channel to start at the playhead
	o	YOU WILL ENTER -3 (negative) as the offset in the runFrameExtraction.m file
•	Figure out the frame number of the pairs you want to extract for calibration or reconstruction
•	Open excel (only write numbers)
•	First two frames are the frames on either side of the transition point (our case 384, 385)
•	Go through frames looking for a good checkerboard
	o	Want frames with whole checkerboard, minimal glare
•	Save frame numbers into excel sheet (want 30 pairs)
•	Save excel as .csv in folder for today
•	Need file ‘runframeExtraction.m’
	o	You’ll have one for each run
•	In Shotcut, save project
•	Click properties (the I in the top)
	o	Need the Frame Rate (29.97 in our case)
	o	Shotcut is flaky; may need to restart and/or click on the L or R channel when you hit properties

Using MATLAB & ffmpeg to extract the frame pairs you identified above 
•	Open MATLAB
•	If this is the first time you’re using this code add Mike’s cloned repositories to path (stereoFrameExtract and surgnav-tools)
•	Go to your project working folder in matlab
•	Open runFrameExtraction.m
•	Put in the L and R video files
	o	Have to figure out where the files are (on the external drives)
	o	Path to Left video file first, then path to Right video file
•	Offset (+/- number of frames, see above)
•	Path to CSV file with frame numbers to extract
•	Frame rate as identified in Shotcut (will be 29.97 for the da Vinci S)
•	Run runFrameExtraction.m
•	Check first two images on each side of transition to make sure you get what you expect

Running Stereo Calibration in MATLAB
•	Move or delete any non-checkerboard images from the L and R folders (e.g. the frames you extracted on either side of the light transition to verify time synchronization)
•	Make sure you have the Computer Vision toolbox for MATLAB
•	In command window type steroCameraCalibrator and enter
•	Click the Add Images button
•	Select folder of L images for camera 1
•	Select folder of R images for camera 2
•	6 mm grid if using Mike’s calibration plate
•	Click OK
•	Check the skew and tangential distortion boxes (computes those parameters as part of calibration)
•	Hit calibrate
•	Delete all pairs with errors over 1 pixel (start with highest error and work down)
•	Export Camera Parameters
•	Export to workspace
•	Check export estimation errors
•	In workspace “svster” and enter (saves stereo calibration as mat files… this is a routine in one of Mike’s repositories)

Read MATLAB info on stereoreconstruction