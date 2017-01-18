#pragma once
#include "Header.h"

class screenscraper
{
public:
	screenscraper();
	~screenscraper();
	void screenscraper::reset();
	HBITMAP getscreen();
	void getscreenmat();
	void errhandler(char *str);
	PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp);
	void CreateBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);
	void saveBmp(HBITMAP hBmp, LPTSTR fname);
	void setroi(RECT region);

	cv::Mat screen;
	cv::Mat roi;
	int offx, offy;
};

