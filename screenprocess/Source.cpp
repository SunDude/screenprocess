#include "Header.h"

HHOOK hHook{ NULL };

bool flag = false, FIN = false;
HWND curhwnd;

screenscraper scraper;
mousecontroller mouse;
clock_t last=0;

LRESULT CALLBACK MyLowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
	if (nCode == HC_ACTION) { // if processable
		DWORD keyState = ((PKBDLLHOOKSTRUCT)lParam)->vkCode; // get vkCode of keyboard input event struct 

		if (keyState == VK_F9) {
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
				if (DEBUG) std::cout << "KeyDown" << std::endl;
				// toggle processes
				flag = !flag;
				std::cout << "Process " << (flag?"started":"paused") << std::endl;
			}

			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				if (DEBUG) std::cout << "KeyUp" << std::endl;
			}
		}

		if (keyState == VK_F10) {
			// quit application
			if (DEBUG) std::cout << "F10" << std::endl;
			FIN = true;
			//HWND myhwnd = GetConsoleWindow();
			//SendMessage(myhwnd, WM_CLOSE, NULL, NULL);
			PostQuitMessage(0);
		}

		if (keyState == VK_F8) {
			if ((clock() - last) * 5 / CLOCKS_PER_SEC) {
				mouse.LeftClick();
				last = clock();
			}
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

/*void display() {
	while (!FIN) {
		while (flag) {
			cv::Mat output, src = scraper.screen;
			RECT windim;
			if (!GetWindowRect(curhwnd, &windim)) break; // hwnd lost

			cv::resize(src, output, cv::Size(round(src.cols / 2.0f), round(src.rows / 2.0f)));

			cv::imshow("Display window", output);
		}
	}
}*/

bool hasout = false;

void process() {
	std::cout << "== processing thread started ==" << std::endl;
	while (!FIN) {
		if (flag) {
			// get screencap
			scraper.getscreenmat();
			if (DEBUG) {
				cv::imwrite("tmp.jpg", scraper.screen);
			}

			// get application of interest + offsets
			RECT windim;
			if (!GetWindowRect(curhwnd, &windim)) continue; // hwnd lost

			scraper.setroi(windim);
			// update display

			cv::Mat output;
			cv::resize(scraper.roi, output, cv::Size(round(scraper.roi.cols / 2.0f), round(scraper.roi.rows / 2.0f)));

			if (output.data) cv::imshow("Display window", output);
			output.release();
		}
		else {

		}
		Sleep(500);
	}
	std::cout << "== processing thread terminated ==" << std::endl;
}

cv::Mat todisplay, croppeddisplay;

cv::Point findtempl(cv::Mat scene, cv::Mat templ, double threshold = 0.85f) {
	if (FIN) return cv::Point(-1, -1);
	int matchmethod = CV_TM_CCOEFF_NORMED; // [0, 6)
	cv::Mat result;

	// initialise what's been displayed
	todisplay.release();
	scraper.roi.copyTo(todisplay);

	// Create the result matrix
	int result_cols = scraper.roi.cols - templ.cols + 1;
	int result_rows = scraper.roi.rows - templ.rows + 1;

	try {
		result.create(result_rows, result_cols, CV_32FC1);

		/// Do the Matching and Normalize

		int d1 = scraper.roi.depth();
		int c1 = scraper.roi.channels();
		int d2 = templ.depth();
		int c2 = templ.channels();

		if (d1 != d2 || c1 != c2) {
			std::cout << "d1/d2: " << d1 << "/" << d2 << " c1/c2:" << c1 << "/" << c2 << std::endl;
			throw "depth/channel mismatch";
		}

		matchTemplate(scraper.roi, templ, result, matchmethod); // last argument (matchmethod) can be [0, 6) 

																//normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat()); // we don't need to normalise again as it's included in match method

																/// Localizing the best match with minMaxLoc
		double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
		cv::Point matchLoc;

		minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

		/// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
		if (matchmethod == CV_TM_SQDIFF || matchmethod == CV_TM_SQDIFF_NORMED) {
			matchLoc = minLoc;
			if (DEBUG) std::cout << "minvalue of : " << minVal << std::endl;
		}
		else {
			// CV_TM_CCOEFF_NORMED - seems to be up to 0.3~4 for non-matches

			matchLoc = maxLoc;
			if (DEBUG) std::cout << "maxvalue of : " << maxVal << std::endl;
		}

		// Show me what you got if similar

		rectangle(todisplay, matchLoc, cv::Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), cv::Scalar(255, 0, 255), 4, 8, 0);
		//rectangle(result, matchLoc, cv::Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), cv::Scalar(255, 0, 255), 2, 8, 0);

		if (DEBUG && templ.data) cv::imshow("Debug display", templ);
		if (todisplay.data) {
			//cv::Rect roirect = cv::Rect(max(0, matchLoc.x - templ.cols), max(0, matchLoc.y - templ.rows), 0, 0);
			//roirect = cv::Rect(roirect.x, roirect.y, min(todisplay.cols-roirect.x, 3 * templ.cols), min(todisplay.rows-roirect.y, 3 * templ.rows));
			//croppeddisplay = todisplay(roirect);
			//cv::imshow("Display window", croppeddisplay);
			cv::resize(todisplay, todisplay, cv::Size(), 0.5f, 0.5f, cv::INTER_AREA);
			cv::imshow("Display window", todisplay);
		}

		if (maxVal > threshold) {
			return maxLoc;
		}
		return cv::Point(-1, -1);
	}
	catch (std::exception exp) {
		std::cout << exp.what() << std::endl;
	}
	catch (...) {
		// ?
		std::cout << "unknown error" << std::endl;
	}
}

