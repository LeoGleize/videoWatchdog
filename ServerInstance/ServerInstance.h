/*
 * ServerInstance.h
 *
 *  Created on: Oct 15, 2015
 *      Author: fcaldas
 */

#ifndef SERVERINSTANCE_H_
#define SERVERINSTANCE_H_

#include "../blackmagic/cameradecklink.h"
#include "base64/base64.hpp"
#include <cpprest/http_listener.h>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <vector>
#include <mutex>
#include <string>

namespace RestServer {

class ServerInstance {

private:
	std::vector<web::http::experimental::listener::http_listener> myListeners;

public:
	ServerInstance();
	void stop();
	void start();
	virtual ~ServerInstance();

	static CameraDecklink *cameraDeckLink;
};

/* Callbacks */

struct __screenState{

};

void grabScreen(web::http::http_request request);
void detectState(web::http::http_request request);
__screenState getState(int dt_ms, int dt_interFramems);

} /* namespace RestServer */

#endif /* SERVERINSTANCE_H_ */
