/*
 * imageRecognition.h
 *
 *  Created on: Oct 19, 2015
 *      Author: fcaldas
 */

#ifndef IMAGERECOGNITION_H_
#define IMAGERECOGNITION_H_

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/features2d.hpp>
#include <cpprest/json.h>

/*
 * Suite of functions for image recognition and processing
 * used to recognize parts of the interface of the webApp
 */

namespace imageRecognition {

class objMatch{
public:
	double matchScore;
	cv::Rect pos;
	double scale;

	web::json::value toJSON(){
		web::json::value v;
		v["matchScore"] = web::json::value::number(matchScore);
		v["x"] = pos.x;
		v["y"] = pos.y;
		v["width"] = pos.width;
		v["height"] = pos.height;
		return v;
	}
};

/**
 * Checks if a image has a black background, will cut a rectangle
 * in the image ignoring the bottom menu and top screen with time
 * and calculate a Norm of the image if smaller than threshold returns
 * true
 */
bool isImageBlackScreenOrZapScreen(cv::Mat &img,const cv::Vec3b &thresholdColor);
objMatch matchTemplateSameScale(cv::Mat &img, cv::Mat &templ);

std::string getTextFromImage(cv::Mat &img);
bool bufferHasAudio(short *audioData, unsigned int nElements);

} /* namespace RestServer */

#endif /* IMAGERECOGNITION_H_ */
