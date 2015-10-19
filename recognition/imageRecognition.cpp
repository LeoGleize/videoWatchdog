/*
 * imageRecognition.cpp
 *
 *  Created on: Oct 19, 2015
 *      Author: fcaldas
 */

#include "imageRecognition.h"
#include <iostream>

namespace imageRecognition {

	bool isImageBlackScreenOrZapScreen(cv::Mat &img, const cv::Vec3b &thresholdColor){
		cv::Mat subrect(img, cv::Rect(5,150,img.cols-10,720));
		cv::Mat grayScale;
		double minVal, maxVal;
		cv::Point minloc, maxloc;
		cv::cvtColor(subrect, grayScale, CV_BGR2GRAY);
		cv::minMaxLoc(grayScale, &minVal, &maxVal, &minloc, &maxloc);
		cv::Vec3b color = subrect.at<cv::Vec3b>(cv::Point(maxloc));
		if(color.val[0] <= thresholdColor.val[0] &&
		   color.val[1] <= thresholdColor.val[1] &&
		   color.val[2] <= thresholdColor.val[2])
			return true;
		return false;
	}

} /* namespace RestServer */
