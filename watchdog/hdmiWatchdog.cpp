/*
 * hdmiWatchdog.cpp
 *
 *  Created on: Nov 3, 2015
 *      Author: fcaldas
 */

#include "hdmiWatchdog.h"

namespace watchdog {

static hdmiWatchdog& hdmiWatchdog::getInstance(){
    static hdmiWatchdog    instance; // Guaranteed to be destroyed.
    return instance;
}

hdmiWatchdog::hdmiWatchdog(){
	 isRunning = false;
}

void hdmiWatchdog::start(){
	mutexLaunch.lock();
	if(!isRunning){
		isRunning = true;
		//launch watchdog
	}
	mutexLaunch.unlock();
}

void hdmiWatchdog::stop(){
	mutexLaunch.lock();
	isRunning = false;
	mutexLaunch.unlock();
}

} /* namespace watchdog */
