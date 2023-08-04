// until we have a makefile or cmake, compile with:
// g++ -Wall -I/usr/include/opencv4 vcrop.cpp -lavutil -lavformat -lavcodec -lswscale -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -o vcrop  

// extracts a clip of v210 video 
// by demuxing out of one MOV wrapper and muxing into another
// should excatly preserve frames!

// followed several examples to put this together

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <opencv2/opencv.hpp>

#define OUTFILE_NAME "out.mov"
#define VIDEO_FILE "test_twoframe.mov"
//#define VIDEO_FILE "20230521_0deg_cal_L01.mov"

int main(void){

    static AVFormatContext * ifmt_ctx; // input format context
    AVStream *istream;
    
    static AVFormatContext * ofmt_ctx; // output format context
    AVStream * ostream;
    
    AVCodecParameters * codec_params;
    AVPacket * pkt = NULL;
    
    unsigned int video_stream_idx = 0;
    bool found_video_stream = false;
    unsigned int my_frame_counter = 0;

    std::cout << "Analyzing " << VIDEO_FILE << "..." << std::endl;

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
    std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl << std::endl;
    av_dump_format(ifmt_ctx, 0, VIDEO_FILE, false);
    std::cout << std::endl;

    // look for a single v210 video stream in this container
    for(unsigned int stream_idx = 0; stream_idx < ifmt_ctx->nb_streams; ++stream_idx){
        istream = ifmt_ctx->streams[stream_idx];
        codec_params =istream->codecpar;
        printf("Located stream %02d: codec type = %03d, codec id = %03d\n",
                stream_idx,
                codec_params->codec_type,
                codec_params->codec_id);

        // determine whether this is v210 encoded video
        if( codec_params->codec_type == AVMEDIA_TYPE_VIDEO && codec_params->codec_id == AV_CODEC_ID_V210){
            //std::cout << "FOUND v210 ENCODED VIDEO" << std::endl;
            if(found_video_stream){
                std::cout << "ERROR: MORE THAN ONE v210 VIDEO STREAM FOUND!" << std::endl;
                return -1;
            } else {
                video_stream_idx = stream_idx;
                found_video_stream = true;
            }
        }
    }

    // error out if we didn't find v210 video
    if(!found_video_stream){
        std::cout << "ERROR: DID NOT FIND A VIDEO STREAM" << std::endl;
        return -1;
    }

    // report success
    istream = ifmt_ctx->streams[video_stream_idx];
    codec_params = istream->codecpar;
    printf("Working with stream %02d: codec type = %03d, codec id = %03d\n",
            video_stream_idx,
            codec_params->codec_type,
            codec_params->codec_id);
    std::cout << "stream timebase: " << (istream->time_base).num << "/" << (istream->time_base).den << std::endl;


    // allocate packet pointer
    pkt = av_packet_alloc();
    if(!pkt){
        std::cout << "ERROR: COULD NOT ALLOCATE DECODER PACKET POINTER" << std::endl;
        return -1;
    }

    // allocate output format context
    if( avformat_alloc_output_context2(&ofmt_ctx,NULL,NULL,OUTFILE_NAME) < 0) {
        std::cout << "ERROR: COULD NOT CREATE OUTPUT CONTEXT" << std::endl;
        return -1;
    }
    
    // single output stream for video
    if( (ostream = avformat_new_stream(ofmt_ctx,NULL)) == NULL ){
        std::cout << "ERROR: COULD NOT ALLOCATE OUTPUT STREAM" << std::endl;
        return -1;
    }

    // copy codec parameters from input stream
    if( avcodec_parameters_copy(ostream->codecpar,istream->codecpar)){
        std::cout << "ERROR: COULD NOT COPY CODEC PARAMETERS FROM INPUT STREAM TO OUTPUT STREAM" << std::endl;
        return -1;
    }

    // open the output file
    if( !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)){
        std::cout << "Trying to open output file..." << std::endl;
        if( avio_open(&ofmt_ctx->pb,OUTFILE_NAME,AVIO_FLAG_WRITE ) < 0){
            std::cout << "ERROR: COULD NOT OPEN OUTPUT FILE" << std::endl;
            return -1;
        }
    }

    // write header
    if( avformat_write_header(ofmt_ctx, NULL) < 0){
        std::cout << "ERROR: COULD NOT WRITE OUTPUT FILE HEADER" << std::endl;
        return -1;
    }




    // now we start reading
    while( av_read_frame(ifmt_ctx,pkt) >= 0 ){
        if((unsigned int)(pkt->stream_index) == video_stream_idx){

            std::cout << "Adding packet of size: " << pkt->size << std::endl;
            
            // add packet to new video file
            if( av_interleaved_write_frame(ofmt_ctx, pkt) < 0){
                std::cout << "ERROR: COULD NOT WRTIE PACKET TO OUTPUT STREAM" << std::endl;
                return -1;
            }


            ++my_frame_counter;
        }

        // unreference the packet pointer
        av_packet_unref(pkt);
    
    }

    std::cout << "Processed " << my_frame_counter << " frames!" << std::endl;

    if( av_write_trailer(ofmt_ctx) ){
        std::cout << "ERROR WRITING OUTPUT FILE TRAILER" << std::endl;
        return -1;
    }

    // done
    return 0;
}


