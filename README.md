R7IntensityProServer
--------------------
	
	R7IntensityPro Server is a software developed for screen monitoring and video processing using an IntensityPro acquisition card.
	The main idea is to connect a STB as video input and offer multiple possibilities of image analysis and event detection, the
	project was written in C++ and is divided in the following components:
	
		
		Main thread: implemented in ./IntensityAcquisition.cpp, this file is responsible for generating real time video output
					 using the data acquired from IntensityPro.
					 
		blackmagic/: This folder contains files used for communicating with IntensityPro drivers and acquiring video and audio,
					 it also contains a SDK for developing software for Blackmagic products.
					 
		recognition/: This folder contains some files specialized in image processing, and implement template matching and level monitoring
					  for OpenCV material objects (cv::Mat).
		
		ServerInstance/: This folder has source code related to the REST Server implementation and declaration of routes.
		
		watchdog/: Implementation of an watchdog that monitors all video output and reports events.
		

	To build R7IntensityPro first run the script installDep.sh, this will install all the dependencies needed, and then you can simply run 'make' and 'sudo make install'.
	Before running the program you'll need to open "Blackmagic Desktop Video Utility" and change "video input" to "component" instead of "hdmi". Now you can use Media Express and check
	you also need to change the resolution on Edit -> Preferences -> "Project Video Format" to either 1080i50 or 720p50 depending on your STB configuration.
	If the card is working correctly, you can see the frames being acquired on "Log and capture" tab.

	Now you can copy the file config.json to the directory where you'll be executing R7IntensityProServer, you'll also need to modify this file to point to your IP address.
	
~/workspace/R7IntensityProServer$ cp config.json ~
~/workspace/R7IntensityProServer$ cd
~$ R7IntensityProServer -h

Intensity Pro Acquisition Server

Options:
    -no-video : video display is not shown, server will be executed normally
    -res X Y  : set resolution of window
    -720p : set resolution of input to 720p instead of 1080i


dependencies:

	g++, libpthread, Boost, opencv, restcpp
	
