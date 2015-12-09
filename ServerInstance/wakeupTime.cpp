/*
 * Implements methods related to measurement of wakeup time of a STB
 * using  an ePowerSwitch4 to turn the box off and on.
 *
 * Wakeup is achieved once we have audio output and live video
 *
 */

#include "ServerInstance.h"
#include "tcpClient/tcpClient.h"
#include <unistd.h>
#include <cpprest/json.h>
#include <iostream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace RestServer{

	/**
	 * Create an electrical reset and calculates time we had to wait until we observed a live stream
	 * Uses an ePowerSwitch to reset the STB
	 */
	void wwwGetWakeupTime(web::http::http_request request){
		json::value params = request.extract_json().get();
		json::value reply;
		if(params.has_field("nplug") && params.has_field("ip") &&
		   params["nplug"].is_string() && params["ip"].is_string()){
			std::string ipAddr = params["ip"].as_string();
			std::string portNumber = params["nplug"].as_string();

			websocket::tcpClient tClient;
			if(tClient.conn(ipAddr, 80)){
				//turn off!
				try{
					tClient.send_data("POST /hidden.htm HTTP/1.1\r\n");
					tClient.send_data("Connection: keep-alive\r\n");
					tClient.send_data("Content-Type: text/plain;charset=UTF-8\r\n");
					tClient.send_data("Content-Length: 9\r\n");
					tClient.send_data("Accept: */*\r\n");
					tClient.send_data("Content-Type: application/json\r\n\r\n");
					tClient.send_data("M0:O" + portNumber + "=Off");
					usleep(1000 * 1000 * 2);
					tClient.close();
					tClient.conn(ipAddr,80);
					tClient.send_data("POST /hidden.htm HTTP/1.1\r\n");
					tClient.send_data("Connection: keep-alive\r\n");
					tClient.send_data("Content-Type: text/plain;charset=UTF-8\r\n");
					tClient.send_data("Content-Length: 8\r\n");
					tClient.send_data("Accept: */*\r\n");
					tClient.send_data("Content-Type: application/json\r\n\r\n");
					tClient.send_data("M0:O" + portNumber + "=On");
					//start counting!
					sleep(10); //wait for a bit before counting
					long t = detectWakeUP(1000*60*10); //max wait is 6 minutes!
					if(t < 0){
						reply["error"] = 1;
						reply["message"] = web::json::value::string("Boot was not detected after 10 minutes");
					}else{
						reply["done"] = 1;
						reply["ms"] = web::json::value::number(t + 10000);
					}
				}catch(const std::exception &e){
					reply["error"] = 1;
					reply["message"] = web::json::value::string(e.what());
				}
			}else{
				reply["error"] = 1;
				reply["message"] = web::json::value::string("Cannot connect to ePowerSwitch at " + ipAddr + ":80");
			}
		}else{
			reply["error"] = 1;
			reply["message"] = web::json::value::string("This route requires two parameters: 'ip' the ip" \
							  " address of the ePowerSwitch and 'nplug' the plug to which the STB is connected");

		}
		request.reply(status_codes::OK, reply);
	}

	/**
	 * Calculates time we had to wait until we observed a live stream
	 */
	void wwwGetTimeToLive(web::http::http_request request){

		json::value params = request.extract_json().get();
		json::value reply;	//start counting!
		if(params.has_field("tempo") && params["tempo"].is_string()){
			try{
				int delta = std::stoi(params["tempo"].as_string());
				long t = detectWakeUP(1000*60*10); //max wait is 6 minutes!
				if(t < 0){
					reply["error"] = 1;
					reply["message"] = web::json::value::string("Boot was not detected after 10 minutes");
				}else{
					reply["done"] = 1;
					reply["ms"] = web::json::value::number(t + delta);
				}
			}catch(const std::exception &e){
				reply["error"] = 1;
				reply["message"] = web::json::value::string(e.what());
			}
		}else{
			reply["error"] = 1;
			std::string msg = "This route requires a variable 'tempo' with a delta that will be added to the mesure";
			reply["message"] = web::json::value::string(msg);
		}
		request.reply(status_codes::OK, reply);
	}

}
