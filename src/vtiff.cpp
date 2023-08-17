// extract frames from a v210-encoded MOV video file to 16-bit RGB TIFF  

// followed several ffmpeg examples to create this

// allow windows builds which would otherwise
// compain about using fopen() instead of fopen_s()
// and sprintf() instead of sprintf_s(), etc...
#define _CRT_SECURE_NO_DEPRECATE

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <cxxopts.hpp>


//#define VIDEO_FILE "test_twoframe.mov"
//#define VIDEO_FILE "20230521_0deg_cal_L01.mov"

static AVFormatContext * ifmt_ctx;

int write_avframe_to_file(AVFrame *frame, unsigned int frame_num){

    const AVCodec *tiff_codec = NULL;
    AVCodecContext *tiff_context = NULL;
    AVPacket *packet = NULL;
    FILE *outfile;
    char outfile_name[256];

    // find encoder
    if( (tiff_codec = avcodec_find_encoder(AV_CODEC_ID_TIFF)) == NULL ){
        std::cout << "ERROR FINDING TIFF ENCODER" << std::endl;
        return -1;
    }


    if( (tiff_context = avcodec_alloc_context3(tiff_codec)) == NULL ){
        std::cout << "ERROR ALLOCATING TIFF ENCODER" << std::endl;
        return -1;
    }

    tiff_context->pix_fmt = (AVPixelFormat) frame->format;
    tiff_context->height = frame->height;
    tiff_context->width = frame->width;
    tiff_context->time_base = AVRational{1,10};

    // set compression type (given in smallest to largest file order)
    av_opt_set(tiff_context->priv_data,"compression_algo","deflate",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","packbits",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","lzw",0);
    //av_opt_set(tiff_context->priv_data,"compression_algo","raw",0);

    if( avcodec_open2(tiff_context, tiff_codec, NULL) < 0 ){
        std::cout << "ERROR: CANNOT OPEN TIFF CODEC" << std::endl;
        return -1;
    }
    //av_opt_show2(tiff_context,NULL,AV_OPT_FLAG_ENCODING_PARAM,0);

    // prepare packet
    if( (packet = av_packet_alloc()) == NULL){
        std::cout << "ERROR: CANNOT ALLOCATE PACKET" << std::endl;
        return -1;
    }
    packet->data = NULL;
    packet->size = 0;

    // send frame to TIFF encoder
    if(avcodec_send_frame(tiff_context, frame) < 0){
        std::cout << "ERROR SENDING FRAME TO TIFF ENCODER" << std::endl;
        return -1;
    } 

    // retrieve encoded TIFF packet
    if(avcodec_receive_packet(tiff_context,packet) < 0){
        std::cout << "ERROR RETRIEVING PACKET FROM TIFF ENCODER" << std::endl;
        return -1;
    }

    // write packet to file
    sprintf(outfile_name,"av_frame_%08d.tif",frame_num);
    outfile = fopen(outfile_name,"wb");
    fwrite(packet->data,1,packet->size,outfile);
    fclose(outfile);

    // clean up
    av_packet_unref(packet);
    avcodec_close(tiff_context);
    return 0;
}