/*

    // prepare an ENCODER to reencode video
    const AVCodec *enc = NULL;
    enc = avcodec_find_encoder(stream->codecpar->codec_id);
    if(!enc){
        std::cout << "ERROR: COULD NOT FIND ENCODER" << std::endl;
        return -1;
    }

    // allocate encoder context
    AVCodecContext *enc_ctx;
    enc_ctx = avcodec_alloc_context3(enc);
    if(!enc_ctx){
        std::cout << "ERROR: COULD NOT OPEN ENCODER CONTEXT" << std::endl;
        return -1;
    }

    // copy codec parameters from stream to encoder
    if( avcodec_parameters_to_context(enc_ctx, stream->codecpar) < 0){
        std::cout << "ERROR: COULD NOT COPY CODEC PARAMS FROM STREAM TO ENCODER" << std::endl;
        return -1;
    }

    // set encoder timebase
    enc_ctx->time_base = stream->time_base;

    // initialize encoder
    if( avcodec_open2(enc_ctx, enc, NULL) < 0){
        std::cout << "ERROR: COULD NOT INITIALIZE ENCODER" << std::endl;
        return -1;
    }




   */






/*  OLD CODE TRYING OUT ENCODER
// get our encoder ready
AVCodec *tiff_codec = NULL;
if( (tiff_codec = avcodec_find_encoder(AV_CODEC_ID_TIFF)) == NULL){
std::cout << "ERROR: COULD NOT FIND OUTPUT IMAGE ENCODER" << std::endl;
return -1;
}

AVCodecContext * img_ctx = NULL;
if( !(img_ctx = avcodec_alloc_context3(tiff_codec)) ){
std::cout << "ERROR: COULD NOT ALLOCATE CONTEXT FOR IMAGES" << std::endl;
return -1;
}
img_ctx->pix_fmt = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_YUV422P;//(AVPixelFormat)expected_pix_fmt;
img_ctx->time_base = (AVRational){1,1};
img_ctx->width = expected_width;
img_ctx->height = expected_height;
// initialize encoder
std::cout << "test" << std::endl;
if( avcodec_open2(img_ctx, tiff_codec, NULL) < 0){
std::cout << "ERROR: COULD NOT INITIALIZE ENCODER" << std::endl;
return -1;
}

AVPacket *pkt_enc = NULL;
pkt_enc = av_packet_alloc();
if(!pkt_enc){
std::cout << "ERROR: COULD NOT ALLOCATE ENCODER PACKET POINTER" << std::endl;
return -1;
}

*/
