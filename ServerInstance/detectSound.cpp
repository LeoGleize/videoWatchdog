/*
 * detectSound.cpp
 *
 *  Created on: Nov 2, 2015
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
	void wwwgetSound(web::http::http_request request){
		void *data;
		char *p;
		int sz;
		ServerInstance::cameraDeckLink->getAudioData(&data, &sz);
		p = (char *)data;
		std::string binsound(p , p + sz);
		free(p);
		request.reply(status_codes::OK, binsound, "Content-type: text/binary");
	}
} /* Namespace RestServer */
