/*
 * webwatchdog.cpp
 *
 *  Created on: Nov 4, 2015
 *      Author: fcaldas
 */


#include "ServerInstance.h"
#include "../watchdog/hdmiWatchdog.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <ctime>
#include <cpprest/interopstream.h>
#include <cpprest/containerstream.h>
#include <ctime>
#include "base64/base64.h"
#include <boost/lexical_cast.hpp>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer{

	void wwwWatchdog(web::http::http_request request){
		json::value params = request.extract_json().get();
		json::value reply;
		if(  params.has_field("start")
		  && params["start"].is_boolean()
		  && params["start"].as_bool() == true){
			watchdog::hdmiWatchdog &watcher = watchdog::hdmiWatchdog::getInstance();
			//get events we are looking for
			bool isStringANumber = false;
			int tEvent;
			if(params.has_field("tevent") && params["tevent"].is_string()){
				try{
					tEvent = boost::lexical_cast<int>(params["tevent"].as_string());
					isStringANumber = true;
				}catch(const std::exception &e){

				}
			}

			if(  params.has_field("tevent") &&
			   ((params["tevent"].is_integer() &&
			     params["tevent"].as_integer() > 0) || isStringANumber) ){

				if(!params.has_field("eventType") || !params["eventType"].is_array()){
					reply["error"] = 1;
					std::string message = "Invalid value for eventType, should be array of: LIVE / FREEZE / BLACK / FREEZE_NO_AUDIO / BLACK_NO_AUDIO";
					reply["message"] = web::json::value::string(message);
					request.reply(status_codes::OK, reply);
					return;
				}
				std::list<outputState> eventsSearch;
				//go through list of events being searched
				for(int i = 0; i < params["eventType"].as_array().size(); i++){
					outputState oState = getStateByName(params["eventType"].as_array().at(i).as_string());
					eventsSearch.push_back(oState);
					if(oState == S_NOT_FOUND){
						reply["error"] = 1;
						std::string message = "Invalid value for eventType, should be: LIVE / FREEZE / BLACK / FREEZE_NO_AUDIO / BLACK_NO_AUDIO";
						reply["message"] = web::json::value::string(message);
						request.reply(status_codes::OK, reply);
						return;
					}
				}
				tEvent = (isStringANumber)?tEvent:params["tevent"].as_integer();
				if(watcher.start(eventsSearch, tEvent)){
					reply["error"] = 0;
					reply["message"] = web::json::value::string("Watchdog started");
				}else{
					reply["error"] = 1;
					reply["message"] = web::json::value::string("Could not start watchdog: config.json not found");
				}

			}else{
				reply["error"] = 1;
				reply["message"] = web::json::value::string("'tevent' : time of the event we're looking for has to be provided");
			}
		}else if(  params.has_field("stop")
			    && params["stop"].is_boolean()
			    && params["stop"].as_bool() == true){
			watchdog::hdmiWatchdog &watcher = watchdog::hdmiWatchdog::getInstance();
			std::vector<watchdog::eventToReport> eventsToReport;
			if(watcher.isWatcherRunning())
				eventsToReport = watcher.getIncidents();
			if(watcher.stop()){
				reply["error"] = 0;
				reply["message"] = web::json::value::string("Watchdog stopped");

				json::value incidents;

				for(unsigned int i = 0; i < eventsToReport.size(); i++){
					json::value objTest;
					objTest = watchdog::incidentToJSON(eventsToReport[i]);
					incidents[i] = objTest;
				}
				reply["incidents"] = incidents;
				char buffer[32];
				struct tm * timeinfo;
				std::time_t tstart = watcher.getTimeStart();
				timeinfo = localtime(&(tstart));
				std::strftime(buffer, 32, "%d.%m.%Y %H:%M:%S", timeinfo);
				reply["startTime"] = web::json::value::string(buffer);
			}else{
				reply["error"] = 1;
				reply["message"] = web::json::value::string("Could not stop watchdog: not running");
			}
		}else{
			reply["error"] = 1;
			reply["message"] = web::json::value::string("This method expects either 'start' = true or 'stop' = true");
			watchdog::hdmiWatchdog &watcher = watchdog::hdmiWatchdog::getInstance();
			reply["isRunning"] = web::json::value::boolean(watcher.isWatcherRunning());
		}
		request.reply(status_codes::OK, reply);
	}

	void wwwReports(web::http::http_request request){
		json::value myReply;

		watchdog::hdmiWatchdog &watcher = watchdog::hdmiWatchdog::getInstance();

		if(watcher.isWatcherRunning()){
			myReply["message"] = web::json::value::string("Watchdog is running");
		}else{
			myReply["message"] = web::json::value::string("Watchdog is not running");
		}

		json::value incidents;
		if(watcher.isWatcherRunning()){
			std::vector<watchdog::eventToReport> events = watcher.getIncidents();
			for(unsigned int i = 0; i < events.size(); i++){
				json::value objTest;
				objTest = watchdog::incidentToJSON(events[i]);
				incidents[i] = objTest;
			}
			char buffer[32];
			struct tm * timeinfo;
			std::time_t tstart = watcher.getTimeStart();
			timeinfo = localtime(&(tstart));
			std::strftime(buffer, 32, "%d.%m.%Y %H:%M:%S", timeinfo);
			myReply["startTime"] = web::json::value::string(buffer);
			myReply["incidents"] = incidents;
		}

		request.reply(status_codes::OK,myReply);

	}
}

