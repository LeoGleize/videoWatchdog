//============================================================================
// Name        : IntensityAquisition.cpp
// Author      : Filipe Caldas
// Version     :
// Copyright   : 
// Description : Hello World in C, Ansi-style
//============================================================================

#include <iostream>
#include "blackmagic/cameradecklink.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <omp.h>
#include "ServerInstance/ServerInstance.h"

using namespace std;


int main(void) {
	RestServer::ServerInstance myServer;


    CameraDecklink    *camera1;

    camera1 = new CameraDecklink();
    RestServer::ServerInstance::cameraDeckLink = camera1;
    myServer.start();

    while (true) {
    	cv::Mat img = camera1->captureLastCvMat();

    	cv::imshow("camera1", img);

    	if (cvWaitKey(5) >= 0)
    		continue;
    }

    myServer.stop();
    return 0;

}
