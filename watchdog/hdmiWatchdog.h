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
#include "../ServerInstance/ServerInstance.h"

using namespace RestServer;

namespace watchdog {


/**
 * Event to report
 */
struct eventToReport{
	outputState eventType;
	std::time_t time_when;
	bool finished;
	std::vector<cv::Mat> images;
	unsigned long howLong;
};

/**
 * Singleton instance of our watchdog
 */
class hdmiWatchdog {

private:
	bool isRunning;
	std::vector<eventToReport> eventList;
	std::thread * threadWatcher;
	std::mutex mutexLaunch;
	std::mutex mutexAccessSharedMessages;
	std::list<outputState> eventsSearch;
	long tEventMS;
	void launchWatchdog();
	hdmiWatchdog(); //constructor is PRIVATE
public:
    static hdmiWatchdog& getInstance();
    std::vector<eventToReport> getIncidents();
    bool start(std::list<outputState> eventsSearch, long tEvent);
    bool stop();
    bool isWatcherRunning();
    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want. (otherwise there would be copies of static)
    hdmiWatchdog(hdmiWatchdog const&)    = delete;
    void operator=(hdmiWatchdog const&)  = delete;

};

} /* namespace watchdog */

#endif /* HDMIWATCHDOG_H_ */
