#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}



//检测支持的硬件编解码器
int main()
{
	enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
	fprintf(stderr, "Available device types:\n");
	while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
		fprintf(stderr, " %s\n", av_hwdevice_get_type_name(type));
	fprintf(stderr, "\n");
	return 0;
}