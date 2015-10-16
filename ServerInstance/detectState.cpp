/*
 * freeze.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: fcaldas
 */

#include "ServerInstance.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <unistd.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer {

	void detectState(http_request request) {

		json::value resp = request.extract_json().get();
		json::value answer;
		if(resp.has_field("timeAnalysis") && resp["timeAnalysis"].is_number()){
			if(resp["timeAnalysis"].as_integer() > 400){
				__screenState sState = getState(resp["timeAnalysis"].as_integer());
				answer["MaxDiffpPixel"] = sState.maxDiffppixel;
				answer["NormpPixel"] = sState.maxNormppixel;
				if(sState.oState == S_LIVE_SIGNAL){
					answer["status"] = web::json::value::string("LIVE");
				}else if(sState.oState == S_FREEZE_SIGNAL){
					answer["status"] = answer["status"] = web::json::value::string("FREEZE");
				}else if(sState.oState == S_BLACK_SCREEN){
					answer["status"] = answer["status"] = web::json::value::string("BLACK");
				}
			}else{
				answer["error"] = 1;
				answer["message"] = web::json::value::string("Variable 'timeAnalysis' has to be greater than 400ms");
			}

		}else{
			answer["error"] = 1;
			answer["message"] = web::json::value::string("This request needs one value: 'timeAnalysis' the time in milliseconds that the video output will be evaluated");
		}
		request.reply(status_codes::OK, answer);
	}


	/*	Return the state of the screen after an dt_ms long analysis
	 * 	during this analysis a frame will be captured every dt_interFramems
	 */
	__screenState getState(int dt_ms){
		__screenState reply;
		int dt_interFramems = 50; //one shot per 50ms
		//threshold for a 1080p is 5000 this is done so there won't be a
		//in results for different resolutions
		double freezeThreshold = 5000.0/(1920*1080);
		double blackThreshold = 8000.0/(1920*1080);
		int nReadings = dt_ms / dt_interFramems;

		cv::Mat imgcmp, subResult;
		cv::Mat fimg;
		double maxDiff = 0, diff;
		//create a DEEP copy of the first image
		fimg = ServerInstance::cameraDeckLink->captureLastCvMat();
		for(int i = 0; i < nReadings; i++){
			usleep(1000* dt_interFramems);
			imgcmp = ServerInstance::cameraDeckLink->captureLastCvMat();
			cv::subtract(imgcmp, fimg, subResult);
			diff = cv::norm(subResult);
			maxDiff = (diff > maxDiff)?diff:maxDiff;
#ifdef DEBUG_IMAGES
			std::string fname = "test" + std::to_string(i) + "_" +  std::to_string((int)diff) + ".jpg";
			cv::imwrite(fname, subResult);
#endif
			std::cout << "Norm diff = " << diff << std::endl;
		}
		if(maxDiff/(fimg.rows * fimg.cols) < freezeThreshold &&
		   cv::norm(fimg)/(fimg.rows * fimg.cols) < blackThreshold){
			reply.oState = S_BLACK_SCREEN;
		}else if(maxDiff/(fimg.rows * fimg.cols) < freezeThreshold){
			reply.oState = S_FREEZE_SIGNAL;
		}else{
			reply.oState = S_LIVE_SIGNAL;
		}
		reply.maxDiffppixel = maxDiff/(fimg.rows * fimg.cols);
		reply.maxNormppixel = cv::norm(fimg)/(fimg.rows * fimg.cols);
		return reply;
	}


}
;
