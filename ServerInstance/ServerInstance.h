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
	enum outputState{S_LIVE_SIGNAL, S_FREEZE_SIGNAL, S_BLACK_SCREEN, S_NO_VIDEO, S_NOT_FOUND};
	std::string getNameOfState(outputState o);
	outputState getStateByName(std::string name);

	struct __screenState{
		//max norm calculated for differences between two frames divided by number of pixels
		double maxDiffppixel;
		//max norm calculated for first frame divided by number of pixels
		double maxNormppixel;
		outputState oState;
	};

	struct __detectScreenState{
		std::vector<outputState> found;
		std::vector<std::time_t> timestamps;
		std::vector<unsigned long> tlast;
	};

	const double freezeThreshold = 5000.0/(1920*1080);
	const cv::Vec3b blackThreshold(35,35,35);

	void wwwgrabScreen(web::http::http_request request);
	void wwwgrabScreenb64(web::http::http_request request);
	void wwwdetectState(web::http::http_request request);
	void wwwdetectEvent(web::http::http_request request);
	void wwwcountEvents(web::http::http_request request);
	void wwwcheckimage(web::http::http_request request);
	void wwwgetSound(web::http::http_request request);
	void wwwWatchdog(web::http::http_request request);
	void wwwReports(web::http::http_request request);
	void wwwReports_sse(web::http::http_request request);
	__screenState getState(int dt_ms);
	__detectScreenState detectStateChange(std::list<outputState> &stateSearch, unsigned int timeAnalysis,unsigned int timeEvent,bool countOc);
} /* namespace RestServer */

#endif /* SERVERINSTANCE_H_ */