int main(int argc, char ** argv){

    cxxopts::Options options("vtiff","video frame extraction to tiff");
    std::string framelist_file_name, input_file_name;    
    std::vector<uint64_t> framelist;
    std::string temp, line, word;
    std::fstream framelist_file;
    uint64_t pts_dts_scale;
    bool crop_flag = false;
    bool display_flag = false;

    // add options
    try
    {
        options.add_options()
            ("c,crop", "boolean flag for cropping to da Vinci Xi frame",cxxopts::value<bool>()->default_value("false"))
            ("d,display", "display extracted frames using OpenCV",cxxopts::value<bool>()->default_value("false"))
            ("f,framelist" , "csv file containing frame numbers to extract", cxxopts::value<std::string>())
            ("i,input" , "name of input file", cxxopts::value<std::string>());
        auto cxxopts_result = options.parse(argc,argv);
        
        // show help if not enough options given
        if(     (cxxopts_result.count("framelist") != 1) ||
                (cxxopts_result.count("input") != 1) ) {
            std::cout << options.help() << std::endl;
            return -1;
        }

        crop_flag = cxxopts_result["crop"].as<bool>();
        display_flag = cxxopts_result["display"].as<bool>();
        framelist_file_name = cxxopts_result["framelist"].as<std::string>(); 
        input_file_name = cxxopts_result["input"].as<std::string>(); 

    }
    catch(const cxxopts::exceptions::exception &e)
    {
        std::cout << "Error parsing options: " << e.what() << std::endl;
        return -1;
    }

    // load framelist csv into a vector
    framelist_file.open(framelist_file_name, std::ios::in); 
    framelist.clear();
    while( std::getline(framelist_file, line) ){
        std::stringstream s(line);
        while(std::getline(s,word,',')){
            framelist.push_back((uint64_t)std::stol(word));
        }
    }
    std::cout << "Extracting frames: " << std::endl;
    for(auto & val : framelist)
        std::cout << " " << val << std::endl;

    std::cout << "Analyzing " << input_file_name << "..." << std::endl;

    // open video file and read headers
    if( avformat_open_input(&ifmt_ctx, input_file_name.c_str(), NULL, NULL) != 0 ){
        std::cout << "Could not open " << input_file_name << std::endl;
        return -1;
    }

    // get stream info from file
    if( avformat_find_stream_info( ifmt_ctx, NULL ) < 0){
        std::cout << "avformat_find_stream_info() failed!" << std::endl;
        return -1;
    }

    // print video format info to screen
    std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl << std::endl;
    av_dump_format(ifmt_ctx, 0, input_file_name.c_str(), false);
    std::cout << std::endl;

    // look for a single v210 video stream in this container
    unsigned int video_stream_idx = 0;
    bool found_video_stream = false;
    for(unsigned int stream_idx = 0; stream_idx < ifmt_ctx->nb_streams; ++stream_idx){
        AVStream *stream = ifmt_ctx->streams[stream_idx];
        AVCodecParameters *codec_params = stream->codecpar;

        printf("Located stream %02d: codec type = %03d, codec id = %03d\n",
                stream_idx,
                codec_params->codec_type,
                codec_params->codec_id);

        // determine whether this is v210 encoded video
        if( codec_params->codec_type == AVMEDIA_TYPE_VIDEO && (codec_params->codec_id == AV_CODEC_ID_V210 || codec_params->codec_id == AV_CODEC_ID_FFV1)){
            //std::cout << "FOUND v210 OR FFV1 ENCODED VIDEO" << std::endl;
            if(found_video_stream){
                std::cout << "ERROR: MORE THAN ONE READABLE VIDEO STREAM FOUND!" << std::endl;
                return -1;
            } else {
                video_stream_idx = stream_idx;
                found_video_stream = true;
            }
        }
    }

    // error out if we didn't find v210 or FFV1 video
    if(!found_video_stream){
        std::cout << "ERROR: DID NOT FIND A PROPERLY ENCODED VIDEO STREAM" << std::endl;
        return -1;
    }

    // report success
    AVStream *stream = ifmt_ctx->streams[video_stream_idx];
    AVCodecParameters *codec_params = stream->codecpar;
    pts_dts_scale = (uint64_t)av_q2d(av_mul_q(av_inv_q(stream->time_base),av_inv_q(stream->avg_frame_rate)));

    printf("Working with stream %02d: codec type = %03d, codec id = %03d\n",
            video_stream_idx,
            codec_params->codec_type,
            codec_params->codec_id);


    // open the decoder
    const AVCodec *dec = NULL;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!dec){
        std::cout << "ERROR: COULD NOT FIND DECODER" << std::endl;
        return -1;
    }

    // allocate decoder context
    AVCodecContext *dec_ctx;
    dec_ctx = avcodec_alloc_context3(dec);
    if(!dec_ctx){
        std::cout << "ERROR: COULD NOT OPEN DECODER CONTEXT" << std::endl;
        return -1;
    }

    // copy codec parameters from stream to decoder
    if( avcodec_parameters_to_context(dec_ctx, stream->codecpar) < 0){
        std::cout << "ERROR: COULD NOT COPY CODEC PARAMS FROM STREAM TO DECODER" << std::endl;
        return -1;
    }

    // initialize decoder
    if( avcodec_open2(dec_ctx, dec, NULL) < 0){
        std::cout << "ERROR: COULD NOT INITIALIZE DECODER" << std::endl;
        return -1;
    }

    // report frame width and height
    std::cout << "Width: " << dec_ctx->width << ", Height: " << dec_ctx->height << std::endl;
    int expected_width = dec_ctx->width;
    int expected_height = dec_ctx->height;
    AVPixelFormat expected_fmt = dec_ctx->pix_fmt;

    // allocate image frame
    static uint8_t *video_dst_data[4] = {NULL};
    static int video_dst_linesize[4];
    if( av_image_alloc(video_dst_data, video_dst_linesize, dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, 1) < 0){
        std::cout << "ERROR: COULD NOT ALLOCATE IMAGE" << std::endl;
        return -1;
    }

    // allocate pointers for a packet and a frame
    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    if(!pkt){
        std::cout << "ERROR: COULD NOT ALLOCATE DECODER PACKET POINTER" << std::endl;
        return -1;
    }

    static AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if(!frame){
        std::cout << "ERROR: COULD NOT ALLOCATE FRAME POINTER" << std::endl;
        return -1;
    }

    AVFrame *converted_frame = NULL;
    converted_frame = av_frame_alloc();
    if(!frame){
        std::cout << "ERROR: COULD NOT ALLOCATE CONVERTED FRAME POINTER" << std::endl;
        return -1;
    } 
    converted_frame->width = expected_width;
    converted_frame->height = expected_height;
    converted_frame->format = AV_PIX_FMT_RGB48LE;
    if( av_frame_get_buffer(converted_frame,0) < 0){
        std::cout << "ERROR: COULD NOT GET BUFFER FOR CONVERTED FRAME" << std::endl;
        return -1;
    }

    // create software scaling context (for colorspace / pixel format conversion, not scaling...)
    SwsContext *sws_ctx = NULL;  
    if( (sws_ctx = sws_getContext(  expected_width,
                    expected_height,
                    expected_fmt,
                    converted_frame->width,
                    converted_frame->height,
                    (AVPixelFormat)converted_frame->format,
                    SWS_POINT,
                    nullptr,
                    nullptr,
                    nullptr)) == NULL){
        std::cout << "ERROR: COULD NOT GET/ALLOCATE SWS CONTEXT" << std::endl;
        return -1;
    }

    // initialize some OpenCV matrices 
    cv::Mat cvFrameYUV(expected_height, expected_width, CV_8UC2);
    cv::Mat cvFrameBGR;

    // now we start reading
    unsigned int my_frame_counter = 0;
    int avreadframe_ret = 123;

    // TODO: SEEK HERE TO EACH FRAME
    for(auto & val : framelist){

        std::cout << "Attempting to decode frame " << val << std::endl;
        if( av_seek_frame(ifmt_ctx,video_stream_idx,val*pts_dts_scale, AVSEEK_FLAG_BACKWARD) < 0){ // seeks to closest previous keyframe
            std::cout << "ERROR: SEEK FAILED" << std::endl;
            return -1;
        }
        bool found_frame = false;
        while(!found_frame){
            if( (avreadframe_ret = av_read_frame(ifmt_ctx,pkt)) < 0 ){
                std::cout << "ERROR: COULD NOT READ FRAME" << std::endl;
                return -1;
            }
            if(((unsigned int)(pkt->stream_index) == video_stream_idx)){

                std::cout << " Decoding frame " << (uint64_t)(pkt->dts/pts_dts_scale) << std::endl;

                // determine whether we're at our desired frame or if we've passed it
                // if we're before it we need to decode everything from the previous keyframe
                if ((uint64_t)pkt->dts == val * pts_dts_scale) {    // we happened to seek right to where we wanted to be
                    found_frame = true;
                }
                else if ((uint64_t)pkt->dts > (val * pts_dts_scale)) { // we missed our frame!
                    std::cout << "ERROR: SEEK PASSED THE DESIRED FRAME!" << std::endl;
                    return -1;
                }

                // send packet to the decoder
                if( avcodec_send_packet(dec_ctx,pkt) < 0){
                    std::cout << "ERROR: COULD NOT SEND PACKET TO DECODER" << std::endl;
                    return -1;
                }

                // get all available frames from the decoder
                int retval = 0;
                while( retval >= 0 ){
                    if((retval = avcodec_receive_frame(dec_ctx,frame)) == 0){

                        // we have a valid AVFrame (frame) from the file video stream
                        // only process the frame if it is the one we want
                        if (found_frame) {

                            // this AVFrame has YUV422 pixel format, so convert it to
                            // RGB444 (16bit)
                            if (sws_scale_frame(sws_ctx, converted_frame, frame) < 0) {
                                std::cout << "ERROR: COULD NOT CONVERT FRAME" << std::endl;
                                return -1;
                            }

                            if (crop_flag) {
                                std::cout << "TODO: IMPLEMENT CROPPING!" << std::endl;
                            }

                            // write frame to file using only ffmpeg (lavf, lavc, etc...)
                            write_avframe_to_file(converted_frame, my_frame_counter);


                            // WE CAN ALSO USE OPENCV TO DISPLAY AND SAVE TIF FILE
                            // CONFIRMED 02-AUG-23 THAT TIFF PIXEL DATA IS IDENTICAL (AFTER LOADING INTO MATLAB)
                            if (display_flag) {
                                // BGR48LE plays nicely with OpenCV (phew!) so convert just copy memory and convert to BGR
                                cv::Mat cv_frame_rgb(expected_height, expected_width, CV_16UC3, (uint16_t*)converted_frame->data[0]);
                                cv::Mat cv_frame_bgr;
                                cv::cvtColor(cv_frame_rgb, cv_frame_bgr, cv::COLOR_RGB2BGR);

                                // write  to file
                                //char img_filename[255]; 
                                //sprintf(img_filename,"cv_frame_%08d.tif",my_frame_counter);
                                //cv::imwrite(img_filename,cv_frame_bgr);


                                char framenum_str[255];
                                sprintf(framenum_str, "%ld", val);
                                cv::putText(cv_frame_bgr, (std::string)framenum_str, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 2.0, CV_RGB(0, 65535, 0), 6);
                                cv::imshow("my window", cv_frame_bgr);
                                cv::waitKey(800);
                            }

                            std::cout << "Saved frame " << (uint64_t)(pkt->dts / pts_dts_scale) << std::endl;

                            // we received a valid frame, so increment counter
                            ++my_frame_counter;
                        }
                    }

                    // unreference the frame pointer
                    av_frame_unref(frame);
                }        
            }

            // unreference the packet pointer
            av_packet_unref(pkt);
        }
    }

    std::cout << "Saved " << my_frame_counter << " frames!" << std::endl;


    // free memory
    avformat_close_input(&ifmt_ctx);
    avformat_free_context(ifmt_ctx);

    // done
    return 0;
}





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
