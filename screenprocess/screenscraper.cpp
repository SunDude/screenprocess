#include "screenscraper.h"


screenscraper::screenscraper() {
	reset();
}


screenscraper::~screenscraper() {
}

void screenscraper::reset() {
	while (screen.data) screen.release();
	while (roi.data) roi.release();
	offx = offy = 0;
}

HBITMAP screenscraper::getscreen() {
	// get the device context of the screen
	HDC hScreenDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	// and a device context to put it in
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	int width = GetDeviceCaps(hScreenDC, HORZRES);
	int height = GetDeviceCaps(hScreenDC, VERTRES);

	// maybe worth checking these are positive values
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// get a new bitmap
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

	// clean up
	DeleteDC(hMemoryDC);
	DeleteDC(hScreenDC);
	return hBitmap;
}

void screenscraper::getscreenmat() {
	reset();
	HDC hwindowDC, hwindowCompatibleDC;
	HBITMAP hbwindow;
	BITMAPINFOHEADER  bi;

	hwindowDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	int width = GetDeviceCaps(hwindowDC, HORZRES);
	int height = GetDeviceCaps(hwindowDC, VERTRES);

	screen.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, width, height, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !

	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, screen.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow
	
	DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); // avoid memory leak
}

void screenscraper::setroi(RECT windim) {
	offx += windim.left;
	offy += windim.top;
	cv::Rect crop(max(0, windim.left), max(0, windim.top), min(screen.cols - windim.left, windim.right - windim.left), min(screen.rows - windim.top, windim.bottom - windim.top));
	// std::cout << "crop dimension (x, y, width, height): " << crop.x << ", " << crop.y << ", " << crop.width << ", " << crop.height << std::endl;

	while (roi.data) roi.release();
	roi = screen(crop);
}

void screenscraper::errhandler(char *str) {
	std::cout << str << std::endl;
}

PBITMAPINFO screenscraper::CreateBitmapInfoStruct(HBITMAP hBmp) {
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD    cClrBits;

	// Retrieve the bitmap color format, width, and height.  
	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp))
		errhandler("GetObject");

	// Convert the color format to a count of bits.  
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	// Allocate memory for the BITMAPINFO structure. (This structure  
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD  
	// data structures.)  

	if (cClrBits < 24)
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
			sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD) * (1 << cClrBits));

	// There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel 

	else
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
			sizeof(BITMAPINFOHEADER));

	// Initialize the fields in the BITMAPINFO structure.  

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

	// If the bitmap is not compressed, set the BI_RGB flag.  
	pbmi->bmiHeader.biCompression = BI_RGB;

	// Compute the number of bytes in the array of color  
	// indices and store the result in biSizeImage.  
	// The width must be DWORD aligned unless the bitmap is RLE 
	// compressed. 
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	// Set biClrImportant to 0, indicating that all of the  
	// device colors are important.  
	pbmi->bmiHeader.biClrImportant = 0;
	return pbmi;
}

void screenscraper::CreateBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC)
{
	HANDLE hf;                 // file handle  
	BITMAPFILEHEADER hdr;       // bitmap file-header  
	PBITMAPINFOHEADER pbih;     // bitmap info-header  
	LPBYTE lpBits;              // memory pointer  
	DWORD dwTotal;              // total count of bytes  
	DWORD cb;                   // incremental count of bytes  
	BYTE *hp;                   // byte pointer  
	DWORD dwTmp;

	pbih = (PBITMAPINFOHEADER)pbi;
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

	if (!lpBits)
		errhandler("GlobalAlloc");

	// Retrieve the color table (RGBQUAD array) and the bits  
	// (array of palette indices) from the DIB.  
	if (!GetDIBits(hDC, hBMP, 0, (WORD)pbih->biHeight, lpBits, pbi,
		DIB_RGB_COLORS))
	{
		errhandler("GetDIBits");
	}

	// Create the .BMP file.  
	hf = CreateFile(pszFile,
		GENERIC_READ | GENERIC_WRITE,
		(DWORD)0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);
	if (hf == INVALID_HANDLE_VALUE)
		errhandler("CreateFile");
	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
								// Compute the size of the entire file.  
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;

	// Compute the offset to the array of color indices.  
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD);

	// Copy the BITMAPFILEHEADER into the .BMP file.  
	if (!WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER),
		(LPDWORD)&dwTmp, NULL))
	{
		errhandler("WriteFile");
	}

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.  
	if (!WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER)
		+ pbih->biClrUsed * sizeof(RGBQUAD),
		(LPDWORD)&dwTmp, (NULL)))
		errhandler("WriteFile");

	// Copy the array of color indices into the .BMP file.  
	dwTotal = cb = pbih->biSizeImage;
	hp = lpBits;
	if (!WriteFile(hf, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL))
		errhandler("WriteFile");

	// Close the .BMP file.  
	if (!CloseHandle(hf))
		errhandler("CloseHandle");

	// Free memory.  
	GlobalFree((HGLOBAL)lpBits);
}

void screenscraper::saveBmp(HBITMAP hBmp, LPTSTR fname) {
	PBITMAPINFO bmpinfo = CreateBitmapInfoStruct(hBmp);
	CreateBMPFile(fname, bmpinfo, hBmp, CreateDC("DISPLAY", NULL, NULL, NULL));
}