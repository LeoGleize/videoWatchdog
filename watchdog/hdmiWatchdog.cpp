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

namespace watchdog {



hdmiWatchdog& hdmiWatchdog::getInstance(){
    static hdmiWatchdog    instance;
    return instance;
}

hdmiWatchdog::hdmiWatchdog(){
	 isRunning = false;
	 threadWatcher = NULL;
}

std::vector<eventToReport> hdmiWatchdog::getIncidents(){
	this->mutexAccessSharedMessages.lock();
	std::vector<eventToReport> eventsNow = this->eventList;
	if(eventList.size() > 0 &&
	   eventList[eventList.size() - 1].finished == true){
		eventList.clear();
	}else if(eventList.size() > 0){
		eventList.erase(eventList.begin(), eventList.end() - 1);
	}
	this->mutexAccessSharedMessages.unlock();
	return eventsNow;
}

bool hdmiWatchdog::start(std::list<outputState> eventsSearch, long tEvent){
	mutexLaunch.lock();
	if(!isRunning){
		this->eventsSearch = eventsSearch;
		this->tEventMS = tEvent;
		isRunning = true;
		threadWatcher = new std::thread(&hdmiWatchdog::launchWatchdog, this);
		mutexLaunch.unlock();
		this->eventList.clear();
		return true;
	}else{
		//if is running restart
		mutexLaunch.unlock();
		this->stop();
		mutexLaunch.lock();
		this->eventsSearch = eventsSearch;
		this->tEventMS = tEvent;
		isRunning = true;
		threadWatcher = new std::thread(&hdmiWatchdog::launchWatchdog, this);
		mutexLaunch.unlock();
		this->eventList.clear();
		return true;
	}
}

bool hdmiWatchdog::isWatcherRunning(){
	bool v;
	mutexLaunch.lock();
	v = isRunning;
	mutexLaunch.unlock();
	return isRunning;
}

bool hdmiWatchdog::stop(){
	this->mutexLaunch.lock();
	if(isRunning == true){
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


void hdmiWatchdog::launchWatchdog(){
	std::cout<<"[+] Starting watchdog"<<std::endl;

	bool searchBlack  = (std::find(eventsSearch.begin(), eventsSearch.end(), S_BLACK_SCREEN)  != eventsSearch.end());
	bool searchFreeze = (std::find(eventsSearch.begin(), eventsSearch.end(), S_FREEZE_SIGNAL) != eventsSearch.end());
	bool searchLive   = (std::find(eventsSearch.begin(), eventsSearch.end(), S_LIVE_SIGNAL)   != eventsSearch.end());
	outputState lastCapturedState = S_NOT_FOUND;
	int nFrames = tEventMS / 100;
		if(nFrames > 15)
			nFrames = 15; //saturate otherwise we'll have too many comparisons
	int dt_interFramems =  tEventMS / nFrames;
	std::deque<cv::Mat> matList;
	timeval t0, t1;
	long culmulativeDelay = 0;

	while(isRunning){
		gettimeofday(&t0, NULL);
		try{
			matList.push_back(ServerInstance::cameraDeckLink->captureLastCvMat());
		}catch(const CardException &e){
			usleep(	1000 * dt_interFramems);
			std::cout<<"Caught exception on detectState():"<<e.what()<<std::endl<<std::flush;
			continue;
		}
		if (matList.size() > nFrames) {
			matList.pop_front();
		}
		//deque is full therefore we can process
		if (matList.size() == nFrames) {
			double maxDiff = 0;
			unsigned int npixel = matList[0].cols * matList[0].rows;
			cv::Mat subtractionResult;
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
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_LIVE_SIGNAL;
					newEvent.images.push_back(matList.front());
					newEvent.images.push_back(matList.back());
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				lastCapturedState = S_LIVE_SIGNAL;
			} else if (searchBlack
					&& maxDiff < freezeThreshold
			        && imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_BLACK_SCREEN){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_BLACK_SCREEN;
					newEvent.images.push_back(matList.front());
					newEvent.images.push_back(matList.back());
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				lastCapturedState = S_BLACK_SCREEN;
			} else if (searchFreeze
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(matList[0],blackThreshold)) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_FREEZE_SIGNAL){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_FREEZE_SIGNAL;
					newEvent.images.push_back(matList.front());
					newEvent.images.push_back(matList.back());
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				lastCapturedState = S_FREEZE_SIGNAL;
			}else{
				if(lastCapturedState != S_NOT_FOUND){
					this->mutexAccessSharedMessages.lock();
					if(this->eventList.size() > 0){
						this->eventList[eventList.size() - 1 ].finished = true;
					}
					this->mutexAccessSharedMessages.unlock();
					lastCapturedState = S_NOT_FOUND;

				}
			}
		}
		gettimeofday(&t1, NULL);

		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 + culmulativeDelay < 1000 * dt_interFramems){
			usleep(	1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 - culmulativeDelay));
			culmulativeDelay = 0;
		}else{
			culmulativeDelay = culmulativeDelay - (1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
		}
	}
	std::cout<<"[-] Stopping watchdog"<<std::endl;
}


} /* namespace watchdog */
