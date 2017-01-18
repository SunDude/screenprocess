#include "imageprocesser.h"



imageprocesser::imageprocesser()
{
}


imageprocesser::~imageprocesser()
{
}

std::vector<cv::Mat>::iterator imageprocesser::loadimagetovector(const cv::String &fname, int flags) {
	// returns iterator to inserted cv::Mat otherwise returns data.end() if fail
	cv::Mat datain;
	if (!(datain = cv::imread(fname, flags)).data) { // failed to read
		return data.end();
	}
	data.push_back(datain);
	return data.end() - 1;
}