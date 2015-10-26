/*
 * freeze.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: fcaldas
 */

#include "ServerInstance.h"
#include "../recognition/imageRecognition.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <queue>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer {

void wwwdetectState(http_request request) {
	json::value resp = request.extract_json().get();
	json::value answer;
	if (resp.has_field("timeAnalysis") && resp["timeAnalysis"].is_number()) {
		if (resp["timeAnalysis"].as_integer() > 400) {
			__screenState sState = getState(resp["timeAnalysis"].as_integer());
			answer["MaxDiffpPixel"] = sState.maxDiffppixel;
			answer["NormpPixel"] = sState.maxNormppixel;
			answer["status"] = web::json::value::string(getNameOfState(sState.oState));
		} else {
			answer["error"] = 1;
			answer["message"] = web::json::value::string(
					"Variable 'timeAnalysis' has to be greater than 400ms");
		}

	} else {
		answer["error"] = 1;
		answer["message"] =
				web::json::value::string(
						"This request needs one value: 'timeAnalysis' the time in milliseconds that the video output will be evaluated");
	}
	request.reply(status_codes::OK, answer);
}

void wwwdetectEvent(http_request request) {

	json::value params = request.extract_json().get();
	json::value reply;

	//check for parameters on received POST request
	if ( params.has_field("timeAnalysis") && params["timeAnalysis"].is_integer()
	  && params.has_field("eventType") && params["eventType"].is_string()
	  && params.has_field("timeEvent") && params["timeEvent"].is_integer()) {
		if (params["timeAnalysis"].as_integer() < 2000) {
			reply["error"] = 1;
			reply["message"] = web::json::value::string("'timeAnalysis' needs to be greater than 2000ms");
		} else if (params["eventType"].as_string() != "FREEZE"
				&& params["eventType"].as_string() != "LIVE"
				&& params["eventType"].as_string() != "BLACK") {
			reply["error"] = 1;
			reply["message"] = web::json::value::string("Valid 'eventType' are 'BLACK', 'LIVE' and 'FREEZE'");
		} else if (params["timeEvent"].as_integer() < 500
				|| params["timeEvent"].as_integer()	> params["timeAnalysis"].as_integer()) {
			reply["error"] = 1;
			std::string message = "'timeEvent' has to be bigger than 500 and smaller than 'timeAnalysis'";
			reply["message"] = web::json::value::string(message);
		} else {
			//if everything is ok we then proceed to launch the process
			bool count = false;
			if(params.has_field("count") && params["count"].is_boolean()){
				count = params["count"].as_bool();
			}
			outputState searchState;
			if(params["eventType"].as_string() == "FREEZE")
				searchState = S_FREEZE_SIGNAL;
			else if(params["eventType"].as_string() == "LIVE")
				searchState = S_LIVE_SIGNAL;
			else if(params["eventType"].as_string() == "BLACK")
				searchState = S_BLACK_SCREEN;
			__detectScreenState state = detectStateChange(searchState,
														  params["timeAnalysis"].as_integer(),
														  params["timeEvent"].as_integer(),
														  count);
			//process output
			for(int i = 0; i < state.found.size(); i++){
				json::value tempObj;
				tempObj["foundState"] = web::json::value::string(getNameOfState(state.found[i]));
				tempObj["timeFromStartMs"] = web::json::value::number(state.msFromStart[i]);
				reply["foundState"][i] = tempObj;
			}
			reply["error"] = 0;
		}
	} else {
		reply["error"] = 1;
		std::string message = "This request needs three parameters: 'timeAnalysis' the time in" \
						      " milliseconds that the video output will be evaluated, 'eventType' " \
						      "the type of event we are looking for and 'timeEvent': how long the " \
						      "event will need to be seen to get an occurrence";
		reply["message"] = web::json::value::string(message);
	}
	request.reply(status_codes::OK, reply);
}

/*
 *  Return the state of the screen after an dt_ms long analysis
 * 	during this analysis a frame will be captured every dt_interFramems
 */
