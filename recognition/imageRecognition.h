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
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

/*
 * Suite of functions for image recognition and processing
 * used to recognize parts of the interface of the webApp
 */

namespace imageRecognition {

/**
 * Checks if a image has a black background, will cut a rectangle
 * in the image ignoring the bottom menu and top screen with time
 * and calculate a Norm of the image if smaller than threshold returns
 * true
 */
bool isImageBlackScreenOrZapScreen(cv::Mat &img,const cv::Vec3b &thresholdColor);

} /* namespace RestServer */

#endif /* IMAGERECOGNITION_H_ */
