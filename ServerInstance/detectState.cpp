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
		getState(1000,100);
		request.reply(status_codes::OK, resp);

	}


	/*	Return the state of the screen after an dt_ms long analysis
	 * 	during this analysis a frame will be captured every dt_interFramems
	 */
	__screenState getState(int dt_ms, int dt_interFramems){
		__screenState reply;
		int nReadings = dt_ms / dt_interFramems;

		cv::Mat imgcmp, subResult;
		cv::Mat fimg;
		//create a DEEP copy of the first image
		fimg = ServerInstance::cameraDeckLink->captureLastCvMat();
		for(int i = 0; i < nReadings; i++){
			usleep(1000* dt_interFramems);
			imgcmp = ServerInstance::cameraDeckLink->captureLastCvMat();
//			cv::subtract(imgcmp, fimg, subResult);
			double diff = cv::norm(subResult);
			std::string fname = "test" + std::to_string(i) + "_" +  std::to_string((int)diff) + ".jpg";
			cv::imwrite(fname, imgcmp);
			std::cout << "Norm diff = " << diff << std::endl;
		}
		return reply;
	}


}
;
