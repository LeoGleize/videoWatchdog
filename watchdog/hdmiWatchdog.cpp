/*
 * hdmiWatchdog.cpp
 *
 *  Created on: Nov 3, 2015
 *      Author: fcaldas
 *		        fcaldas@canal-plus.fr
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
	 configFile.open("config.json",std::ios::in);
	 std::string data = "";
	 configLoaded = true;
	 if(configFile.is_open() != true){
		 configLoaded = false;
		 std::cout<<"Could not open config.json"<<std::endl;
		 std::cout<<"This file is required to run HDMIWatchdog"<<std::endl;
		 std::cout<<"Copy it to the current directory."<<std::endl;
		 return;
	 }
	 while(configFile.eof() != true){
		 std::string line;
		 std::getline(configFile, line);
		 data += line;
	 }
	 this->config = web::json::value::parse(data);
}

/*
 * Write one frame to video
 */
void hdmiWatchdog::writeFrame(cv::Mat &mat){
	web::json::value watchConfig = config["watchdog"];
	if(watchConfig["saveVideos"].as_bool() == true){
		videoWriter.write(mat);
	}
}

/*
 * Transforms an eventToReport object into an json::value.
 */
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


std::time_t hdmiWatchdog::getTimeStart(){
	return this->startTime;
}

/*
 * Generates an random name with length = size
 * used for video name generation
 */
std::string hdmiWatchdog::getRandomName(int size){
	boost::random::random_device rng;
	boost::random::uniform_int_distribution<> index_dist(0, watchdog::chars.size() - 1);
	std::string randomName;
	for(int i = 0; i < size; ++i) {
	        randomName += watchdog::chars[index_dist(rng)];
	}
	return randomName;
}

/*
 * Get list of incidents, this has to be accessed with mutexes
 * since the method might be called by our REST server
 */
std::vector<eventToReport> hdmiWatchdog::getIncidents(){
	this->mutexAccessSharedMessages.lock();
	std::vector<eventToReport> eventsNow = this->eventList;
	this->mutexAccessSharedMessages.unlock();
	return eventsNow;
}


/*
 * Starts watchdog thread
 */
bool hdmiWatchdog::start(std::list<outputState> eventsSearch, long tEvent){
	if(configLoaded == false)
		return false;
	mutexLaunch.lock();
	if(!isRunning){
		this->eventsSearch = eventsSearch;
		this->tEventMS = tEvent;
		isRunning = true;
		this->eventList.clear();
		threadWatcher = new std::thread(&hdmiWatchdog::launchWatchdog, this);
		mutexLaunch.unlock();
		eventCounter = 0;
		this->startTime = std::time(NULL);
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
		this->startTime = std::time(NULL);
		return true;
	}
}

/*
 * Returns a boolean indicating if the watchdog is still running
 */
bool hdmiWatchdog::isWatcherRunning(){
	mutexLaunch.lock();
	bool v = isRunning;
	mutexLaunch.unlock();
	return v;
}

/*
 * Stop watchdog thread
 */
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


/*
 * Create a video file and write all frames in &toDump to
 * the file, to be used when an incident is reported
 */
std::string hdmiWatchdog::createVideoAndDumpFiles(std::deque<watchDogData> &toDump, unsigned int eventID, std::string suffix){
	web::json::value watchConfig = config["watchdog"];
	if(watchConfig["saveVideos"].as_bool() == false)
		return "";
	std::string dirToSave = watchConfig["saveVideosTo"].as_string();

	std::string fname = "logvideo_" + std::to_string(eventID) + "_" + suffix + "_" + getRandomName(3) + ".avi";
	cv::Size s(toDump.front().mat.cols, toDump.front().mat.rows);

	videoWriter.open(dirToSave + fname, __MY_CODEC, 5, s);

	if(!videoWriter.isOpened()){
		std::cout<<"Error opening video file: "<<fname<<std::endl;
		return "Could not open " + dirToSave + fname;
	}
	for (std::deque<watchDogData>::iterator it = toDump.begin(); it!=toDump.end(); ++it)
	    videoWriter.write(it->mat);
	return watchConfig["prefixToLog"].as_string() + fname;

}

/*
 * Go through *audioData and checks if all values are smaller in module
 * than a threshold, this is used to verify if audio data is present in a video
 * frame
 */
bool hdmiWatchdog::checkForAudio(short *audioData, unsigned int nElements){
	short max = 0;
	for(unsigned int i = 0; i < nElements; i++)
		if(audioData[i] > max)
			max = audioData[i];
	if(max > soundThreshold){
		return true;
	}
	return false;
}


/*
 * Watchdog main loop
 */
