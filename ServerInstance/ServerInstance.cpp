/*
 * ServerInstance.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: fcaldas
 */

#include "ServerInstance.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer {

	ServerInstance::ServerInstance() {
		std::cout<<"Starting server on http://localhost:8080"<<std::endl;
		web::uri myRoute("http://localhost:8080/image");
		myListeners.push_back(http_listener(myRoute));
		myListeners[0].support(methods::GET, wwwgrabScreen);

		web::uri myRoute2("http://localhost:8080/status");
		myListeners.push_back(http_listener(myRoute2));
		myListeners[1].support(methods::POST, wwwdetectState);

		web::uri myRoute3("http://localhost:8080/getScreenEvent");
		myListeners.push_back(http_listener(myRoute3));
		myListeners[2].support(methods::POST, wwwdetectEvent);
	}

	ServerInstance::~ServerInstance() {
		// TODO Auto-generated destructor stub
	}

	void ServerInstance::start() {
		try {
			std::cout<<"Adding routes to server"<<std::endl;
			for(unsigned int i = 0; i < myListeners.size(); i++){
				http_listener *listener = &(myListeners[i]);
				listener->open().then([listener]() {std::cout<<"";}).wait();
			}
		} catch (std::exception const & e) {
			std::cout << e.what() << std::endl;
		}
	}

	void ServerInstance::stop() {

	}

	/*Sends screen actually being show to client (cv format maybe change that to image?)*/
	void wwwgrabScreen(http_request request) {
		if(ServerInstance::cameraDeckLink != NULL){
			cv::Mat img = ServerInstance::cameraDeckLink->captureLastCvMat();
			std::vector<uchar> imageData;
			cv::imencode(".png",img,imageData);
			std::string data(imageData.begin(), imageData.end());
			request.reply(status_codes::OK, data, "Content-type: image/png");
		}else{
			json::value answer;
			answer["error"] = json::value::number(1);
			answer["message"] = json::value::string("Cannot access camera");
			request.reply(status_codes::OK, answer);
		}
	}

	CameraDecklink * ServerInstance::cameraDeckLink;
} /* namespace RestServer */
