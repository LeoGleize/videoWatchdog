/*
x * imageRecognition.cpp
 *
 *  Created on: Oct 19, 2015
 *      Author: fcaldas
 */

#include "imageRecognition.h"
#include <iostream>
//#include <opencv2/features2d.hpp>
#include <tesseract/baseapi.h>

namespace imageRecognition {

	bool isImageBlackScreenOrZapScreen(cv::Mat &img, const cv::Vec3b &thresholdColor){
		cv::Mat subrect(img, cv::Rect(5,0.14 * img.rows,img.cols-10,img.rows * 0.55));

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

	/*
	 * Performs OCR extracting text from image.
	 *
	 * Uses french libraries for character recognition.
	 */
	std::string getTextFromImage(cv::Mat &img){
		cv::Mat inverted, bw;
		//we invert the image and threshold it before
		//doing character recognition
		cv::bitwise_not(img,inverted);
		cv::cvtColor( inverted, bw, CV_RGB2GRAY);
		// Initialize tesseract-ocr
		tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	    if (api->Init(NULL, "fra") != 0){
	        std::cerr << "Could not initialize tesseract, text recognition is NOT avaiable.\n" << std::endl;
	    	throw std::runtime_error("OCR Engine not avaiable");
		}
	    api->SetImage((unsigned char*)bw.data, bw.cols, bw.rows, bw.channels(), bw.step1());
        api->SetRectangle(0, 0, bw.cols, bw.rows);
	    const char* out = api->GetUTF8Text();
	    std::string ret = out;
	    delete []out;
	    delete api;
		return ret;
	}
}
