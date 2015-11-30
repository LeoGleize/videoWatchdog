/*
 * hdmiWatchdog.h
 *
 *  Created on: Nov 3, 2015
 *      Author: fcaldas
 */

#ifndef HDMIWATCHDOG_H_
#define HDMIWATCHDOG_H_

#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <random>
#include "../ServerInstance/ServerInstance.h"
#include <cpprest/json.h>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <cpprest/json.h>

#define __MY_CODEC CV_FOURCC('M','J','P','G')
using namespace RestServer;

namespace watchdog {

const std::string chars("abcdefghijklmnopqrstuvwxyz1234567890");

/**
 * Event to report
 */
struct eventToReport{
	unsigned int eventID;
	outputState eventType;
	std::time_t time_when;
	bool finished;
	double howLong;
	std::string videoName;
	std::string ocr_text; // text on frame
};

/**
 * Container used to store frame information for the watchdog
 */
struct watchDogData{
	cv::Mat mat;
	IplImage* pointerToFree;
	bool hasAudio;
};

/**
 * Get a JSON representation of an eventToReport object
 */
web::json::value incidentToJSON(eventToReport &evt);

/**
 * Singleton instance of our watchdog
 */
class hdmiWatchdog {

private:
	cv::VideoWriter videoWriter;
	bool isRunning;
	unsigned int eventCounter;
	std::vector<eventToReport> eventList;
	std::thread * threadWatcher;
	std::mutex mutexLaunch;
	std::mutex mutexAccessSharedMessages;
	web::json::value config;
	std::time_t startTime;
	std::list<outputState> eventsSearch;
	bool configLoaded;
	long tEventMS;
	void launchWatchdog();
	bool checkForAudio(short *audioData, unsigned int nElements);
	void getImgTextSaveToEvent(cv::Mat m, int evtIndex);
	void writeFrame(cv::Mat &mat);
	void saveLogsToFile(std::string basepath);
	hdmiWatchdog(); //constructor is PRIVATE
	std::string getRandomName(int size);
	std::string createVideoAndDumpFiles(std::deque<watchDogData> &toDump, unsigned int eventID, std::string suffix);
public:
    static hdmiWatchdog& getInstance();
    std::vector<eventToReport> getIncidents();
    bool start(std::list<outputState> eventsSearch, long tEvent);
    bool stop();
    bool isWatcherRunning();
    std::time_t getTimeStart();
    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want. (otherwise there would be copies of static)
    hdmiWatchdog(hdmiWatchdog const&)    = delete;
    void operator=(hdmiWatchdog const&)  = delete;

};

} /* namespace watchdog */

#endif /* HDMIWATCHDOG_H_ */
