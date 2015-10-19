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


int main(int argc, char **argv) {
	RestServer::ServerInstance myServer;
	bool showScreen = true;
	bool resize = false;
	int Xres, Yres;
	if(argc == 2 && strcmp(argv[1],"-h") == 0){
		std::cout<<"Intensity Pro Acquisition Server"<<std::endl<<std::endl;
		std::cout<<"Options:"<<std::endl;
		std::cout<<"	-no-video : video display is not shown, server will be executed normally"<<std::endl;
		std::cout<<"    -res X Y  : set resolution of window"<<std::endl;
		return 0;
	}else if(argc == 2 && strcmp(argv[1],"-no-video") == 0){
		showScreen = false;
	}else if(argc == 4 && strcmp(argv[1],"-res") == 0){
		std::cout<<"Changing window resolution to "<<argv[2]<<" x "<<argv[3]<<std::endl<<std::flush;
		Xres = std::atoi(argv[2]);
		Yres = std::atoi(argv[3]);
		resize = true;
	}
    CameraDecklink    *camera1;

    camera1 = new CameraDecklink();
    RestServer::ServerInstance::cameraDeckLink = camera1;
    myServer.start();

    //we wont be timing this since this is just to give an idea to the user of what is going
    //on on the STB screen
    while (true) {
    	if(showScreen){
    		cv::Mat img = camera1->captureLastCvMat();
    		if(!resize)
    			cv::imshow("IntensityPro", img);
    		else{
    			cv::Mat resizedimg(Yres,Xres,CV_8U);
    			cv::resize(img, resizedimg, resizedimg.size(),0,0,cv::INTER_CUBIC);
    			cv::imshow("IntensityPro", resizedimg);
    		}
    	}

    	if (cvWaitKey(10) >= 0)
    		continue;
    }
    myServer.stop();
    return 0;
}
