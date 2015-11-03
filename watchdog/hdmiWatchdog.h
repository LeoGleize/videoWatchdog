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

namespace watchdog {

/**
 * Singleton instance of our watchdog
 */
class hdmiWatchdog {

private:
	bool isRunning;
	std::mutex mutexLaunch;
	std::mutex mutexAccessSharedMessages;

	void launchWatchdog();
	hdmiWatchdog(); //constructor is PRIVATE
public:
    static hdmiWatchdog& getInstance();

    void start();
    void stop();

    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want. (otherwise there would be copies of static)
    hdmiWatchdog(hdmiWatchdog const&)    = delete;
    void operator=(hdmiWatchdog const&)  = delete;
};

} /* namespace watchdog */

#endif /* HDMIWATCHDOG_H_ */
