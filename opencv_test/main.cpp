#include <opencv2/opencv.hpp>
#include <chrono>

using namespace cv;

int64_t getTimeStampMilli()
{
#ifdef _WIN32
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	auto tt = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
	return tt.count();
#else
	gettimeofday(&gtv, NULL);
	int64_t micros = gtv.tv_sec * 1000 + gtv.tv_usec / 1000;
	return micros;
#endif
}

int video_capture() 
{
	int camera_id = 0;
	cv::VideoCapture vc(camera_id);
	//先设置采集格式
	//vc.set(cv::CV_CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
	////再设置高清采集分辨率
	//vc.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
	//vc.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);
	bool flag = vc.isOpened();
	if (!flag) {
		std::cout << "无法打开摄像头" << std::endl;
	}
	
	while (true) {
		cv::Mat frame;
		vc >> frame;

		if (frame.empty()) 
		{
			break;
		}
		int64_t t = getTimeStampMilli();
		cv::imshow("capture", frame);
		int64_t t2 = getTimeStampMilli();
		auto t1 = t2 - t;
		char ch = (char)cv::waitKey(30);
		if (ch == 27)
		{
			break;
		}
	}
	return 0;
}

void show_image() {
	cv::Mat test = cv::imread("vx_20240911165841_2.jpg");
	cv::imshow("beauty", test);
	cv::waitKey(0);
}

int main() 
{
	video_capture();
	return 0;
}