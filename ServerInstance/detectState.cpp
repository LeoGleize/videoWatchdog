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
#include <algorithm>
#include <ctime>

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
	  && params.has_field("eventType") && params["eventType"].is_array()
	  && params.has_field("timeEvent") && params["timeEvent"].is_integer()) {
		if (params["timeAnalysis"].as_integer() < 2000) {
			reply["error"] = 1;
			reply["message"] = web::json::value::string("'timeAnalysis' needs to be greater than 2000ms");
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
			std::list<outputState> eventsSearch;
			//go through list of events being searched
			for(int i = 0; i < params["eventType"].as_array().size(); i++){
				outputState oState = getStateByName(params["eventType"].as_array().at(i).as_string());
				eventsSearch.push_back(oState);
				if(oState == S_NOT_FOUND){
					reply["error"] = 1;
					std::string message = "Invalid value for eventType, should be: LIVE / FREEZE / BLACK";
					reply["message"] = web::json::value::string(message);
					return;
				}
			}
			eventsSearch.unique();
			__detectScreenState state = detectStateChange(eventsSearch,
														  params["timeAnalysis"].as_integer(),
														  params["timeEvent"].as_integer(),
														  count);
			//process output
			for(int i = 0; i < state.found.size(); i++){
				char buffer[32];
				json::value tempObj;

				tempObj["foundState"] = web::json::value::string(getNameOfState(state.found[i]));
				tempObj["observedFor"] = web::json::value::number(state.tlast[i]);
				struct tm * timeinfo;
				timeinfo = localtime(&(state.timestamps[i]));
				std::strftime(buffer, 32, "%d.%m.%Y %H:%M:%S", timeinfo);
				tempObj["when"] = web::json::value::string(buffer);
				reply["foundState"][i] = tempObj;
			}
			reply["error"] = 0;
		}
	} else {
		reply["error"] = 1;
		std::string message = "This request needs three parameters: 'timeAnalysis' the time in" \
						      " milliseconds that the video output will be evaluated, 'eventType': " \
						      "the type of events we are looking for in an array and 'timeEvent': how long the " \
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

__detectScreenState detectStateChange(std::list<outputState>  &stateSearch,
									  unsigned int timeAnalysis,
									  unsigned int timeEvent,
									  bool countOc) {
	__detectScreenState screenDetection;
	bool searchBlack  = (std::find(stateSearch.begin(), stateSearch.end(), S_BLACK_SCREEN)  != stateSearch.end());
	bool searchFreeze = (std::find(stateSearch.begin(), stateSearch.end(), S_FREEZE_SIGNAL) != stateSearch.end());
	bool searchLive   = (std::find(stateSearch.begin(), stateSearch.end(), S_LIVE_SIGNAL)   != stateSearch.end());
	outputState lastCapturedState = S_NOT_FOUND;

	//number of frames used for status calculation 1 frame / 100ms
	int nFrames = timeEvent / 100;
	if(nFrames > 15)
		nFrames = 15; //saturate otherwise we'll have too many comparisons

	int dt_interFramems =  timeEvent / nFrames;

	unsigned int nReadings = timeAnalysis / dt_interFramems;
	std::deque<cv::Mat> matList;
	timeval t0, t1, tStart;

	gettimeofday(&tStart, NULL);
	long culmulativeDelay = 0;
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
			for (unsigned int i = 1; i < matList.size(); i++) {
				double n1,n2;
				cv::subtract(matList[0], matList[i], subtractionResult);
				n1 = cv::norm(subtractionResult);
				cv::subtract(matList[i], matList[0], subtractionResult);
				n2 = cv::norm(subtractionResult);
				maxDiff = (n1 > maxDiff) ? n1 : maxDiff;
				maxDiff = (n2 > maxDiff) ? n2 : maxDiff;
			}
			maxDiff = maxDiff / npixel;

			if (searchLive && maxDiff > freezeThreshold) {

				if(lastCapturedState == S_LIVE_SIGNAL){
					//just increment last capture
					screenDetection.tlast[screenDetection.tlast.size() - 1] += dt_interFramems;
				}else{
					screenDetection.timestamps.push_back(std::time(NULL));
					screenDetection.found.push_back(S_LIVE_SIGNAL);
					screenDetection.tlast.push_back(timeEvent);
				}
				lastCapturedState = S_LIVE_SIGNAL;
			} else if (searchBlack
					&& maxDiff < freezeThreshold
			        && imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				if(lastCapturedState == S_BLACK_SCREEN){
					screenDetection.tlast[screenDetection.tlast.size() - 1] += dt_interFramems;
				}else{
					screenDetection.timestamps.push_back(std::time(NULL));
					screenDetection.found.push_back(S_BLACK_SCREEN);
					screenDetection.tlast.push_back(timeEvent);
				}
				lastCapturedState = S_BLACK_SCREEN;
			} else if (searchFreeze
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				if(lastCapturedState == S_FREEZE_SIGNAL){
					screenDetection.tlast[screenDetection.tlast.size() - 1] += dt_interFramems;
				}else{
					screenDetection.timestamps.push_back(std::time(NULL));
					screenDetection.found.push_back(S_FREEZE_SIGNAL);
					screenDetection.tlast.push_back(timeEvent);
				}
				lastCapturedState = S_FREEZE_SIGNAL;
			}else{
				lastCapturedState = S_NOT_FOUND;
			}
		}
		gettimeofday(&t1, NULL);

		if(countOc == false && screenDetection.found.size() > 0)
			return screenDetection;

		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 + culmulativeDelay < 1000 * dt_interFramems){
			usleep(	1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 - culmulativeDelay));
			culmulativeDelay = 0;
		}else{
			culmulativeDelay = culmulativeDelay - (1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
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

outputState getStateByName(std::string name){
	if(name == "LIVE")
		return S_LIVE_SIGNAL;
	else if(name == "FREEZE")
		return S_FREEZE_SIGNAL;
	else if(name == "BLACK")
		return S_BLACK_SCREEN;
	else if(name == "NOSIGNAL")
		return S_NO_VIDEO;
	return S_NOT_FOUND;
}

};
