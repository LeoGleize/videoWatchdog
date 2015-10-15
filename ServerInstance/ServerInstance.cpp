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
#pragma comment(lib, "cpprest110_1_1")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer {

ServerInstance::ServerInstance() {
	web::uri myRoute("http://localhost:8080/rest");
	http_listener listener(myRoute);

	listener.support(methods::GET, grabScreen);

	try {
		listener.open().then([&listener]() {std::cout<<"starting to listen\n";}).wait();

		while (true)
			;
	} catch (std::exception const & e) {
		std::cout << e.what() << std::endl;
	}
}

ServerInstance::~ServerInstance() {
	// TODO Auto-generated destructor stub
}

void ServerInstance::stop() {

}

/*Sends screen actually being show to client*/
void grabScreen(http_request request) {
	request.reply(status_codes::OK, "test");
}

} /* namespace RestServer */
