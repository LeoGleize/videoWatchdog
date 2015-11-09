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

dependencies:

	g++, libpthread, 

	restcpp: Also known as Casablanca this in an REST server implementation and JSON parser library
		 https://casablanca.codeplex.com
	
	opencv:	 OpenComputer Vision used for image processing in the software
		 
	BOOST: sudo apt-get install libboost-dev
