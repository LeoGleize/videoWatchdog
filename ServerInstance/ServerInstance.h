/*
 * ServerInstance.h
 *
 *  Created on: Oct 15, 2015
 *      Author: fcaldas
 */

#ifndef SERVERINSTANCE_H_
#define SERVERINSTANCE_H_

#include "../blackmagic/cameradecklink.h"
#include <cpprest/http_listener.h>
#include <iostream>
#include <mutex>
#include <string>

namespace RestServer {

static CameraDecklink *cameraDeckLink;

class ServerInstance {

private:

public:
	ServerInstance();
	void stop();
	virtual ~ServerInstance();
};

/* Callbacks */
void grabScreen(web::http::http_request request);

} /* namespace RestServer */

#endif /* SERVERINSTANCE_H_ */
