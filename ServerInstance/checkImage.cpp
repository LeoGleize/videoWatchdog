/*
 * checkImage.cpp
 *
 *  Created on: Oct 22, 2015
 *      Author: fcaldas
 */

#include "ServerInstance.h"
#include "../recognition/imageRecognition.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>
#include <cpprest/astreambuf.h>
#include <cpprest/containerstream.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <queue>
#include <pplx/pplxtasks.h>
#include <cpprest/json.h>
#include <system_error>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

struct imageReconData{
	cv::Mat originalImage;
	bool isValidImage;
	std::string exception;
	imageRecognition::objMatch match;
};

namespace RestServer {

/*
 * Checks if an image is present in the current screen
 */
void wwwcheckimage(web::http::http_request request){
	json::value params = request.extract_json().get();
	json::value reply;

	//check if image field is valid
	if(params.has_field("imageURL") && params["imageURL"].is_string()){
		std::string url = params["imageURL"].as_string();
		web::http::client::http_client client(url);
		http_request myrequest(methods::GET);

		//create request task (similar to JS promises)
	    pplx::task<imageReconData> requestTask = client.request(myrequest).then([](http_response response)
	    {
	    	//on error (2xx = valid reply)
	    	if(response.status_code()/100 != 2){
	    		return pplx::create_task([]
	    		{
	    			imageReconData imrData;
	    			imrData.isValidImage = false;
	    			return imrData;
	    		});
	    	}
	    	unsigned int lenght = response.headers().content_length();
	    	Concurrency::streams::istream bodyStream = response.body();
	        Concurrency::streams::container_buffer<std::string> inStringBuffer;
	        return bodyStream.read(inStringBuffer, lenght).then([inStringBuffer](size_t bytesRead)
	        {
	        	imageReconData imrData;
	        	imrData.isValidImage = true;
	            const std::string &imData = inStringBuffer.collection();
	            //try to get image from screen
	            cv::Mat screen;
	            try{
	            	screen = ServerInstance::cameraDeckLink->captureLastCvMatClone();
	            }catch(CardException &e){
	            	imrData.isValidImage = false;
	            	imrData.exception = e.what();
	            	return imrData;
	            }
				cv::Mat bwScreen;
				cv::cvtColor(screen, bwScreen, CV_BGR2GRAY);
				std::vector<unsigned char> imvector(imData.begin(), imData.end());
				cv::Mat pattern(cv::imdecode(imvector, 0)); //load grayscale
				cv::Mat templateBorder, imageBorder;
				cv::Canny(pattern,templateBorder,100,100);
				cv::Canny(bwScreen,imageBorder,100,100);
				imrData.match = imageRecognition::matchTemplateSameScale(imageBorder,templateBorder);
				imrData.originalImage = screen;
				cv::Point pt1(imrData.match.pos.x,imrData.match.pos.y);
				cv::Point pt2(imrData.match.pos.x + imrData.match.pos.width,imrData.match.pos.y + imrData.match.pos.height);
				cv::rectangle(screen,pt1, pt2, cv::Scalar(0,0,255),3);
				pt1.y -= 5;
				cv::putText(screen,"match value = "+ std::to_string(imrData.match.matchScore), pt1, cv::FONT_HERSHEY_SIMPLEX, 0.8,cv::Scalar(0,0,255));
				return imrData;

	        });

	    });
	    try{
	    	requestTask.wait();
	    	imageReconData res = requestTask.get();
		    if(res.isValidImage == true){
				//return image or JSON?
				if(params.has_field("returnImage") &&
				   params["returnImage"].is_boolean() &&
				   params["returnImage"].as_bool() == true){
					std::vector<uchar> imageData;
					cv::imencode(".png",res.originalImage,imageData);
					std::string data(imageData.begin(), imageData.end());
					request.reply(status_codes::OK, data, "Content-type: image/png");
				}else{
					reply["error"] = 0;
					reply["match"] = res.match.toJSON();
					request.reply(status_codes::OK, reply);
				}
		    }else{
		    	reply["error"] = 1;
		    	reply["message"] = web::json::value::string("imageURL returned an invalid image");
		    	reply["exception"] = web::json::value::string(res.exception);
		    	request.reply(status_codes::OK, reply);
		    }
	    }catch(const std::exception &e){
	    	reply["error"] = 1;
	    	reply["message"] = web::json::value::string(e.what());
	    	request.reply(status_codes::OK, reply);
	    }
	}else{
		reply["error"] = 1;
		std::string message = "This request needs the following parameters: 'imageURL' address " \
						      "where the image we will be searching can be found, optional fields are: " \
						      " 'returnImage':true/false - returns either an image or JSON indicating whether the pattern was found or not";
		reply["message"] = web::json::value::string(message);
		request.reply(status_codes::OK, reply);
	}
}


void wwwGetText(web::http::http_request request){
		cv::Mat img = ServerInstance::cameraDeckLink->captureLastCvMatClone();
		json::value reply;
		std::string data;
		cv::Mat inverted = img;
		cv::Mat thImage;

		try{
			data = imageRecognition::getTextFromImage(img);
		}catch(const std::exception &e){
			reply["error"] = web::json::value::boolean("true");
			reply["errorMessage"] = web::json::value::string(e.what());
		}
		reply["text"] = web::json::value::string(data);
		request.reply(status_codes::OK, reply);
}

}

