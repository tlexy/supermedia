#include "h264_muxer.h"
#include <iostream>

H264Muxer::H264Muxer(const H264MuxerConfig& config)
	:_config(config)
{}

bool H264Muxer::open(const std::string& filename)
{
	if (avformat_alloc_output_context2(&_fmt_ctx, NULL, NULL, filename.c_str()) < 0) {
		return false;
	}
	_out_fmt = _fmt_ctx->oformat;

	//[3]!打开输出文件
	if (avio_open(&_fmt_ctx->pb, filename.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
		//printf("output file open failed.\n");
		return false;
	}

	//[4]!创建h264视频流，并设置参数
	_video_stream = avformat_new_stream(_fmt_ctx, _video_codec);
	if (_video_stream == NULL) {
		//printf("failed create new video stream.\n");
		//break;
		return false;
	}
	_video_stream->time_base.den = _config.fps;
	_video_stream->time_base.num = 1;

	//[5]!编码参数相关
	AVCodecParameters* codecPara = _fmt_ctx->streams[_video_stream->index]->codecpar;
	codecPara->codec_type = AVMEDIA_TYPE_VIDEO;
	codecPara->width = _config.width;
	codecPara->height = _config.height;

	//[6]!查找编码器
	_video_codec = avcodec_find_encoder(_out_fmt->video_codec);
	if (_video_codec == NULL) {
		//printf("Cannot find any endcoder.\n");
		//break;
		return false;
	}

	//[7]!设置编码器内容
	_codec_ctx = avcodec_alloc_context3(_video_codec);
	avcodec_parameters_to_context(_codec_ctx, codecPara);
	if (_codec_ctx == NULL) {
		//printf("Cannot alloc context.\n");
		//break;
		return false;
	}

	_codec_ctx->codec_id = _out_fmt->video_codec;
	_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	_codec_ctx->width = _config.width;
	_codec_ctx->height = _config.height;
	_codec_ctx->time_base.num = 1;
	_codec_ctx->time_base.den = _config.fps;
	if (_config.bitrate > 0) {
		_codec_ctx->bit_rate = _config.bitrate;
	}
	else {
		_codec_ctx->bit_rate = 400000;
	}
	_codec_ctx->gop_size = 12;

	if (_codec_ctx->codec_id == AV_CODEC_ID_H264) {
		_codec_ctx->qmin = 10;
		_codec_ctx->qmax = 51;
		_codec_ctx->qcompress = (float)0.6;
	}
	if (_codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		_codec_ctx->max_b_frames = 2;
	if (_codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
		_codec_ctx->mb_decision = 2;
	//[7]!

	//[8]!打开编码器
	if (avcodec_open2(_codec_ctx, _video_codec, NULL) < 0) {
		//printf("Open encoder failed.\n");
		//break;
		return false;
	}
	//[8]!

	av_dump_format(_fmt_ctx, 0, filename.c_str(), 1);//输出 输出文件流信息

	//[9] --写头文件
	int ret = avformat_write_header(_fmt_ctx, NULL);
	if (ret < 0) {
		return false;
	}
	return true;
}

int H264Muxer::encode(const char* y, const char* u, const char* v, int yLen, int uLen, int vLen)
{
	return 0;
}

void H264Muxer::close()
{
	//[11] --Flush encoder
	int ret = flush_encoder();
	if (ret < 0) {
		printf("flushing encoder failed!\n");
		//break;
	}
	ret = av_write_trailer(_fmt_ctx);
	if (ret != 0) {
		std::cout << "av_write_trailer failed, ret = " << ret << std::endl;
	}
	if (_codec_ctx) {
		avcodec_close(_codec_ctx);
	}

	if (_fmt_ctx) {
		avio_close(_fmt_ctx->pb);
		avformat_free_context(_fmt_ctx);
	}
}

int H264Muxer::flush_encoder() {
	int      ret;
	AVPacket* enc_pkt = av_packet_alloc();
	enc_pkt->data = NULL;
	enc_pkt->size = 0;

	if (!(_codec_ctx->codec->capabilities & AV_CODEC_CAP_DELAY))
		return 0;

	printf("Flushing stream #%u encoder\n", _video_stream->index);
	if ((ret = avcodec_send_frame(_codec_ctx, 0)) >= 0) {
		while (avcodec_receive_packet(_codec_ctx, enc_pkt) >= 0) {
			printf("success encoder 1 frame.\n");

			// parpare packet for muxing
			enc_pkt->stream_index = _video_stream->index;
			av_packet_rescale_ts(enc_pkt, _codec_ctx->time_base,
				_fmt_ctx->streams[_video_stream->index]->time_base);
			ret = av_interleaved_write_frame(_fmt_ctx, enc_pkt);
			if (ret < 0) {
				printf("av_interleaved_write_frame failed, ret = %d", ret);
				break;
			}
		}
	}

	return ret;
}