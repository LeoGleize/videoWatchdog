//============================================================================
// Name        : IntensityAquisition.cpp
// Author      : Filipe Caldas (fcaldas@canal-plus.fr)
// Version     : 1.0
// Copyright   : 
// Description : Video acquisition an analysis software
//============================================================================

#include <iostream>
#include "blackmagic/cameradecklink.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <omp.h>
#include "ServerInstance/ServerInstance.h"
using namespace std;

int main(int argc, char **argv) {
	RestServer::ServerInstance myServer;
	bool showScreen = true;
	bool resize = false;
	bool isFullHD = true;
	int Xres, Yres;
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		std::cout << "videoWatchdog Server" << std::endl
				<< std::endl;
		std::cout << "Options:" << std::endl;
		std::cout
				<< "	-no-video : video display is not shown, server will be executed normally"
				<< std::endl;
		std::cout << "    -res X Y  : set resolution of window" << std::endl;
		std::cout << "    -720p : set resolution of input to 720p instead of 1080i" << std::endl;
		return 0;
	} else if (argc == 2 && strcmp(argv[1], "-no-video") == 0) {
		showScreen = false;
	} else if (argc == 4 && strcmp(argv[1], "-res") == 0) {
		std::cout << "Changing window resolution to " << argv[2] << " x "
				<< argv[3] << std::endl << std::flush;
		Xres = std::atoi(argv[2]);
		Yres = std::atoi(argv[3]);
		resize = true;
	} else if (argc == 2 && strcmp(argv[1], "-720p") == 0) {
		std::cout << "Output set to 720p"<< std::endl;
		isFullHD = false;
	}
	CameraDecklink *camera1;

	camera1 = new CameraDecklink(isFullHD);
	RestServer::ServerInstance::cameraDeckLink = camera1;

	myServer.start();

	while (true) {
		if (showScreen) {
			cv::Mat img;
			bool deleteAfter = false;
			IplImage *dataToFree;
			try{

				img = camera1->captureLastCvMat(&dataToFree);
				deleteAfter = true;
			}catch(const CardException &e){
				if(isFullHD)
					img = cv::Mat(1080,1920, CV_8UC3, cv::Scalar(0,0,0));
				else
					img = cv::Mat(720,1280, CV_8UC3, cv::Scalar(0,0,0));
				cv::putText( img,
							 e.what(),
			    			 cv::Point(100,100),
			    			 CV_FONT_HERSHEY_PLAIN,
			    			 1.8,
			    			 cv::Scalar(250,250,250)
			    		   );
			}
			if (!resize){
				cv::imshow("VideoWatchdog Server", img);
			}else {
				cv::Mat resizedimg(Yres, Xres, CV_8U);
				cv::resize(img, resizedimg, resizedimg.size(), 0, 0, cv::INTER_CUBIC);
				cv::imshow("VideoWatchdog Server", resizedimg);
			}
			if(deleteAfter)
				cvRelease((void**) &dataToFree);

		}

		if (cvWaitKey(15) >= 0)
			continue;
	}
	myServer.stop();
	return 0;
}
