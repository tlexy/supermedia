#include "h264_muxer.h"


int main()
{
	H264MuxerConfig config;
	config.height = 1080;
	config.width = 1920;
	config.fps = 30;
	H264Muxer muxer(config);
	muxer.open("test.h264");

	muxer.close();


	return 0;
}