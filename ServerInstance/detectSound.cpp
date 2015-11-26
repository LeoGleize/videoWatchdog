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

	/*
	 * Get RAW sound data, this will return an interleaved signed short
	 * audio file with 2 channels.
	 *
	 * You can transform it into an wav file using sox:
	 * sox -r48000 -e signed-integer -b 16 -c 2 DATA.raw out.wav
	 */
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
