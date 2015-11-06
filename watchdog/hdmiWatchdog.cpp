/*
 * hdmiWatchdog.cpp
 *
 *  Created on: Nov 3, 2015
 *      Author: fcaldas
 */

#include "hdmiWatchdog.h"
#include <unistd.h>
#include <list>
#include <sys/time.h>
#include "../recognition/imageRecognition.h"
#include <opencv2/opencv.hpp>
#include "../ServerInstance/ServerInstance.h"
#include "../ServerInstance/base64/base64.h"
#include <cpprest/json.h>
#include <fstream>

namespace watchdog {



hdmiWatchdog& hdmiWatchdog::getInstance(){
    static hdmiWatchdog    instance;
    return instance;
}

hdmiWatchdog::hdmiWatchdog(){
	 isRunning = false;
	 threadWatcher = NULL;
	 std::ifstream configFile;
	 configFile.open("config.json");
	 std::string data = "";
	 while(configFile.eof() != true){
		 std::string line;
		 std::getline(configFile, line);
		 data += line;
	 }
	 this->config = web::json::value::parse(data);
}

web::json::value incidentToJSON(eventToReport &evt){
	web::json::value objTest;
	objTest["event"] = web::json::value::string(getNameOfState(evt.eventType));
	if(evt.finished){
		objTest["isIncidentFinished"] = web::json::value::boolean(true);
	}else{
		objTest["isIncidentFinished"] = web::json::value::boolean(false);
	}
	char buffer[32];
	struct tm * timeinfo;
	timeinfo = localtime(&(evt.time_when));
	std::strftime(buffer, 32, "%d.%m.%Y %H:%M:%S", timeinfo);
	objTest["when"] = web::json::value::string(buffer);
	objTest["duration_ms"] = web::json::value::number(evt.howLong);
	objTest["videoName"] = web::json::value::string(evt.videoName);
	return objTest;
}

std::string hdmiWatchdog::getRandomName(int size){
	boost::random::random_device rng;
	boost::random::uniform_int_distribution<> index_dist(0, watchdog::chars.size() - 1);
	std::string randomName;
	for(int i = 0; i < size; ++i) {
	        randomName += watchdog::chars[index_dist(rng)];
	}
	return randomName;
}

std::vector<eventToReport> hdmiWatchdog::getIncidents(){
	this->mutexAccessSharedMessages.lock();
	std::vector<eventToReport> eventsNow = this->eventList;
	this->mutexAccessSharedMessages.unlock();
	return eventsNow;
}

bool hdmiWatchdog::start(std::list<outputState> eventsSearch, long tEvent){
	mutexLaunch.lock();
	if(!isRunning){
		this->eventsSearch = eventsSearch;
		this->tEventMS = tEvent;
		isRunning = true;
		this->eventList.clear();
		threadWatcher = new std::thread(&hdmiWatchdog::launchWatchdog, this);
		mutexLaunch.unlock();
		eventCounter = 0;
		return true;
	}else{
		//if is running restart
		mutexLaunch.unlock();  //avoid deadlock
		this->stop();		   //stop will use the same mutex
		mutexLaunch.lock();
		this->eventsSearch = eventsSearch;
		this->tEventMS = tEvent;
		isRunning = true;
		this->eventList.clear();
		threadWatcher = new std::thread(&hdmiWatchdog::launchWatchdog, this);
		mutexLaunch.unlock();
		eventCounter = 0;
		return true;
	}
}

bool hdmiWatchdog::isWatcherRunning(){
	mutexLaunch.lock();
	bool v = isRunning;
	mutexLaunch.unlock();
	return v;
}

bool hdmiWatchdog::stop(){
	this->mutexLaunch.lock();
	if(isRunning == true){
		eventCounter = 0;
		isRunning = false;
		threadWatcher->join();
		delete threadWatcher;
		threadWatcher = NULL;
		this->mutexLaunch.unlock();
		return true;
	}
	this->mutexLaunch.unlock();
	return false;
}

std::string hdmiWatchdog::createVideoAndDumpFrames(std::deque<cv::Mat> toDump, unsigned int eventID){
	web::json::value watchConfig = config["watchdog"];
	std::string dirToSave = watchConfig["saveVideosTo"].as_string();

	std::string fname = "logVideo_" + getRandomName(5) + "_" + std::to_string(eventID) + ".avi";
	cv::Size s(toDump.front().cols, toDump.front().rows);

	videoWriter.open(dirToSave + fname, __MY_CODEC, 5, s);

	if(!videoWriter.isOpened()){
		std::cout<<"Error opening video file: "<<fname<<std::endl;
		return "Could not open " + dirToSave + fname;
	}
	for (std::deque<cv::Mat>::iterator it = toDump.begin(); it!=toDump.end(); ++it)
	    videoWriter.write(*it);
	return watchConfig["prefixToLog"].as_string() + fname;

}


void hdmiWatchdog::launchWatchdog(){
	std::cout<<"[+] Starting watchdog"<<std::endl;

	std::deque<cv::Mat> matList;
	std::deque<IplImage *> pointersToFree;

	bool searchBlack  = (std::find(eventsSearch.begin(), eventsSearch.end(), S_BLACK_SCREEN)  != eventsSearch.end());
	bool searchFreeze = (std::find(eventsSearch.begin(), eventsSearch.end(), S_FREEZE_SIGNAL) != eventsSearch.end());
	bool searchLive   = (std::find(eventsSearch.begin(), eventsSearch.end(), S_LIVE_SIGNAL)   != eventsSearch.end());

	outputState lastCapturedState = S_NOT_FOUND;
	outputState newCapturedState = S_NOT_FOUND;

	//TODO: find more efficient way to to this
	unsigned int dt_interFramems =  200;
	unsigned int nFrames = tEventMS / dt_interFramems;
//		if(nFrames > 15)
//			nFrames = 15; //saturate otherwise we'll have too many comparisons


	timeval t0, t1;
	long culmulativeDelay = 0;

	while(isRunning){
		gettimeofday(&t0, NULL);
		try{
			IplImage *pToFree;
//			matList.push_back(ServerInstance::cameraDeckLink->captureLastCvMatClone());
			matList.push_back(ServerInstance::cameraDeckLink->captureLastCvMat(&pToFree));
			pointersToFree.push_back(pToFree);
		}catch(const CardException &e){
			usleep(	1000 * dt_interFramems);
			std::cout<<"Caught exception on detectState():"<<e.what()<<std::endl<<std::flush;
			continue;
		}
		if (matList.size() > nFrames) {
			IplImage *p = pointersToFree.front();
			cvRelease((void **) &p);
			matList.pop_front();
			pointersToFree.pop_front();
		}
		//deque is full therefore we can process
		if (matList.size() == nFrames) {
			double maxDiff = 0;
			unsigned int npixel = matList[0].cols * matList[0].rows;
			cv::Mat subtractionResult;
			//assume that max diff is between first and last?

			for (unsigned int i = 1; i < matList.size(); i++) {
				double n1,n2;
				cv::subtract(matList[0], matList[i], subtractionResult);
				n1 = cv::norm(subtractionResult);
				cv::subtract(matList[i], matList[0], subtractionResult);
				n2 = cv::norm(subtractionResult);
				maxDiff = (n1 > maxDiff) ? n1 : maxDiff;
				maxDiff = (n2 > maxDiff) ? n2 : maxDiff;
			}
			maxDiff = maxDiff / npixel;
			if (searchLive && maxDiff > freezeThreshold) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_LIVE_SIGNAL){
					//just increment last capture
					eventList[eventList.size() - 1].howLong += dt_interFramems;
					videoWriter.write(matList.back());
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_LIVE_SIGNAL;
					newEvent.videoName = createVideoAndDumpFrames(matList,eventCounter);
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_LIVE_SIGNAL;
			} else if (searchBlack
					&& maxDiff < freezeThreshold
			        && imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_BLACK_SCREEN){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
					videoWriter.write(matList.back());
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_BLACK_SCREEN;
					newEvent.videoName = createVideoAndDumpFrames(matList,eventCounter);
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_BLACK_SCREEN;
			} else if (searchFreeze
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_FREEZE_SIGNAL){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
					videoWriter.write(matList.back());
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_FREEZE_SIGNAL;
					newEvent.videoName = createVideoAndDumpFrames(matList,eventCounter);
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_FREEZE_SIGNAL;
			}else{
				newCapturedState = S_NOT_FOUND;
			}

			if(lastCapturedState != S_NOT_FOUND && lastCapturedState != newCapturedState){
				this->mutexAccessSharedMessages.lock();
				if(this->eventList.size() > 0){
					this->eventList[eventList.size() - 1 ].finished = true;
					this->eventList[eventList.size() - 1 ].eventID = eventCounter++;
				}
				this->mutexAccessSharedMessages.unlock();
			}
			lastCapturedState = newCapturedState;
		}
		gettimeofday(&t1, NULL);

//		std::cout<<"Tloop = "<<((t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000))<<std::endl;
		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 + culmulativeDelay < 1000 * dt_interFramems){
			usleep(	1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 - culmulativeDelay));
			culmulativeDelay = 0;
		}else{
			culmulativeDelay = culmulativeDelay - (1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
		}
	}

	//free any memory that is still allocated
	matList.clear();
	for(std::deque<IplImage *>::iterator i = pointersToFree.begin(); i != pointersToFree.end(); i++){
		IplImage *v = *i;
		cvRelease((void **) &v);
	}
	if(videoWriter.isOpened())
		videoWriter.release();

	std::cout<<"[-] Stopping watchdog"<<std::endl;
}


} /* namespace watchdog */