int hourstorun=0;

void debugtest() {
	// http://docs.opencv.org/2.4/doc/tutorials/imgproc/histograms/template_matching/template_matching.html
	// matches closest, but does not give measurment of similarity?

	cv::Mat data[6];
	data[0] = cv::imread("resource\\freechest.png", CV_LOAD_IMAGE_COLOR);
	data[1] = cv::imread("resource\\taptounlock.png", CV_LOAD_IMAGE_COLOR);
	data[2] = cv::imread("resource\\startunlock.png", CV_LOAD_IMAGE_COLOR);
	data[3] = cv::imread("resource\\freeclick2.png", CV_LOAD_IMAGE_COLOR);
	data[4] = cv::imread("resource\\dc.png", CV_LOAD_IMAGE_COLOR);
	for (int i = 0; i < 5; i++) {
		if (data[i].empty()) continue;
		cvtColor(data[i], data[5], CV_RGB2RGBA);
		data[i] = data[5];
	}
	

	while (!FIN) {
		while (!flag && !FIN) {
			Sleep(100);
		}
		if (FIN) break;
		// refresh captured image
		scraper.getscreenmat();
		RECT windim, areaofinterest;
		if (!GetWindowRect(curhwnd, &windim)) return; // hwnd lost
		areaofinterest = windim;
		areaofinterest.left += ((windim.right - windim.left) * 4.0f) / 12.0f;
		areaofinterest.right -= ((windim.right - windim.left) * 3.5f) / 12.0f;

		// std::cout << "windows dimension (left, top, right, bottom): " << windim.left << ", " << windim.top << ", " << windim.right << ", " << windim.bottom << std::endl;
		
		if (areaofinterest.left >= areaofinterest.right) {
			std::cout << "windim left/right crossed" << std::endl;
		}
		if (areaofinterest.left >= windim.right || areaofinterest.right <= windim.left) {
			std::cout << "windim left/right crossed" << std::endl;
		}
		scraper.setroi(areaofinterest);

		cv::Point freechestloc = findtempl(scraper.roi, data[0]);
		Sleep(200);
		cv::Point unlockableloc = findtempl(scraper.roi, data[1]);
		Sleep(200);
		cv::Point startunlockloc = findtempl(scraper.roi, data[2]);
		Sleep(200);
		cv::Point clickthisloc = findtempl(scraper.roi, data[3], 0.7f);
		Sleep(200);
		cv::Point dc = findtempl(scraper.roi, data[4]);
		Sleep(200);
		
		if (DEBUG) {
			std::cout << "freechest at: " << freechestloc.x << ", " << freechestloc.y << std::endl;
			std::cout << "unlockable at: " << unlockableloc.x << ", " << unlockableloc.y << std::endl;
			std::cout << "startunlock at: " << startunlockloc.x << ", " << startunlockloc.y << std::endl;
			std::cout << "freeclick at: " << clickthisloc.x << ", " << clickthisloc.y << std::endl;
		}

		cv::Point notfound(-1, -1);

		clock_t haveran = clock();
		int hoursran = haveran / CLOCKS_PER_SEC / 60 / 60;
		// std::cout << "have ran for seconds: " << haveran / CLOCKS_PER_SEC << std::endl;

		if (dc != notfound) {
			if (hoursran >= hourstorun) {
				flag = false;
				std::cout << "disconnected, puasing..." << std::endl;
				continue;
			}
			else {
				mouse.generateclick(windim.left + 1045, windim.top + 45, 10, 10);
				std::cout << "reconnecting..." << std::endl;
			}
		}
		else if (startunlockloc != notfound) {
			mouse.generateclick(startunlockloc.x + scraper.offx, startunlockloc.y + scraper.offy, data[2].cols, data[2].rows);
			std::cout << "start chest unlock" << std::endl;
		}
		else if (unlockableloc != notfound) {
			mouse.generateclick(unlockableloc.x + scraper.offx, unlockableloc.y + scraper.offy, data[1].cols, data[1].rows);
			std::cout << "located unlockable chest ready to start" << std::endl;
		}
		else if (freechestloc != notfound) {
			mouse.generateclick(freechestloc.x + scraper.offx, freechestloc.y + scraper.offy, data[0].cols, data[0].rows);
			std::cout << "free chest ready to collect" << std::endl;
		}
		else if (clickthisloc != notfound) {
			// mouse.generateclick(clickthisloc.x + scraper.offx, clickthisloc.y + scraper.offy, data[3].cols, data[3].rows);
			std::cout << "waiting for 5 seconds..." << std::endl;
			Sleep(4000);
		}
		else {
			mouse.generateclick(windim.left + 1045, windim.top + 45, 10, 10);
			std::cout << "in transition screen" << std::endl;
		}

		Sleep(1000);
	}
}
//*/

