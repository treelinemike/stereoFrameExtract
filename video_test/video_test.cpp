// until we have a makefile or cmake, compile with:
// g++ -Wall -o video_test video_test.cpp -lavutil -lavformat -lavutils

// followed several ffmpeg examples to create this

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIDEO_FILE "test_twoframe.mov"

typedef struct StreamContext{
    AVCodecContext * dec_ctx;
    AVCodecContext * enc_ctx;
    AVFrame * dec_frame;
} StreamContext;

static AVFormatContext * ifmt_ctx;
static AVFormatContext * ofmt_ctx;
static StreamContext * stream_ctx;

int main(void){
    std::cout << "Hello, world!" << std::endl;

    // open video file and read headers
    if( avformat_open_input(&ifmt_ctx, VIDEO_FILE, NULL, NULL) != 0 ){
        std::cout << "Could not open " << VIDEO_FILE << std::endl;
        return -1;
    }
  
    // get stream info from file
    if( avformat_find_stream_info( ifmt_ctx, NULL ) < 0){
        std::cout << "avformat_find_stream_info() failed!" << std::endl;
        return -1;
    }
    
    // print video format info to screen
    av_dump_format(ifmt_ctx, 0, VIDEO_FILE, false);
    std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl;

    // allocate memory for streams
    stream_ctx = (StreamContext*) av_calloc(ifmt_ctx->nb_streams, sizeof(*stream_ctx)); 
    if(!stream_ctx){
        std::cout << "Error, cannot allocate memory for stream!" << std::endl;
    }

    // iterate through all streams
    for(unsigned int stream_idx = 0; stream_idx < ifmt_ctx->nb_streams; ++stream_idx){
        AVStream *stream = ifmt_ctx->streams[stream_idx];

    }

    // done
    return 0;
}