__screenState getState(int dt_ms) {
	__screenState reply;
	int dt_interFramems = 100; //one shot per 100ms = we'll try to keep 10fps on average

	//threshold for a 1080p is 5000 this is done so there won't be
	//different results for different resolutions
	int nReadings = dt_ms / dt_interFramems;

	cv::Mat imgcmp, subResult;
	cv::Mat fimg;
	double maxDiff = 0, diff;
	timeval t0, t1;

	try{
		fimg = ServerInstance::cameraDeckLink->captureLastCvMat();
	}catch(const CardException &d){
		if(d.getExceptionType() == NO_INPUT_EXCEPTION)
			reply.oState = S_NO_VIDEO;
		return reply;
	}

	for (unsigned int i = 0; i < nReadings; i++) {
		gettimeofday(&t0, NULL);

		imgcmp = ServerInstance::cameraDeckLink->captureLastCvMat();
		cv::subtract(imgcmp, fimg, subResult);
		diff = cv::norm(subResult);
		maxDiff = (diff > maxDiff) ? diff : maxDiff;
		gettimeofday(&t1, NULL);
		//fix time: to keep an average of one frame per 50ms
		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000	< 1000 * dt_interFramems)
			usleep(1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
	}

	if (maxDiff / (fimg.rows * fimg.cols) < freezeThreshold
			&& imageRecognition::isImageBlackScreenOrZapScreen(fimg, blackThreshold)) {
		reply.oState = S_BLACK_SCREEN;
	} else if (maxDiff / (fimg.rows * fimg.cols) < freezeThreshold) {
		reply.oState = S_FREEZE_SIGNAL;
	} else {
		reply.oState = S_LIVE_SIGNAL;
	}
	reply.maxDiffppixel = maxDiff / (fimg.rows * fimg.cols);
	reply.maxNormppixel = cv::norm(fimg) / (fimg.rows * fimg.cols);
	return reply;
}

__detectScreenState detectStateChange(outputState stateSearch,
									  unsigned int timeAnalysis,
									  unsigned int timeEvent,
									  bool countOc) {
	__detectScreenState screenDetection;
	int dt_interFramems = 100; //one shot per 100ms = we'll try to keep 10fps on average
	int nFrames = timeEvent / 100;
	unsigned int nReadings = timeAnalysis / 100;
	std::deque<cv::Mat> matList;
	timeval t0, t1, tStart;
	unsigned int msFromStart;
	gettimeofday(&tStart, NULL);
	for (unsigned int i = 0; i < nReadings; i++) {
		gettimeofday(&t0, NULL);
		try{
			matList.push_back(ServerInstance::cameraDeckLink->captureLastCvMat());
		}catch(const CardException &e){
			usleep(	1000 * dt_interFramems);
			std::cout<<"Caught exception on detectState():"<<e.what()<<std::endl<<std::flush;
			continue;
		}
		if (matList.size() > nFrames) {
			matList.pop_front();
		}
		//deque is full therefore we can process
		if (matList.size() == nFrames) {
			double maxDiff = 0;
			unsigned int npixel = matList[0].cols * matList[0].rows;
			cv::Mat subtractionResult;
			for (int i = 1; i < matList.size(); i++) {
				double n1,n2;
				cv::subtract(matList[0], matList[i], subtractionResult);
				n1 = cv::norm(subtractionResult);
				cv::subtract(matList[i], matList[0], subtractionResult);
				n2 = cv::norm(subtractionResult);
				maxDiff = (n1 > maxDiff) ? n1 : maxDiff;
				maxDiff = (n2 > maxDiff) ? n2 : maxDiff;
			}
			maxDiff = maxDiff / npixel;

			if (stateSearch == S_LIVE_SIGNAL && maxDiff > freezeThreshold) {
				matList.clear();
				screenDetection.timestamps.push_back(std::time(NULL));
				screenDetection.found.push_back(S_LIVE_SIGNAL);
				gettimeofday(&t1, NULL);
				msFromStart = (t1.tv_sec - tStart.tv_sec)*1000 + (t1.tv_usec - tStart.tv_usec)/1000;
				screenDetection.msFromStart.push_back(msFromStart);
			} else if (stateSearch == S_BLACK_SCREEN
					&& maxDiff < freezeThreshold &&
					imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				matList.clear();
				screenDetection.timestamps.push_back(std::time(NULL));
				screenDetection.found.push_back(S_BLACK_SCREEN);
				gettimeofday(&t1, NULL);
				msFromStart = (t1.tv_sec - tStart.tv_sec)*1000 + (t1.tv_usec - tStart.tv_usec)/1000;
				screenDetection.msFromStart.push_back(msFromStart);
			} else if (stateSearch == S_FREEZE_SIGNAL
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				matList.clear();
				screenDetection.timestamps.push_back(std::time(NULL));
				screenDetection.found.push_back(S_FREEZE_SIGNAL);
				gettimeofday(&t1, NULL);
				msFromStart = (t1.tv_sec - tStart.tv_sec)*1000 + (t1.tv_usec - tStart.tv_usec)/1000;
				screenDetection.msFromStart.push_back(msFromStart);
			}
		}
		gettimeofday(&t1, NULL);

		if(countOc == false && screenDetection.found.size() > 0)
			return screenDetection;

		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 < 1000 * dt_interFramems){
			usleep(	1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
		}
	}
	return screenDetection;
}

std::string getNameOfState(outputState o){
	switch(o){
		case S_LIVE_SIGNAL:
			return "Live signal";
		case S_FREEZE_SIGNAL:
			return "Freeze";
		case S_NOT_FOUND:
			return "Output not found";
		case S_NO_VIDEO:
			return "No video";
		case S_BLACK_SCREEN:
			return "Black Screen";
	}
	return "Not found";
}

};
