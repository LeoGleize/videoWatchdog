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

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer {

	void detectState(http_request request) {

		json::value resp = request.extract_json().get();
		request.reply(status_codes::OK, resp);

	}

}
;
