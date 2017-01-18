#pragma once
#include "Header.h"

class imageprocesser
{
public:
	imageprocesser();
	~imageprocesser();
	std::vector<cv::Mat> data;
	std::vector<cv::Mat>::iterator loadimagetovector(const cv::String &fname, int flags);
};