void loading() {
	std::cout << std::endl << "Loading";
	Sleep(150);
	for (int i = 0; i < 5; i++) {
		std::cout << ".";
		Sleep(std::rand() % 100 + 150);
	}
	std::cout << "done" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
	std::cout << "*****" << std::endl;
	std::cout << "IMPORTANT: Make sure to run bluestacks in 1600x900 resolution" << std::endl;
	std::cout << "Do this by click cog on top right -> preferences -> resolution" << std::endl;
	std::cout << "*****" << std::endl;
	std::cout << std::endl;
	std::cout << "Please open bluestacks";
	Sleep(1000);
	while ((curhwnd = FindWindow(NULL, "BlueStacks App Player")) == NULL) {
		std::cout << ".";
		Sleep(1000);
	}
	std::cout << std::endl << "Bluestacks found!" << std::endl;
	std::cout << std::endl;
	std::cout << "=====" << std::endl << "Usage:" << std::endl << "* F9: start/pause bot" << std::endl << "* F10: exit" << std::endl << "=====" << std::endl;
	
	loading();

	std::cout << "Please enter number of hours to run with reconnect (or 0 for none): ";
	std::cin >> hourstorun;

	cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
	//cv::namedWindow("Keypoints 1", cv::WINDOW_AUTOSIZE);// Create a window for display.
	//cv::namedWindow("Keypoints 2", cv::WINDOW_AUTOSIZE);// Create a window for display.
	if (DEBUG) cv::namedWindow("Debug display", cv::WINDOW_AUTOSIZE);// Create a window for debug.

	// set up callback keyhook
	if (DEBUG) std::cout << "keyhook started" << std::endl;
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, MyLowLevelKeyboardProc, NULL, 0);
	if (hHook == NULL) {
		std::cout << "Keyboard hook failed!" << std::endl;
	}

	//std::thread first(process);

	///*
	std::thread second;
	second = std::thread(debugtest);
	///*/

	int ret;
	MSG msg;
	while (ret = GetMessage(&msg, NULL, 0, 0)) {
		if (ret == -1) {
			// error
			std::cout << "GetMessage() error" << std::endl;
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::cout << "exiting..." << std::endl;

	//first.join();

	second.join(); /// testing

	UnhookWindowsHookEx(hHook);

	return 0;
}