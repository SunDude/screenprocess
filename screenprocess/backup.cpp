
void debugtest() {
	// finds matching keypoint set and finds perspective too
	cv::Mat templ = cv::imread("resource\\freechest.png", CV_LOAD_IMAGE_COLOR), result;
	int matchmethod = 2; // [0, 6)

	while (!FIN) {
		// refresh captured image
		scraper.getscreenmat();
		RECT windim;
		if (!GetWindowRect(curhwnd, &windim)) return; // hwnd lost
		windim.left += ((windim.right - windim.left) * 4) / 12;
		windim.right -= ((windim.right - windim.left) * 5) / 12;
		scraper.setroi(windim);

		cv::Mat &img_1 = templ;
		cv::Mat &img_2 = scraper.roi;
		//cv::Mat img_1, img_2;
		//cv::resize(scraper.roi, img_1, cv::Size(round(scraper.roi.rows / 2.0f), round(scraper.roi.cols / 2.0f)), 0.5f, 0.5f);
		//cv::resize(templ, img_2, cv::Size(round(templ.rows / 2.0f), round(templ.cols / 2.0f)), 0.5f, 0.5f);

		//-- Step 1: Detect the keypoints using SURF Detector
		int minHessian = 400;

		cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create(minHessian);

		std::vector<cv::KeyPoint> keypoints_1, keypoints_2;

		detector->detect(img_1, keypoints_1);
		detector->detect(img_2, keypoints_2);

		//-- Draw keypoints
		cv::Mat img_keypoints_1, img_keypoints_2, tmpimg1, tmpimg2;

		//cv::cvtColor(img_1, tmpimg1, cv::COLOR_RGBA2RGB);
		cv::cvtColor(img_2, tmpimg2, cv::COLOR_RGBA2RGB);

		//drawKeypoints(tmpimg1, keypoints_1, img_keypoints_1, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
		//drawKeypoints(tmpimg2, keypoints_2, img_keypoints_2, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);

		///cv::cvtColor(tmpimg1, img_1, cv::COLOR_RGB2RGBA);
		///cv::cvtColor(tmpimg2, img_2, cv::COLOR_RGB2RGBA);

		//-- Show detected (drawn) keypoints
		//cv::resize(img_keypoints_1, tmpimg1, cv::Size(round(img_keypoints_1.cols / 2.0f), round(img_keypoints_1.rows / 2.0f)));
		///cv::resize(img_keypoints_2, tmpimg2, cv::Size(round(img_keypoints_2.cols / 2.0f), round(img_keypoints_2.rows / 2.0f)));
		//imshow("Keypoints 1", tmpimg1);
		//imshow("Keypoints 2", tmpimg2);

		//-- Step 2: Calculate descriptors (feature vectors)
		cv::Ptr<cv::DescriptorExtractor> extractor = cv::xfeatures2d::SURF::create(minHessian);

		cv::Mat descriptors_1, descriptors_2;

		extractor->compute(img_1, keypoints_1, descriptors_1);
		extractor->compute(tmpimg2, keypoints_2, descriptors_2);

		//-- Step 3: Matching descriptor vectors using FLANN matcher
		cv::FlannBasedMatcher matcher;
		std::vector< cv::DMatch > matches;
		matcher.match(descriptors_1, descriptors_2, matches);

		double max_dist = 0; double min_dist = 100;

		//-- Quick calculation of max and min distances between keypoints
		for (int i = 0; i < descriptors_1.rows; i++)
		{
			double dist = matches[i].distance;
			if (dist < min_dist) min_dist = dist;
			if (dist > max_dist) max_dist = dist;
		}

		printf("-- Max dist : %f \n", max_dist);
		printf("-- Min dist : %f \n", min_dist);

		//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
		//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
		//-- small)
		//-- PS.- radiusMatch can also be used here.
		std::vector< cv::DMatch > good_matches;
		for (int i = 0; i < descriptors_1.rows; i++)
		{
			if (matches[i].distance <= max(2 * min_dist, 0.02))
			{
				good_matches.push_back(matches[i]);
			}

		}
		//-- Draw only "good" matches
		cv::Mat img_matches;
		cv::drawMatches(img_1, keypoints_1, tmpimg2, keypoints_2,
			good_matches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1),
			std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

		// determing position of match
		std::vector<cv::Point2f> pt1;
		std::vector<cv::Point2f> pt2;

		for (int i = 0; i < good_matches.size(); i++)
		{
			//-- Get the keypoints from the good matches
			pt1.push_back(keypoints_1[good_matches[i].queryIdx].pt);
			pt2.push_back(keypoints_2[good_matches[i].trainIdx].pt);
		}

		cv::Mat H = cv::findHomography(pt1, pt2, CV_RANSAC);

		//-- Get the corners from the image_1 ( the object to be "detected" )
		std::vector<cv::Point2f> i1_corners(4);
		i1_corners[0] = cvPoint(0, 0); i1_corners[1] = cvPoint(img_1.cols, 0);
		i1_corners[2] = cvPoint(img_1.cols, img_1.rows); i1_corners[3] = cvPoint(0, img_1.rows);
		std::vector<cv::Point2f> i2_corners(4);

		perspectiveTransform(i1_corners, i2_corners, H);

		//-- Draw lines between the corners (the mapped object in the scene - image_2 )
		line(img_matches, i2_corners[0] + cv::Point2f(img_1.cols, 0), i2_corners[1] + cv::Point2f(img_1.cols, 0), cv::Scalar(0, 255, 0), 4);
		line(img_matches, i2_corners[1] + cv::Point2f(img_1.cols, 0), i2_corners[2] + cv::Point2f(img_1.cols, 0), cv::Scalar(0, 255, 0), 4);
		line(img_matches, i2_corners[2] + cv::Point2f(img_1.cols, 0), i2_corners[3] + cv::Point2f(img_1.cols, 0), cv::Scalar(0, 255, 0), 4);
		line(img_matches, i2_corners[3] + cv::Point2f(img_1.cols, 0), i2_corners[0] + cv::Point2f(img_1.cols, 0), cv::Scalar(0, 255, 0), 4);


		//-- Show detected matches
		cv::imshow("Display window", img_matches);
		//cv::imshow("Display window", img_1);

		for (int i = 0; i < (int)good_matches.size(); i++)
		{
			printf("-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx);
		}

		Sleep(50);
	}
}

/// checking similarity http://docs.opencv.org/2.4/doc/tutorials/gpu/gpu-basics-similarity/gpu-basics-similarity.html
