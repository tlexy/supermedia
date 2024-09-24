#pragma once

#include <string>
#include <stdint.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

struct H264MuxerConfig
{
	int width;
	int height;
	int fps;
	int pix_format;
	int bitrate = 0;
};

class H264Muxer
{
public:
	H264Muxer(const H264MuxerConfig& config);
	bool open(const std::string& filename);
	int encode(const char* y, const char* u, const char* v, int yLen, int uLen, int vLen);
	void close();

private:
	int flush_encoder();

private:
	H264MuxerConfig _config;

	AVFormatContext* _fmt_ctx{NULL};
	AVCodecContext* _codec_ctx{ NULL };
	const AVOutputFormat* _out_fmt;
	AVStream* _video_stream;
	const AVCodec* _video_codec;
	AVFrame* _pic_frame;
	AVPacket* _pkt;
	int64_t _pts_counter{1123};
	char _errstr[64];
};

