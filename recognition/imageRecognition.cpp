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
		cv::Mat subrect(img, cv::Rect(5,150,img.cols-10,600));
		cv::Mat grayScale;
		double minVal, maxVal;
		cv::Point minloc, maxloc;
		cv::cvtColor(subrect, grayScale, CV_BGR2GRAY);
		cv::minMaxLoc(grayScale, &minVal, &maxVal, &minloc, &maxloc);
		cv::Vec3b color = subrect.at<cv::Vec3b>(cv::Point(maxloc));
//		cv::imwrite("subrect.png",subrect);
//		std::cout<<"Maxloc = "<<maxloc.x<<","<<maxloc.y<<std::endl;
//		std::cout<<"Max v = "<<(int)color.val[0] << ","<<(int)color.val[1] <<","<<(int)color.val[2]<<std::endl;
//		std::cout<<"Max vs= "<<color.val[0] << ","<<color.val[1] <<","<<color.val[2]<<std::endl;
//		std::cout<<"Max t = "<<(int)thresholdColor.val[0] << ","<<(int)thresholdColor.val[1] <<","<<(int)thresholdColor.val[2]<<std::endl;
//		std::cout<<"Max ts= "<<thresholdColor.val[0] << ","<<thresholdColor.val[1] <<","<<thresholdColor.val[2]<<std::endl;
		if((int)color.val[0] <= (int)thresholdColor.val[0] &&
		   (int)color.val[1] <= (int)thresholdColor.val[1] &&
		   (int)color.val[2] <= (int)thresholdColor.val[2]){
			return true;
		}
		return false;
	}

} /* namespace RestServer */