void hdmiWatchdog::launchWatchdog(){
	std::cout<<"[+] Starting watchdog"<<std::endl;

	std::deque<watchDogData> imageList;

	bool searchBlack  = (std::find(eventsSearch.begin(), eventsSearch.end(), S_BLACK_SCREEN)  != eventsSearch.end());
	bool searchFreeze = (std::find(eventsSearch.begin(), eventsSearch.end(), S_FREEZE_SIGNAL) != eventsSearch.end());
	bool searchLive   = (std::find(eventsSearch.begin(), eventsSearch.end(), S_LIVE_SIGNAL)   != eventsSearch.end());
	bool searchFreezeNoAudio = (std::find(eventsSearch.begin(), eventsSearch.end(), S_FREEZE_SIGNAL_NO_AUDIO) != eventsSearch.end());
	bool searchBlackNoAudio  = (std::find(eventsSearch.begin(), eventsSearch.end(), S_BLACK_SCREEN_NO_AUDIO)  != eventsSearch.end());

	outputState lastCapturedState = S_NOT_FOUND;
	outputState newCapturedState = S_NOT_FOUND;

	unsigned int dt_interFramems =  200;
	unsigned int nFrames = tEventMS / dt_interFramems;
	boost::random::random_device randomGen;
	boost::random::uniform_int_distribution<> randomIntDist( 1 , nFrames-1);

	timeval t0, t1;
	long culmulativeDelay = 0;

	while(isRunning){
		gettimeofday(&t0, NULL);
		try{
			watchDogData data;
			IplImage *pToFree;
			void *ptrAudioData;
			int nBytes;
			data.mat = ServerInstance::cameraDeckLink->captureLastCvMatAndAudio(&pToFree, &ptrAudioData, &nBytes);
			data.pointerToFree = pToFree;
			int nElements = nBytes / sizeof(short);
			data.hasAudio = this->checkForAudio((short *) ptrAudioData, nElements);
			free(ptrAudioData);
			imageList.push_back(data);
		}catch(const CardException &e){
			/**
			 * In case of an exception on video acquisition
			 * we assume that the cable went off.
			 *
			 * The only other reason why this could happen would
			 * be a change in screen resolution in the STB
			 */
			if(lastCapturedState != S_NOT_FOUND && newCapturedState != S_NO_VIDEO){
				this->mutexAccessSharedMessages.lock();
				if(this->eventList.size() > 0){
					this->eventList[eventList.size() - 1].finished = true;
				}
				this->mutexAccessSharedMessages.unlock();
			}

			if(lastCapturedState == S_NO_VIDEO){
				eventList[eventList.size() - 1].howLong += dt_interFramems;
			}else{
				eventToReport newEvent;
				newEvent.finished = false;
				newEvent.howLong = 100;
				newEvent.time_when = std::time(NULL);
				newEvent.eventType = S_NO_VIDEO;
				newEvent.videoName = "";
				newEvent.eventID = eventCounter++;
				eventList.push_back(newEvent);
			}
			newCapturedState = S_NO_VIDEO;
			lastCapturedState = S_NO_VIDEO;
			usleep(	1000 * dt_interFramems);
			std::cout<<"Caught exception on detectState():"<<e.what()<<std::endl;
			continue;
		}

		//Limit the size of the deque
		if (imageList.size() > nFrames) {
			IplImage *p = imageList.front().pointerToFree;
			cvRelease((void **) &p);
			imageList.pop_front();
		}
		//deque is full therefore we can process
		if (imageList.size() == nFrames) {
			double maxDiff = 0;
			unsigned int npixel = imageList[0].mat.cols * imageList[0].mat.rows;
			cv::Mat subtractionResult;

			//measure max diff: between first and last frame and random samples in between
			int otherIndex;
			for (unsigned int i = 1; i < 10; i++) {

				if(i == 9)
					otherIndex = imageList.size() - 1;
				else
					otherIndex = randomIntDist(randomGen);

				double n1,n2;
				cv::subtract(imageList[0].mat, imageList[otherIndex].mat, subtractionResult);
				n1 = cv::norm(subtractionResult);
				cv::subtract(imageList[otherIndex].mat, imageList[0].mat, subtractionResult);
				n2 = cv::norm(subtractionResult);
				maxDiff = (n1 > maxDiff) ? n1 : maxDiff;
				maxDiff = (n2 > maxDiff) ? n2 : maxDiff;
			}

			//check if any of the frames has audio in them (except first)
			bool hasAudio = false;
			for(unsigned int i = 1; i < imageList.size(); i++){
				hasAudio = (imageList[i].hasAudio == true || hasAudio) ? true: false;
			}
			maxDiff = maxDiff / npixel;
			/*
			 * Detect live signal.
			 */
			if (searchLive && maxDiff > freezeThreshold) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_LIVE_SIGNAL){
					//just increment last capture
					eventList[eventList.size() - 1].howLong += dt_interFramems;
					writeFrame(imageList.back().mat);
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = S_LIVE_SIGNAL;
					newEvent.videoName = createVideoAndDumpFiles(imageList,eventCounter, "live");
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_LIVE_SIGNAL;
			/*
			 * Check if last group of frames are a Black screen.
			 */
			} else if ((searchBlack
					 && maxDiff < freezeThreshold
			         && imageRecognition::isImageBlackScreenOrZapScreen(imageList[0].mat,blackThreshold))
					 || (searchBlackNoAudio
					 && hasAudio == false
					 && maxDiff < freezeThreshold
					 && imageRecognition::isImageBlackScreenOrZapScreen(imageList[0].mat,blackThreshold))) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_BLACK_SCREEN){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
					writeFrame(imageList.back().mat);
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = (hasAudio)?S_BLACK_SCREEN:S_BLACK_SCREEN_NO_AUDIO;
					if(hasAudio)
						newEvent.videoName = createVideoAndDumpFiles(imageList, eventCounter, "black");
					else
						newEvent.videoName = createVideoAndDumpFiles(imageList, eventCounter, "blackmute");
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_BLACK_SCREEN;
			/*
			 * Check if last group of frames is a FREEZE
			 */
			} else if ((searchFreeze
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(imageList[0].mat,blackThreshold))
					|| (searchFreezeNoAudio
					&& hasAudio == false
					&& maxDiff < freezeThreshold
					&& !imageRecognition::isImageBlackScreenOrZapScreen(imageList[0].mat,blackThreshold) )) {
				this->mutexAccessSharedMessages.lock();
				if(lastCapturedState == S_FREEZE_SIGNAL || lastCapturedState == S_FREEZE_SIGNAL_NO_AUDIO){
					eventList[eventList.size() - 1 ].howLong += dt_interFramems;
					writeFrame(imageList.back().mat);
				}else{
					eventToReport newEvent;
					newEvent.finished = false;
					newEvent.howLong = tEventMS;
					newEvent.time_when = std::time(NULL);
					newEvent.eventType = (hasAudio)?S_FREEZE_SIGNAL:S_FREEZE_SIGNAL_NO_AUDIO;
					if(hasAudio)
						newEvent.videoName = createVideoAndDumpFiles(imageList,eventCounter, "freeze");
					else
						newEvent.videoName = createVideoAndDumpFiles(imageList,eventCounter, "freezemute");
					newEvent.eventID = eventCounter++;
					eventList.push_back(newEvent);
				}
				this->mutexAccessSharedMessages.unlock();
				newCapturedState = S_FREEZE_SIGNAL;
			}else{
				newCapturedState = S_NOT_FOUND;
			}

			/*
			 * Set finished flag on an event once event has changed
			 */
			if(lastCapturedState != S_NOT_FOUND && lastCapturedState != newCapturedState){
				this->mutexAccessSharedMessages.lock();
				if(this->eventList.size() > 0){
					//if we added a new one we will need to go back an extra index
					if(newCapturedState != S_NOT_FOUND)
						this->eventList[eventList.size() - 2].finished = true;
					else
						this->eventList[eventList.size() - 1].finished = true;
				}
				this->mutexAccessSharedMessages.unlock();
			}
			lastCapturedState = newCapturedState;
		}
		gettimeofday(&t1, NULL);

		/*
		 * Calculate how long the loop took to be executed an sleep the amount of time
		 * between each call minus the amount of time the loop took to be executed.
		 *
		 * THIS IS NOT EXACT but is close enough for our application
		 * A better implementation of this can be found on boost::Asio
		 */
		if (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 + culmulativeDelay < 1000 * dt_interFramems){
			usleep(	1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000 - culmulativeDelay));
			culmulativeDelay = 0;
		}else{
			culmulativeDelay = culmulativeDelay - (1000 * dt_interFramems - (t1.tv_usec - t0.tv_usec + (t1.tv_sec - t0.tv_sec) * 1000000));
		}
	}

	//free any memory that is still allocated
	for(std::deque<watchDogData>::iterator i = imageList.begin(); i != imageList.end(); i++){
		IplImage *v = i->pointerToFree;
		cvRelease((void **) &v);
	}
	imageList.clear();
	if(videoWriter.isOpened())
		videoWriter.release();

	std::cout<<"[-] Stopping watchdog"<<std::endl;
}


} /* namespace watchdog */
