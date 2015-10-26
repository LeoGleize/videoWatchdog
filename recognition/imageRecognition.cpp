/*
x * imageRecognition.cpp
 *
 *  Created on: Oct 19, 2015
 *      Author: fcaldas
 */

#include "imageRecognition.h"
#include <iostream>
#include <opencv2/features2d.hpp>

namespace imageRecognition {

	bool isImageBlackScreenOrZapScreen(cv::Mat &img, const cv::Vec3b &thresholdColor){
		cv::Mat subrect(img, cv::Rect(5,150,img.cols-10,600));
		cv::Mat grayScale;
		double minVal, maxVal;
		cv::Point minloc, maxloc;
		cv::cvtColor(subrect, grayScale, CV_BGR2GRAY);
		cv::minMaxLoc(grayScale, &minVal, &maxVal, &minloc, &maxloc);
		cv::Vec3b color = subrect.at<cv::Vec3b>(cv::Point(maxloc));
		if((int)color.val[0] <= (int)thresholdColor.val[0] &&
		   (int)color.val[1] <= (int)thresholdColor.val[1] &&
		   (int)color.val[2] <= (int)thresholdColor.val[2]){
			return true;
		}
		return false;
	}

	/**
	 * Searches for the cv::Mat templ on the cv::Mat img, returns the best match
	 * found on img on an objMatch object with the score of the best match and position
	 * scale is always one since this method ONLY SEARCHES for images with the same
	 * dimensions
	 */
	objMatch matchTemplateSameScale(cv::Mat &img, cv::Mat &templ){
		objMatch match;
		cv::Mat result;
		cv::matchTemplate(img,templ,result,CV_TM_CCORR_NORMED);
		double minv, maxv;
		cv::Point min, max;
		cv::minMaxLoc(result, &minv, &maxv, &min,&max);
		match.matchScore = maxv;
		match.pos.x = max.x;
		match.pos.y = max.y;
		match.pos.width = templ.cols;
		match.pos.height = templ.rows;
		match.scale = 1;
		return match;
	}

}
