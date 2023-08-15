// extracts a clip of v210 video 
// by demuxing packets out of one MOV wrapper and muxing them directly back into another
// should preserve exact frames!

// followed several examples to put this together

// TODO: add better exception handling, now most exceptions just fail out opaquely

// need extern to include FFmpeg C libraries
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#include <iostream>
#include <string>
#include <cxxopts.hpp>  // https://www.github.com/jarro2783/cxxopts
#include <yaml-cpp/yaml.h>

int main(int argc, char** argv) {

	// INPUT-RELATED VARIABLES
	static AVFormatContext* ifmt_ctx; // input format context
	AVStream* istream;
	AVCodecParameters* codec_params;
	AVPacket* inpkt = NULL;
	AVRational iframerate;

	// OUTPUT-RELATED VARIABLES
	static AVFormatContext* ofmt_ctx; // output format context
	AVStream* ostream;
	const AVCodec* enc = NULL;
	AVCodecContext* enc_ctx;


	// general variables
	static AVFrame* frame = NULL;
	YAML::Node config;
	unsigned int video_stream_idx = 0;
	bool found_video_stream = false;
	uint64_t my_frame_counter = 0;
	uint64_t firstframe, lastframe, num_frames_to_extract, pts_dts_scale;
	int prev_pct = -1;
	cxxopts::Options options("vcrop", "temporal video cropping");
	std::string infile_name, outfile_name, yamlfile_name;
	bool yaml_mode = false;
	bool compress_flag = false;

	// struct and vector for storing clip details
	struct ClipDef {
		std::string name;
		uint64_t first_frame;
		uint64_t last_frame;
	};
	std::vector<ClipDef> clips;

	// PARSE COMMAND LINE OPTIONS
	try
	{
		options.add_options()
			("f,first", "number of first frame to include in output video", cxxopts::value<uint64_t>())
			("l,last", "number of last frame to inclue in output video", cxxopts::value<uint64_t>())
			("o,output", "name of output file", cxxopts::value<std::string>())
			("i,input", "name of input file", cxxopts::value<std::string>())
			("z", "flag to compress output video", cxxopts::value<bool>()->default_value("false"))
			("c,config", "name of config YAML file - use without setting any other options", cxxopts::value<std::string>());
		auto cxxopts_result = options.parse(argc, argv);

		if (cxxopts_result.count("config") == 1)
		{
			yaml_mode = true;
			yamlfile_name = cxxopts_result["config"].as<std::string>();
		}
		else if (
			(cxxopts_result.count("first") == 1) &&
			(cxxopts_result.count("last") == 1) &&
			(cxxopts_result.count("output") == 1) &&
			(cxxopts_result.count("input") == 1)) {
			yaml_mode = false;
			compress_flag = cxxopts_result["z"].as<bool>();
			firstframe = cxxopts_result["first"].as<uint64_t>();
			lastframe = cxxopts_result["last"].as<uint64_t>();
			infile_name = cxxopts_result["input"].as<std::string>();
			outfile_name = cxxopts_result["output"].as<std::string>();

			// add our single clip to the clip storage vector
			struct ClipDef singleclip = { outfile_name, firstframe, lastframe };
			clips.push_back(singleclip);

		}
		else {
			std::cout << options.help() << std::endl;
			return -1;
		}
	}
	catch (const cxxopts::exceptions::exception& e)
	{
		std::cout << "Error parsing options: " << e.what() << std::endl;
		return -1;
	}



	// PARSE YAML CONFIG FILE
	// add each clip definition to our vector
	if (yaml_mode) {
		try {
			// get input file
			config = YAML::LoadFile(yamlfile_name);
			if (!config["input_file"]) {
				std::cout << "ERROR: No 'input_file' key found in YAML config file" << std::endl;
				return -1;
			}
			infile_name = config["input_file"].as<std::string>();
			//std::cout << "YAML input file: " << infile_name << std::endl;

			// get compression flag
			if (config["compress"]) {
				compress_flag = config["compress"].as<bool>();
				//std::cout << "YAML compression: " << compress_flag << std::endl;
			}
			else {
				compress_flag = false;
				std::cout << "YAML: no compression set, not compressing output" << std::endl;
			}

			// parse each clip definition
			if (!config["clips"]) {
				std::cout << "ERROR: no 'clips' key found in YAML config file" << std::endl;
				return -1;
			}
			YAML::Node yamlclips = config["clips"];
			for (YAML::const_iterator it = yamlclips.begin(); it != yamlclips.end(); ++it) {
				outfile_name = it->first.as<std::string>();
				YAML::Node clipdetails = it->second;
				firstframe = clipdetails["first"].as<uint64_t>();
				lastframe = clipdetails["last"].as<uint64_t>();
				std::cout << "YAML clip def: " << outfile_name << " [" << firstframe << "," << lastframe << "]" << std::endl;
				struct ClipDef singleclip = { outfile_name, firstframe, lastframe };
				clips.push_back(singleclip);
			}

			// make sure we have some clip definitions in our vector
			if (!clips.size()) {
				std::cout << "ERROR: no valid clip definitions found in YAML config file" << std::endl;
				return -1;
			}
		}
		catch (const YAML::Exception& e)
		{
			std::cout << "ERROR PARSING YAML CONFIGURATION: " << e.what() << std::endl;
			return -1;
		}
	}

	if(compress_flag)
		std::cout << "COMPRESSING VIDEO" << std::endl;



	// INPUT FORMAT
	// open video file and read headers
	if (avformat_open_input(&ifmt_ctx, infile_name.c_str(), NULL, NULL) != 0) {
		std::cout << "Could not open file: " << infile_name << std::endl;
		return -1;
	}

	// get stream info from file
	if (avformat_find_stream_info(ifmt_ctx, NULL) < 0) {
		std::cout << "avformat_find_stream_info() failed!" << std::endl;
		return -1;
	}

	// print video format info to screen
	std::cout << "Number of streams found: " << ifmt_ctx->nb_streams << std::endl << std::endl;
	av_dump_format(ifmt_ctx, 0, infile_name.c_str(), false);
	std::cout << std::endl;

	// look for a single v210-encoded video stream in this container
	for (unsigned int stream_idx = 0; stream_idx < ifmt_ctx->nb_streams; ++stream_idx) {
		istream = ifmt_ctx->streams[stream_idx];
		codec_params = istream->codecpar;
		printf("Located stream %02d: codec type = %03d, codec id = %03d\n",
			stream_idx,
			codec_params->codec_type,
			codec_params->codec_id);

		// determine whether this is v210 encoded video
		if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO && codec_params->codec_id == AV_CODEC_ID_V210) {
			//std::cout << "FOUND v210 ENCODED VIDEO" << std::endl;
			if (found_video_stream) {
				std::cout << "ERROR: MORE THAN ONE v210 VIDEO STREAM FOUND!" << std::endl;
				return -1;
			}
			else {
				video_stream_idx = stream_idx;
				found_video_stream = true;
			}
		}
	}

	// error out if we didn't find v210 video
	if (!found_video_stream) {
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

	// guess input framerate
	iframerate = av_guess_frame_rate(ifmt_ctx, istream, NULL);

	// set scaling
	pts_dts_scale = (uint64_t)av_q2d(av_mul_q(av_inv_q(istream->time_base), av_inv_q(istream->avg_frame_rate)));





	// DECODER (only needed if transcoding / compressing video - otherwise just transmux packets) 
	// open the decoder
	const AVCodec* dec = NULL;
	dec = avcodec_find_decoder(istream->codecpar->codec_id);
	if (!dec) {
		std::cout << "ERROR: COULD NOT FIND DECODER" << std::endl;
		return -1;
	}

	// allocate decoder context
	AVCodecContext* dec_ctx = NULL;
	dec_ctx = avcodec_alloc_context3(dec);
	if (!dec_ctx) {
		std::cout << "ERROR: COULD NOT OPEN DECODER CONTEXT" << std::endl;
		return -1;
	}

	// copy codec parameters from stream to decoder
	if (avcodec_parameters_to_context(dec_ctx, istream->codecpar) < 0) {
		std::cout << "ERROR: COULD NOT COPY CODEC PARAMS FROM STREAM TO DECODER" << std::endl;
		return -1;
	}

	// initialize decoder
	if (avcodec_open2(dec_ctx, dec, NULL) < 0) {
		std::cout << "ERROR: COULD NOT INITIALIZE DECODER" << std::endl;
		return -1;
	}

	// allocate frame, this can happen anywhere
	// we will keep reusing the frame memory
	if((frame = av_frame_alloc()) == 0){
		std::cout << "ERROR: COULD NOT ALLOCATE FRAME POINTER" << std::endl;
		return -1;
	}

	
	// ITERATE THROUGH ALL CLIP DEFINITIONS
	// EXTRACTING THE DESIRED SEGMENT OF VIDEO TO FILE
	//std::cout << "Analyzing " << infile_name << "..." << std::endl;
	for (auto& item : clips) {

		// reset frame counters
		my_frame_counter = 0;
		prev_pct = -1;

		// get clip definition parameters
		// NOTE: 'infile' is common and has already been set
		outfile_name = item.name;
		firstframe = item.first_frame;
		lastframe = item.last_frame;
		num_frames_to_extract = (lastframe - firstframe + 1);
		std::cout << "Processing: " << outfile_name << " [" << firstframe << "," << lastframe << "]" << std::endl;




		// OUTPUT FORMAT
		// NEED THIS HERE BECAUSE WE INITIAIZE OUTPUT FORMAT CONTEXT FOR EACH NEW FILE
		// allocate output format context
		if (avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outfile_name.c_str()) < 0) {
			std::cout << "ERROR: COULD NOT CREATE OUTPUT CONTEXT" << std::endl;
			return -1;
		}

		// single output stream for video
		if ((ostream = avformat_new_stream(ofmt_ctx, NULL)) == NULL) {
			std::cout << "ERROR: COULD NOT ALLOCATE OUTPUT STREAM" << std::endl;
			return -1;
		}


		// ENCODER (only needed if transcoding / compressing video - otherwise just transmux packets)

		enc = avcodec_find_encoder(AV_CODEC_ID_FFV1);
		if (!enc) {
			std::cout << "ERROR: COULD NOT FIND ENCODER" << std::endl;
			return -1;
		}

		// allocate encoder context
		if ((enc_ctx = avcodec_alloc_context3(enc)) == 0) {
			std::cout << "ERROR: COULD NOT OPEN ENCODER CONTEXT" << std::endl;
			return -1;
		}

		// set output stream and encoder parameters
		// these hold for both transmuxing and transcoding
		enc_ctx->framerate = iframerate;
		enc_ctx->time_base = istream->time_base;
		enc_ctx->codec_type = enc->type;
		enc_ctx->codec_id = enc->id;
		enc_ctx->pix_fmt = dec_ctx->pix_fmt;
		enc_ctx->width = dec_ctx->width;
		enc_ctx->height = dec_ctx->height;
		enc_ctx->bits_per_raw_sample = istream->codecpar->bits_per_raw_sample;
		enc_ctx->bits_per_coded_sample = istream->codecpar->bits_per_raw_sample;
		ostream->sample_aspect_ratio.num = 1;
		ostream->sample_aspect_ratio.den = 1;
		ostream->time_base = istream->time_base;
		ostream->avg_frame_rate = istream->avg_frame_rate;
		ostream->r_frame_rate = istream->r_frame_rate;

		// update output stream and encoder codec parameters
		if (!compress_flag) {

			// TRANSMUXING ONLY: get codec parameters from input stream
			if (avcodec_parameters_copy(ostream->codecpar, istream->codecpar) < 0) {
				std::cout << "ERROR: COULD NOT COPY CODEC PARAMETERS FROM INPUT STREAM TO OUTPUT STREAM" << std::endl;
				return -1;
			}
		}
		else {
			// TRANSCODING: get codec parametrs from encoder context
			if (avcodec_parameters_from_context(ostream->codecpar, enc_ctx) < 0) {
				std::cout << "ERROR: COULD NOT COPY CODEC PARAMETERS FROM ENCODER CONTEXT" << std::endl;
			}
		}

		enc_ctx->time_base = istream->time_base;
		enc_ctx->codec_type = enc->type;
		enc_ctx->codec_id = enc->id;
		enc_ctx->pix_fmt = dec_ctx->pix_fmt;
		enc_ctx->width = dec_ctx->width;
		enc_ctx->height = dec_ctx->height;
		enc_ctx->bits_per_raw_sample = istream->codecpar->bits_per_raw_sample;
		enc_ctx->bits_per_coded_sample = istream->codecpar->bits_per_raw_sample;
		ostream->sample_aspect_ratio.num = 1;
		ostream->sample_aspect_ratio.den = 1;
		ostream->time_base = istream->time_base;
		ostream->avg_frame_rate = istream->avg_frame_rate;
		ostream->r_frame_rate = istream->r_frame_rate;
		pts_dts_scale = (uint64_t)av_q2d(av_mul_q(av_inv_q(istream->time_base), av_inv_q(istream->avg_frame_rate)));

		std::cout << "Enc ctx width: " << enc_ctx->width << ", height: " << enc_ctx->height << std::endl;
		//return -1;
		std::cout << "Output stream timebase: " << (ostream->time_base).num << "/" << (ostream->time_base).den << std::endl;
		//std::cout << "Output stream index: " << ostream->index << std::endl;
		//std::cout << "SCALE FACTOR: " << pts_dts_scale << std::endl;
		//std::cout << "OUT STREAM ID " << ostream->id << std::endl;
		//std::cout << "CODEC SAR: " << istream->codecpar->sample_aspect_ratio.num << ":" << istream->codecpar->sample_aspect_ratio.den << std::endl;
		//std::cout << "STREAM SAR: " << ostream->sample_aspect_ratio.num << ":" << ostream->sample_aspect_ratio.den << std::endl;

		// initialize encoder
		if (avcodec_open2(enc_ctx, enc, NULL) < 0) {
			std::cout << "ERROR: COULD NOT INITIALIZE ENCODER" << std::endl;
			return -1;
		}

		// allocate packet pointer
		inpkt = av_packet_alloc();
		if (!inpkt) {
			std::cout << "ERROR: COULD NOT ALLOCATE DECODER PACKET POINTER" << std::endl;
			return -1;
		}

		// open the output file
		if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
			std::cout << "Opening output file" << std::endl;
			if (avio_open(&ofmt_ctx->pb, outfile_name.c_str(), AVIO_FLAG_WRITE) < 0) {
				std::cout << "ERROR: COULD NOT OPEN OUTPUT FILE" << std::endl;
				return -1;
			}
		}

		// write header
		if (avformat_write_header(ofmt_ctx, NULL) < 0) {
			std::cout << "ERROR: COULD NOT WRITE OUTPUT FILE HEADER" << std::endl;
			return -1;
		}

		std::cout << "Seeking to first frame in clip" << std::endl;
		if (av_seek_frame(ifmt_ctx, video_stream_idx, firstframe * pts_dts_scale, 0) < 0) {
			std::cout << "ERROR: SEEK FAILED" << std::endl;
			return -1;
		}

		// now we start reading
		while (av_read_frame(ifmt_ctx, inpkt) >= 0 && my_frame_counter < num_frames_to_extract) {
			if ((unsigned int)(inpkt->stream_index) == video_stream_idx) {

				// skip this packet if it is flagged to discard
				if (inpkt->flags & AV_PKT_FLAG_DISCARD) {
					std::cout << "Discarding packet." << std::endl;
					continue;
				}

				//std::cout << "Adding packet of size: " << inpkt->size << std::endl;
				//printf("PTS: %ld; DTS: %ld; POS: %ld; FLAG? %d\n",inpkt->pts,inpkt->dts,inpkt->pos, (inpkt->flags & AV_PKT_FLAG_KEY) ); 

				if (!compress_flag) {

					// TRANSMUXING ONLY

					// correct PTS and DTS for output stream
					inpkt->pts = my_frame_counter * pts_dts_scale;
					inpkt->dts = my_frame_counter * pts_dts_scale;

					// mux packet into output format
					inpkt->stream_index = ostream->index;
					if (av_interleaved_write_frame(ofmt_ctx, inpkt) < 0) {
						std::cout << "ERROR: COULD NOT WRTIE PACKET TO OUTPUT STREAM" << std::endl;
						return -1;
					}
					++my_frame_counter;
				}
				else {

					// TRANSCODING

					// send packet to decoder
					if (avcodec_send_packet(dec_ctx, inpkt) < 0) {
						std::cout << "ERROR SENDING PACKET TO DECODER" << std::endl;
						return -1;
					}
					else {
						std::cout << "SENT PACKET TO DECODER" << std::endl;
					}

					// get all available frames from the decoder
					int retval = 0;
					while (retval >= 0) {
						if ((retval = avcodec_receive_frame(dec_ctx, frame)) == 0) {
							std::cout << "RECEIVED A DECODED FRAME" << std::endl;

							// encode the decoded frame with the output codec
							AVPacket* outpkt = av_packet_alloc();;
							int enc_resp = avcodec_send_frame(enc_ctx, frame);

							while (enc_resp >= 0) {
								enc_resp = avcodec_receive_packet(enc_ctx, outpkt);
								if (enc_resp == AVERROR(EAGAIN) || enc_resp == AVERROR_EOF) {
									std::cout << "NO PACKET AVAILBLE FROM ENCODER" << std::endl;
									break;
								}
								else if (enc_resp < 0) {
									std::cout << "ERROR RECEIVING PACKET FROM ENCODER" << std::endl;
									return -1;
								}
								std::cout << "RECEIVED PACKET FROM ENCODER" << std::endl;
								
								// correct output stream parameters
								outpkt->pts = my_frame_counter * pts_dts_scale;
								outpkt->dts = my_frame_counter * pts_dts_scale;
								outpkt->stream_index = ostream->index;
								outpkt->time_base = inpkt->time_base;
								outpkt->duration = inpkt->duration;
								if ((enc_resp = av_interleaved_write_frame(ofmt_ctx, outpkt)) != 0) {
									std::cout << "ERROR WRITING COMPRESSED PACKET: " << enc_resp << std::endl;
									return -1;
								}
								std::cout << "WROTE COMPRESSED PACKET" << std::endl;
							}

							// done with this packet, so unreference and free it
							av_packet_unref(outpkt);
							av_packet_free(&outpkt);

							++my_frame_counter;
						}
	
						// unreference the frame
						av_frame_unref(frame);
						//av_frame_free(&frame);
					}
				}

				if ((int)((my_frame_counter * 100) / num_frames_to_extract) > prev_pct) {
					prev_pct = (int)((my_frame_counter * 100) / num_frames_to_extract);
					printf("\r%4d%% (%8ld/%8ld)", prev_pct, my_frame_counter, num_frames_to_extract);
					fflush(stdout);
				}
			}

			// unreference the packet pointer
			av_packet_unref(inpkt);
		}

		std::cout << std::endl;

		// write trailer to finish file
		std::cout << "Writing trailer" << std::endl;
		if (av_write_trailer(ofmt_ctx)) {
			std::cout << "ERROR WRITING OUTPUT FILE TRAILER" << std::endl;
			return -1;
		}

		// free output-related memory
		avcodec_free_context(&enc_ctx);
		avio_close(ofmt_ctx->pb);
		avformat_free_context(ofmt_ctx);

		// done processing this clip, move on to next one
	}

	// free input-related memory
	avcodec_free_context(&dec_ctx);
	avformat_close_input(&ifmt_ctx);
	avformat_free_context(ifmt_ctx);

	// done
	return 0;
}