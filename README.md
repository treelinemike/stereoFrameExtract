# video-tools
Some relatively simple tools for accurate temporal cropping and frame extraction from v210-encoded MOV files. Uses ffmpeg libav libraries despite unsatisfactory results from the ffmpeg command line utility. Tools should build under GNU g++ and MS Visual Studio 2022.

## Requirements for vcrop
- [ffmpeg](https://ffmpeg.org/download.html) 5.1.2: dynamically linked 
- [cxxopts](https://github.com/jarro2783/cxxopts): header-only library
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) 0.7.0: statically linked

## Requirements for vtiff
- [ffmpeg](https://ffmpeg.org/download.html) 5.1.2: dynamically linked 
- [cxxopts](https://github.com/jarro2783/cxxopts): header-only library
- OpenCV 4.8.0: dynamically linked
